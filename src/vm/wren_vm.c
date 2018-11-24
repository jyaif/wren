
#include "wren_vm.h"

#include "wren_core.h"
#include "wren_debug.h"

#include "canary_api.h"
#include "canary_vm.h"

#include <stdlib.h>

#if WREN_DEBUG_TRACE_MEMORY || WREN_DEBUG_TRACE_GC
  #include <time.h>
  #include <stdio.h>
#endif

// The behavior of realloc() when the size is 0 is implementation defined. It
// may return a non-NULL pointer which must not be dereferenced but nevertheless
// should be freed. To prevent that, we avoid calling realloc() with a zero
// size.
static void *defaultReallocate(void *user_data, void *ptr, size_t newSize)
{
  if (newSize == 0)
  {
    free(ptr);
    return NULL;
  }

  return realloc(ptr, newSize);
}

void wrenInitConfiguration(WrenConfiguration* config)
{
  config->reallocateFn = defaultReallocate;
  config->resolveModuleFn = NULL;
  config->loadModuleFn = NULL;
  config->bindForeignMethodFn = NULL;
  config->bindForeignClassFn = NULL;
  config->writeFn = NULL;
  config->errorFn = NULL;
  config->initialHeapSize = 1024 * 1024 * 10;
  config->minHeapSize = 1024 * 1024;
  config->heapGrowthPercent = 50;
  config->userData = NULL;
}

WrenVM* wrenNewVM(WrenConfiguration* config)
{
  WrenReallocateFn reallocate = defaultReallocate;
  void *user_data = NULL;
  if (config != NULL) {
    reallocate = config->reallocateFn;
    user_data = config->userData;
  }
  
  WrenVM* vm = canary_vm_bootstrap(reallocate, user_data);
  memset(vm, 0, sizeof(WrenVM));

  // Copy the configuration if given one.
  if (config != NULL)
  {
    memcpy(&vm->config, config, sizeof(WrenConfiguration));
  }
  else
  {
    wrenInitConfiguration(&vm->config);
  }

  // TODO: Should we allocate and free this during a GC?
  vm->grayCount = 0;
  // TODO: Tune this.
  vm->grayCapacity = 4;
  // Cannot use realloc canary_vm_malloc here, since it might trigger the gc.
  vm->gray = (Obj**)vm->config.reallocateFn(vm->config.userData, vm->gray,
                                            sizeof(Obj*) * vm->grayCapacity);
  vm->nextGC = vm->config.initialHeapSize;

  wrenSymbolTableInit(&vm->methodNames);

  vm->modules = wrenNewMap(vm);
  wrenInitializeCore(vm);
  return vm;
}

WrenVM* wrenVMFromContext(canary_context_t *context)
{
  return canary_context_to_vm(context);
}

void wrenFreeVM(WrenVM* vm)
{
  ASSERT(vm->methodNames.count > 0, "VM appears to have already been freed.");
  
  // Free all of the GC objects.
  Obj* obj = vm->first;
  while (obj != NULL)
  {
    Obj* next = obj->next;
    wrenFreeObj(vm, obj);
    obj = next;
  }

  // Free up the GC gray set.
  // Cannot use realloc canary_vm_free here, since it might trigger the gc.
  vm->gray = (Obj**)vm->config.reallocateFn(vm->config.userData, vm->gray, 0);

  // Tell the user if they didn't free any handles. We don't want to just free
  // them here because the host app may still have pointers to them that they
  // may try to use. Better to tell them about the bug early.
  ASSERT(vm->handles == NULL, "All handles have not been released.");

  wrenSymbolTableClear(vm, &vm->methodNames);

  canary_vm_free(vm, vm);
}

void wrenCollectGarbage(WrenVM* vm)
{
#if WREN_DEBUG_TRACE_MEMORY || WREN_DEBUG_TRACE_GC
  printf("-- gc --\n");

  size_t before = vm->bytesAllocated;
  double startTime = (double)clock() / CLOCKS_PER_SEC;
#endif

  // Mark all reachable objects.

  wrenGrayObj(vm, (Obj*)vm->modules);

  // Temporary roots.
  for (int i = 0; i < vm->numTempRoots; i++)
  {
    wrenGrayObj(vm, vm->tempRoots[i]);
  }

  // The current fiber.
  wrenGrayObj(vm, (Obj*)vm->fiber);

  // The handles.
  for (WrenHandle* handle = vm->handles;
       handle != NULL;
       handle = handle->next)
  {
    wrenGrayValue(vm, handle->value);
  }

  // Any object the compiler is using (if there is one).
  if (vm->compiler != NULL) wrenMarkCompiler(vm, vm->compiler);

  // Method names.
  wrenBlackenSymbolTable(vm, &vm->methodNames);

  // Now that we have grayed the roots, do a depth-first search over all of the
  // reachable objects.
  wrenBlackenObjects(vm);

  // Collect the white objects.
  Obj** obj = &vm->first;
  while (*obj != NULL)
  {
    if (!((*obj)->isDark))
    {
      // This object wasn't reached, so remove it from the list and free it.
      Obj* unreached = *obj;
      *obj = unreached->next;
      wrenFreeObj(vm, unreached);
    }
    else
    {
      // This object was reached, so unmark it (for the next GC) and move on to
      // the next.
      (*obj)->isDark = false;
      obj = &(*obj)->next;
    }
  }

  // Calculate the next gc point, this is the current allocation plus
  // a configured percentage of the current allocation.
  vm->nextGC = vm->bytesAllocated + ((vm->bytesAllocated * vm->config.heapGrowthPercent) / 100);
  if (vm->nextGC < vm->config.minHeapSize) vm->nextGC = vm->config.minHeapSize;

#if WREN_DEBUG_TRACE_MEMORY || WREN_DEBUG_TRACE_GC
  double elapsed = ((double)clock() / CLOCKS_PER_SEC) - startTime;
  // Explicit cast because size_t has different sizes on 32-bit and 64-bit and
  // we need a consistent type for the format string.
  printf("GC %lu before, %lu after (%lu collected), next at %lu. Took %.3fs.\n",
         (unsigned long)before,
         (unsigned long)vm->bytesAllocated,
         (unsigned long)(before - vm->bytesAllocated),
         (unsigned long)vm->nextGC,
         elapsed);
#endif
}

// Looks up a foreign method in [moduleName] on [className] with [signature].
//
// This will try the host's foreign method binder first. If that fails, it
// falls back to handling the built-in modules.
static WrenForeignMethodFn findForeignMethod(WrenVM* vm,
                                             const char* moduleName,
                                             const char* className,
                                             bool isStatic,
                                             const char* signature)
{
  WrenForeignMethodFn method = NULL;
  
  if (vm->config.bindForeignMethodFn != NULL)
  {
    method = vm->config.bindForeignMethodFn(vm, moduleName, className, isStatic,
                                            signature);
  }
  
  return method;
}

// Defines [methodValue] as a method on [classObj].
//
// Handles both foreign methods where [methodValue] is a string containing the
// method's signature and Wren methods where [methodValue] is a function.
//
// Aborts the current fiber if the method is a foreign method that could not be
// found.
void bindMethod(WrenVM* vm, int methodType, int symbol,
                ObjModule* module, ObjClass* classObj, Value methodValue)
{
  const char* className = classObj->name->value;
  if (methodType == CODE_METHOD_STATIC) classObj = classObj->obj.classObj;

  Method method;
  if (IS_STRING(methodValue))
  {
    const char* name = AS_CSTRING(methodValue);
    method.type = METHOD_FOREIGN;
    method.as.foreign = findForeignMethod(vm, module->name->value,
                                          className,
                                          methodType == CODE_METHOD_STATIC,
                                          name);

    if (method.as.foreign == NULL)
    {
      canary_thread_set_error_str_format(vm->fiber,
          "Could not find foreign method '@' for class $ in module '$'.",
          methodValue, classObj->name->value, module->name->value);
      return;
    }
  }
  else
  {
    method.as.closure = AS_CLOSURE(methodValue);
    method.type = METHOD_BLOCK;

    // Patch up the bytecode now that we know the superclass.
    wrenBindMethodCode(classObj, method.as.closure->fn);
  }

  wrenBindMethod(vm, classObj, symbol, method);
}

// Looks up the previously loaded module with [name].
//
// Returns `NULL` if no module with that name has been loaded.
static ObjModule* getModule(WrenVM* vm, Value name)
{
  Value moduleValue = wrenMapGet(vm->modules, name);
  return !IS_UNDEFINED(moduleValue) ? AS_MODULE(moduleValue) : NULL;
}

static ObjClosure* compileInModule(WrenVM* vm, Value name, const char* source,
                                   bool isExpression, bool printErrors)
{
  // See if the module has already been loaded.
  ObjModule* module = getModule(vm, name);
  if (module == NULL)
  {
    module = wrenNewModule(vm, AS_STRING(name));

    // Store it in the VM's module registry so we don't load the same module
    // multiple times.
    wrenMapSet(vm, vm->modules, name, OBJ_VAL(module));

    // Implicitly import the core module.
    ObjModule* coreModule = getModule(vm, NULL_VAL);
    for (int i = 0; i < coreModule->variables.count; i++)
    {
      wrenDefineVariable(vm, module,
                         coreModule->variableNames.data[i]->value,
                         coreModule->variableNames.data[i]->length,
                         coreModule->variables.data[i]);
    }
  }

  ObjFn* fn = wrenCompile(vm, module, source, isExpression, printErrors);
  if (fn == NULL)
  {
    // TODO: Should we still store the module even if it didn't compile?
    return NULL;
  }

  // Functions are always wrapped in closures.
  wrenPushRoot(vm, (Obj*)fn);
  ObjClosure* closure = wrenNewClosure(vm, fn);
  wrenPopRoot(vm); // fn.

  return closure;
}

// Verifies that [superclassValue] is a valid object to inherit from. That
// means it must be a class and cannot be the class of any built-in type.
//
// Also validates that it doesn't result in a class with too many fields and
// the other limitations foreign classes have.
//
// If successful, returns `null`. Otherwise, returns a string for the runtime
// error message.
static Value validateSuperclass(WrenVM* vm, Value name, Value superclassValue,
                                int numFields)
{
  // Make sure the superclass is a class.
  if (!IS_CLASS(superclassValue))
  {
    return wrenStringFormat(vm,
        "Class '@' cannot inherit from a non-class object.",
        name);
  }

  // Make sure it doesn't inherit from a sealed built-in type. Primitive methods
  // on these classes assume the instance is one of the other Obj___ types and
  // will fail horribly if it's actually an ObjInstance.
  ObjClass* superclass = AS_CLASS(superclassValue);
  if (superclass == vm->classClass ||
      superclass == vm->fiberClass ||
      superclass == vm->fnClass || // Includes OBJ_CLOSURE.
      superclass == vm->listClass ||
      superclass == vm->mapClass ||
      superclass == vm->rangeClass ||
      superclass == vm->stringClass)
  {
    return wrenStringFormat(vm,
        "Class '@' cannot inherit from built-in class '@'.",
        name, OBJ_VAL(superclass->name));
  }

  if (superclass->numFields == -1)
  {
    return wrenStringFormat(vm,
        "Class '@' cannot inherit from foreign class '@'.",
        name, OBJ_VAL(superclass->name));
  }

  if (numFields == -1 && superclass->numFields > 0)
  {
    return wrenStringFormat(vm,
        "Foreign class '@' may not inherit from a class with fields.",
        name);
  }

  if (superclass->numFields + numFields > MAX_FIELDS)
  {
    return wrenStringFormat(vm,
        "Class '@' may not have more than 255 fields, including inherited "
        "ones.", name);
  }

  return NULL_VAL;
}

static void bindForeignClass(WrenVM* vm, ObjClass* classObj, ObjModule* module)
{
  WrenForeignClassMethods methods;
  methods.allocate = NULL;
  methods.finalize = NULL;
  
  // Check the optional built-in module first so the host can override it.
  
  if (vm->config.bindForeignClassFn != NULL)
  {
    methods = vm->config.bindForeignClassFn(vm, module->name->value,
                                            classObj->name->value);
  }
  
  Method method;
  method.type = METHOD_FOREIGN;

  // Add the symbol even if there is no allocator so we can ensure that the
  // symbol itself is always in the symbol table.
  int symbol = wrenSymbolTableEnsure(vm, &vm->methodNames, "<allocate>", 10);
  if (methods.allocate != NULL)
  {
    method.as.foreign = methods.allocate;
    wrenBindMethod(vm, classObj, symbol, method);
  }
  
  // Add the symbol even if there is no finalizer so we can ensure that the
  // symbol itself is always in the symbol table.
  symbol = wrenSymbolTableEnsure(vm, &vm->methodNames, "<finalize>", 10);
  if (methods.finalize != NULL)
  {
    method.as.foreign = (WrenForeignMethodFn)methods.finalize;
    wrenBindMethod(vm, classObj, symbol, method);
  }
}

// Creates a new class.
//
// If [numFields] is -1, the class is a foreign class. The name and superclass
// should be on top of the fiber's stack. After calling this, the top of the
// stack will contain the new class.
//
// Aborts the current fiber if an error occurs.
void createClass(WrenVM* vm, int numFields, ObjModule* module)
{
  // Pull the name and superclass off the stack.
  Value name = vm->fiber->stackTop[-2];
  Value superclass = vm->fiber->stackTop[-1];

  // We have two values on the stack and we are going to leave one, so discard
  // the other slot.
  vm->fiber->stackTop--;

  canary_thread_set_error(vm->fiber,
                          validateSuperclass(vm, name, superclass, numFields));
  if (canary_thread_has_error(vm->fiber)) return;

  ObjClass* classObj = wrenNewClass(vm, AS_CLASS(superclass), numFields,
                                    AS_STRING(name));
  vm->fiber->stackTop[-1] = OBJ_VAL(classObj);

  if (numFields == -1) bindForeignClass(vm, classObj, module);
}

void createForeign(ObjFiber* fiber, Value* stack)
{
  canary_context_t *context = canary_context_from_thread(fiber);
  WrenVM* vm = fiber->vm;
  ObjClass* classObj = AS_CLASS(stack[0]);
  ASSERT(classObj->numFields == -1, "Class must be a foreign class.");

  // TODO: Don't look up every time.
  int symbol = wrenSymbolTableFind(&vm->methodNames, "<allocate>", 10);
  ASSERT(symbol != -1, "Should have defined <allocate> symbol.");

  ASSERT(classObj->methods.count > symbol, "Class should have allocator.");
  Method* method = &classObj->methods.data[symbol];
  ASSERT(method->type == METHOD_FOREIGN, "Allocator should be foreign.");

  // Pass the constructor arguments to the allocator as well.
  bool old_is_api_call = vm->is_api_call;
  vm->is_api_call = true;
  ptrdiff_t old_stack_base_diff = fiber->stack_base - fiber->stack;
  fiber->stack_base = stack;

  method->as.foreign(context);

  fiber->stack_base = fiber->stack + old_stack_base_diff;
  vm->is_api_call = old_is_api_call;
  // TODO: Check that allocateForeign was called.
}

// Let the host resolve an imported module name if it wants to.
static Value resolveModule(WrenVM* vm, Value name)
{
  // If the host doesn't care to resolve, leave the name alone.
  if (vm->config.resolveModuleFn == NULL) return name;

  ObjFiber* fiber = vm->fiber;
  ObjFn* fn = fiber->frames[fiber->numFrames - 1].closure->fn;
  ObjString* importer = fn->module->name;
  
  const char* resolved = vm->config.resolveModuleFn(vm, importer->value,
                                                    AS_CSTRING(name));
  if (resolved == NULL)
  {
    canary_thread_set_error_str_format(fiber,
        "Could not resolve module '@' imported from '@'.",
        name, OBJ_VAL(importer));
    return NULL_VAL;
  }
  
  // If they resolved to the exact same string, we don't need to copy it.
  if (resolved == AS_CSTRING(name)) return name;

  // Copy the string into a Wren String object.
  name = wrenNewString(vm, resolved);
  
  // FIXME: The mallocator used here is unknown. Change to something under the
  //        vm control.
  free((char*)resolved);
  return name;
}

Value importModule(WrenVM* vm, Value name)
{
  name = resolveModule(vm, name);
  
  // If the module is already loaded, we don't need to do anything.
  Value existing = wrenMapGet(vm->modules, name);
  if (!IS_UNDEFINED(existing)) return existing;

  wrenPushRoot(vm, AS_OBJ(name));

  const char* source = NULL;
  
  // Let the host try to provide the module.
  if (vm->config.loadModuleFn != NULL)
  {
    source = vm->config.loadModuleFn(vm, AS_CSTRING(name));
  }
  
  if (source == NULL)
  {
    canary_thread_set_error_str_format(vm->fiber,
                                       "Could not load module '@'.", name);
    wrenPopRoot(vm); // name.
    return NULL_VAL;
  }
  
  ObjClosure* moduleClosure = compileInModule(vm, name, source, false, true);
  
  // Modules loaded by the host are expected to be dynamically allocated with
  // ownership given to the VM, which will free it. The built in optional
  // modules are constant strings which don't need to be freed.
  //
  // FIXME: The mallocator used here is unknown. Change to something under the
  //        vm control.
  vm->config.reallocateFn(vm->config.userData, (char*)source, 0);
  
  if (moduleClosure == NULL)
  {
    canary_thread_set_error_str_format(vm->fiber,
                                       "Could not compile module '@'.", name);
    wrenPopRoot(vm); // name.
    return NULL_VAL;
  }

  wrenPopRoot(vm); // name.

  // Return the closure that executes the module.
  return OBJ_VAL(moduleClosure);
}

Value getModuleVariable(WrenVM* vm, ObjModule* module, Value variableName)
{
  ObjString* variable = AS_STRING(variableName);
  uint32_t variableEntry = wrenSymbolTableFind(&module->variableNames,
                                               variable->value,
                                               variable->length);
  
  // It's a runtime error if the imported variable does not exist.
  if (variableEntry != UINT32_MAX)
  {
    return module->variables.data[variableEntry];
  }
  
  canary_thread_set_error_str_format(vm->fiber,
      "Could not find a variable named '@' in module '@'.",
      variableName, OBJ_VAL(module->name));
  return NULL_VAL;
}

WrenHandle* wrenMakeCallHandle(WrenVM* vm, const char* signature)
{
  ASSERT(signature != NULL, "Signature cannot be NULL.");
  
  int signatureLength = (int)strlen(signature);
  ASSERT(signatureLength > 0, "Signature cannot be empty.");
  
  // Count the number parameters the method expects.
  int numParams = 0;
  if (signature[signatureLength - 1] == ')')
  {
    for (int i = signatureLength - 1; i > 0 && signature[i] != '('; i--)
    {
      if (signature[i] == '_') numParams++;
    }
  }
  
  // Count subscript arguments.
  if (signature[0] == '[')
  {
    for (int i = 0; i < signatureLength && signature[i] != ']'; i++)
    {
      if (signature[i] == '_') numParams++;
    }
  }
  
  // Add the signatue to the method table.
  int method =  wrenSymbolTableEnsure(vm, &vm->methodNames,
                                      signature, signatureLength);
  
  // Create a little stub function that assumes the arguments are on the stack
  // and calls the method.
  ObjFn* fn = wrenNewFunction(vm, NULL, numParams + 1);
  
  // Wrap the function in a closure and then in a handle. Do this here so it
  // doesn't get collected as we fill it in.
  WrenHandle* value = wrenMakeHandle(vm, OBJ_VAL(fn));
  value->value = OBJ_VAL(wrenNewClosure(vm, fn));
  
  wrenByteBufferWrite(vm, &fn->code, (uint8_t)(CODE_CALL_0 + numParams));
  wrenByteBufferWrite(vm, &fn->code, (method >> 8) & 0xff);
  wrenByteBufferWrite(vm, &fn->code, method & 0xff);
  wrenByteBufferWrite(vm, &fn->code, CODE_RETURN);
  wrenByteBufferWrite(vm, &fn->code, CODE_END);
  wrenIntBufferFill(vm, &fn->debug->sourceLines, 0, 5);
  wrenFunctionBindName(vm, fn, signature, signatureLength);

  return value;
}

WrenInterpretResult wrenCall(WrenVM* vm, WrenHandle* method)
{
  ASSERT(method != NULL, "Method cannot be NULL.");
  ASSERT(IS_CLOSURE(method->value), "Method must be a method handle.");
  ASSERT(vm->fiber != NULL, "Must set up arguments for call first.");
  ASSERT(vm->is_api_call, "Must set up arguments for call first.");
  ASSERT(vm->fiber->numFrames == 0, "Can not call from a foreign method.");
  
  ObjFiber* fiber = vm->fiber;
  ObjClosure* closure = AS_CLOSURE(method->value);
  WrenSlot slots = closure->fn->arity + 1;
  ASSERT(canary_thread_get_stack_size(fiber) >= slots,
         "Stack must have enough arguments for method.");
  
  canary_thread_call_function(fiber, closure, slots);
  return canary_thread_interpret(fiber);
}

WrenHandle* wrenMakeHandle(WrenVM* vm, Value value)
{
  if (IS_OBJ(value)) wrenPushRoot(vm, AS_OBJ(value));
  
  // Make a handle for it.
  WrenHandle* handle = ALLOCATE(vm, WrenHandle);
  handle->value = value;

  if (IS_OBJ(value)) wrenPopRoot(vm);

  // Add it to the front of the linked list of handles.
  if (vm->handles != NULL) vm->handles->prev = handle;
  handle->prev = NULL;
  handle->next = vm->handles;
  vm->handles = handle;
  
  return handle;
}

void wrenReleaseHandle(WrenVM* vm, WrenHandle* handle)
{
  ASSERT(handle != NULL, "Handle cannot be NULL.");

  // Update the VM's head pointer if we're releasing the first handle.
  if (vm->handles == handle) vm->handles = handle->next;

  // Unlink it from the list.
  if (handle->prev != NULL) handle->prev->next = handle->next;
  if (handle->next != NULL) handle->next->prev = handle->prev;

  // Clear it out. This isn't strictly necessary since we're going to free it,
  // but it makes for easier debugging.
  handle->prev = NULL;
  handle->next = NULL;
  handle->value = NULL_VAL;
  DEALLOCATE(vm, handle);
}

WrenInterpretResult wrenInterpret(WrenVM* vm, const char* module,
                                  const char* source)
{
  ObjClosure* closure = wrenCompileSource(vm, module, source, false, true);
  if (closure == NULL) return WREN_RESULT_COMPILE_ERROR;
  
  wrenPushRoot(vm, (Obj*)closure);
  ObjFiber* fiber = canary_thread_new(vm, closure);
  wrenPopRoot(vm); // closure.
  
  return canary_thread_interpret(fiber);
}

ObjClosure* wrenCompileSource(WrenVM* vm, const char* module, const char* source,
                            bool isExpression, bool printErrors)
{
  Value nameValue = NULL_VAL;
  if (module != NULL)
  {
    nameValue = wrenNewString(vm, module);
    wrenPushRoot(vm, AS_OBJ(nameValue));
  }
  
  ObjClosure* closure = compileInModule(vm, nameValue, source,
                                        isExpression, printErrors);

  if (module != NULL) wrenPopRoot(vm); // nameValue.
  return closure;
}

Value wrenGetModuleVariable(WrenVM* vm, Value moduleName, Value variableName)
{
  ObjModule* module = getModule(vm, moduleName);
  if (module == NULL)
  {
    canary_thread_set_error_str_format(vm->fiber,
                                       "Module '@' is not loaded.", moduleName);
    return NULL_VAL;
  }
  
  return getModuleVariable(vm, module, variableName);
}

Value wrenFindVariable(WrenVM* vm, ObjModule* module, const char* name)
{
  int symbol = wrenSymbolTableFind(&module->variableNames, name, strlen(name));
  return module->variables.data[symbol];
}

int wrenDeclareVariable(WrenVM* vm, ObjModule* module, const char* name,
                        size_t length, int line)
{
  if (module->variables.count == MAX_MODULE_VARS) return -2;

  // Implicitly defined variables get a "value" that is the line where the
  // variable is first used. We'll use that later to report an error on the
  // right line.
  wrenValueBufferWrite(vm, &module->variables, NUM_VAL(line));
  return wrenSymbolTableAdd(vm, &module->variableNames, name, length);
}

int wrenDefineVariable(WrenVM* vm, ObjModule* module, const char* name,
                       size_t length, Value value)
{
  if (module->variables.count == MAX_MODULE_VARS) return -2;

  if (IS_OBJ(value)) wrenPushRoot(vm, AS_OBJ(value));

  // See if the variable is already explicitly or implicitly declared.
  int symbol = wrenSymbolTableFind(&module->variableNames, name, length);

  if (symbol == -1)
  {
    // Brand new variable.
    symbol = wrenSymbolTableAdd(vm, &module->variableNames, name, length);
    wrenValueBufferWrite(vm, &module->variables, value);
  }
  else if (IS_NUM(module->variables.data[symbol]))
  {
    // An implicitly declared variable's value will always be a number. Now we
    // have a real definition.
    module->variables.data[symbol] = value;
  }
  else
  {
    // Already explicitly declared.
    symbol = -1;
  }

  if (IS_OBJ(value)) wrenPopRoot(vm);

  return symbol;
}

// TODO: Inline?
void wrenPushRoot(WrenVM* vm, Obj* obj)
{
  ASSERT(obj != NULL, "Can't root NULL.");
  ASSERT(vm->numTempRoots < WREN_MAX_TEMP_ROOTS, "Too many temporary roots.");

  vm->tempRoots[vm->numTempRoots++] = obj;
}

void wrenPopRoot(WrenVM* vm)
{
  ASSERT(vm->numTempRoots > 0, "No temporary roots to release.");
  vm->numTempRoots--;
}

WrenSlot wrenGetSlotCount(WrenVM* vm)
{
  if (!vm->is_api_call) return 0;
  
  return canary_thread_get_frame_size(vm->fiber);
}

void wrenSetSlotCount(WrenVM* vm, WrenSlot numSlots)
{
  // If we don't have a fiber accessible, create one for the API to use.
  if (!vm->is_api_call)
  {
    vm->is_api_call = true;
    vm->fiber = canary_thread_new(vm, NULL);
  }
  canary_thread_set_frame_size(vm->fiber, numSlots);
}

// Gets the type of the object in [slot].
WrenType wrenGetSlotType(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  
  if (IS_BOOL(value)) return WREN_TYPE_BOOL;
  if (IS_NUM(value)) return WREN_TYPE_NUM;
  if (IS_FOREIGN(value)) return WREN_TYPE_FOREIGN;
  if (IS_LIST(value)) return WREN_TYPE_LIST;
  if (IS_MAP(value)) return WREN_TYPE_MAP;
  if (IS_NULL(value)) return WREN_TYPE_NULL;
  if (IS_STRING(value)) return WREN_TYPE_STRING;
  
  return WREN_TYPE_UNKNOWN;
}

bool wrenGetSlotBool(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  ASSERT(IS_BOOL(value), "Slot must hold a bool.");
  
  return AS_BOOL(value);
}

const void* wrenGetSlotBytes(WrenVM* vm, WrenSlot srcSlot, size_t* length)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  ASSERT(IS_STRING(value), "Slot must hold a string.");
  
  ObjString* string = AS_STRING(value);
  *length = string->length;
  return string->value;
}

double wrenGetSlotDouble(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  ASSERT(IS_NUM(value), "Slot must hold a number.");
  
  return AS_NUM(value);
}

void* wrenGetSlotForeign(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  ASSERT(IS_FOREIGN(value), "Slot must hold a foreign instance.");
  
  return AS_FOREIGN(value)->data;
}

const char* wrenGetSlotString(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  ASSERT(IS_STRING(value), "Slot must hold a string.");
  
  return AS_CSTRING(value);
}

WrenHandle* wrenGetSlotHandle(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  
  return wrenMakeHandle(vm, value);
}

static Value getSlot(WrenVM* vm, WrenSlot srcSlot)
{
  return canary_thread_get_slot(vm->fiber, srcSlot);
}

void wrenSetSlotBool(WrenVM* vm, WrenSlot dstSlot, bool value)
{
  canary_thread_set_slot(vm->fiber, dstSlot, BOOL_VAL(value));
}

void wrenSetSlotBytes(WrenVM* vm, WrenSlot dstSlot,
                      const void* bytes, size_t length)
{
  ASSERT(bytes != NULL, "Byte array cannot be NULL.");
  
  canary_thread_set_slot(vm->fiber, dstSlot,
                   wrenNewStringLength(vm, (const char*)bytes, length));
}

void wrenSetSlotDouble(WrenVM* vm, WrenSlot dstSlot, double value)
{
  canary_thread_set_slot(vm->fiber, dstSlot, NUM_VAL(value));
}

void* wrenSetSlotNewForeign(WrenVM* vm, WrenSlot dstSlot,
                            WrenSlot classSlot, size_t size)
{
  Value classValue = canary_thread_get_slot(vm->fiber, classSlot);
  ASSERT(IS_CLASS(classValue), "Slot must hold a class.");
  
  ObjClass* classObj = AS_CLASS(classValue);
  ASSERT(classObj->numFields == -1, "Class must be a foreign class.");
  
  ObjForeign* foreign = wrenNewForeign(vm, classObj, size);
  canary_thread_set_slot(vm->fiber, dstSlot, OBJ_VAL(foreign));
  return (void*)foreign->data;
}

void wrenSetSlotNewList(WrenVM* vm, WrenSlot dstSlot)
{
  canary_thread_set_slot(vm->fiber, dstSlot, OBJ_VAL(wrenNewList(vm, 0)));
}

void wrenSetSlotNull(WrenVM* vm, WrenSlot dstSlot)
{
  canary_thread_set_slot(vm->fiber, dstSlot, NULL_VAL);
}

void wrenSetSlotString(WrenVM* vm, WrenSlot dstSlot, const char* text)
{
  ASSERT(text != NULL, "String cannot be NULL.");
  
  canary_thread_set_slot(vm->fiber, dstSlot, wrenNewString(vm, text));
}

void wrenSetSlotHandle(WrenVM* vm, WrenSlot dstSlot, WrenHandle* handle)
{
  ASSERT(handle != NULL, "Handle cannot be NULL.");
  
  canary_thread_set_slot(vm->fiber, dstSlot, handle->value);
}

int wrenGetListCount(WrenVM* vm, WrenSlot listSlot)
{
  Value listValue = canary_thread_get_slot(vm->fiber, listSlot);
  ASSERT(IS_LIST(listValue), "Slot must hold a list.");
  
  ObjList* listObj = AS_LIST(listValue);
  return listObj->elements.count;
}

void wrenSetSlotNewMap (WrenVM* vm, WrenSlot slot)
{
  canary_thread_set_slot(vm->fiber, slot, OBJ_VAL(wrenNewMap(vm)));
}

 bool wrenGetMapValue (WrenVM* vm, int mapSlot, int keySlot, WrenSlot valueSlot)
{
  ASSERT(IS_MAP(getSlot(vm, mapSlot)), "Must be a map");
  ObjMap* map = AS_MAP(getSlot(vm, mapSlot));
  Value value = wrenMapGet(map, getSlot(vm, keySlot));
  if (valueSlot>=0)
  {
    if (IS_UNDEFINED(value)) {
       canary_thread_set_slot(vm->fiber, valueSlot, NULL_VAL);
    } else {
      canary_thread_set_slot(vm->fiber, valueSlot, value);
    }
  }
  return !IS_UNDEFINED(value);
}

/*
 void wrenPutInMap (WrenVM* vm, int mapSlot, int keySlot, int valueSlot)
{
  ASSERT(IS_MAP(getSlot(vm, mapSlot)), "Must be a map");
  ObjMap* map = AS_MAP(getSlot(vm, mapSlot));
  wrenMapSet(vm, map, getSlot(vm, keySlot), getSlot(vm, valueSlot));
}
 bool wrenRemoveMap (WrenVM* vm, int mapSlot, int keySlot, int valueSlot)
{
  ASSERT(IS_MAP(getSlot(vm, mapSlot)), "Must be a map");
  ObjMap* map = AS_MAP(getSlot(vm, mapSlot));
  Value removedValue = wrenMapRemoveKey(vm, map, getSlot(vm, keySlot));
  if (valueSlot>=0) setSlot(vm, valueSlot, removedValue);
  return !IS_NULL(removedValue);
}
 void wrenClearMap (WrenVM* vm, int mapSlot)
{
  ASSERT(IS_MAP(getSlot(vm, mapSlot)), "Must be a map");
  ObjMap* map = AS_MAP(getSlot(vm, mapSlot));
  wrenMapClear(vm, map);
}
*/

void wrenGetListElement(WrenVM* vm, WrenSlot dstSlot,
                        WrenSlot listSlot, int index)
{
  Value listValue = canary_thread_get_slot(vm->fiber, listSlot);
  ASSERT(IS_LIST(listValue), "Slot must hold a list.");
  
  ObjList* listObj = AS_LIST(listValue);
  canary_thread_set_slot(vm->fiber, dstSlot, listObj->elements.data[index]);
}

void wrenInsertInList(WrenVM* vm, WrenSlot listSlot, int index,
                      WrenSlot srcSlot)
{
  Value listValue = canary_thread_get_slot(vm->fiber, listSlot);
  ASSERT(IS_LIST(listValue), "Must insert into a list.");
  
  ObjList* list = AS_LIST(listValue);
  
  // Negative indices count from the end.
  if (index < 0) index = list->elements.count + 1 + index;
  
  ASSERT(index <= list->elements.count, "Index out of bounds.");
  
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  wrenListInsert(vm, list, value, index);
}

void wrenGetVariable(WrenVM* vm, WrenSlot dstSlot,
                     const char* module, const char* name)
{
  ASSERT(module != NULL, "Module cannot be NULL.");
  ASSERT(name != NULL, "Variable name cannot be NULL.");  
  
  Value moduleName = wrenStringFormat(vm, "$", module);
  wrenPushRoot(vm, AS_OBJ(moduleName));
  
  ObjModule* moduleObj = getModule(vm, moduleName);
  ASSERT(moduleObj != NULL, "Could not find module.");
  
  wrenPopRoot(vm); // moduleName.

  int variableSlot = wrenSymbolTableFind(&moduleObj->variableNames,
                                         name, strlen(name));
  ASSERT(variableSlot != -1, "Could not find variable.");
  
  canary_thread_set_slot(vm->fiber, dstSlot,
                         moduleObj->variables.data[variableSlot]);
}

void wrenAbortFiber(WrenVM* vm, WrenSlot srcSlot)
{
  Value value = canary_thread_get_slot(vm->fiber, srcSlot);
  
  canary_thread_set_error(vm->fiber, value);
}

void* wrenGetUserData(WrenVM* vm)
{
	return vm->config.userData;
}

void wrenSetUserData(WrenVM* vm, void* userData)
{
	vm->config.userData = userData;
}
