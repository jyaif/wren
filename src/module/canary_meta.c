
#include "canary_meta.h"

#include <string.h>

#include "wren_vm.h"

void metaCompile(WrenVM* vm)
{
  const char* source = wrenGetSlotString(vm, 1);
  bool isExpression = wrenGetSlotBool(vm, 2);
  bool printErrors = wrenGetSlotBool(vm, 3);

  // TODO: Allow passing in module?
  // Look up the module surrounding the callsite. This is brittle. The -2 walks
  // up the callstack assuming that the meta module has one level of
  // indirection before hitting the user's code. Any change to meta may require
  // this constant to be tweaked.
  ObjFiber* currentFiber = vm->fiber;
  ObjFn* fn = currentFiber->frames[currentFiber->numFrames - 2].closure->fn;
  ObjString* module = fn->module->name;

  ObjClosure* closure = wrenCompileSource(vm, module->value, source,
                                          isExpression, printErrors);
  
  // Return the result. We can't use the public API for this since we have a
  // bare ObjClosure*.
  canary_thread_set_slot(vm->fiber, 0,
                         closure != NULL ? OBJ_VAL(closure) : NULL_VAL);
}

void metaGetModuleVariables(WrenVM* vm)
{
  wrenSetSlotCount(vm, 3);
  
  Value moduleValue = wrenMapGet(vm->modules,
                                 canary_thread_get_slot(vm->fiber, 1));
  if (IS_UNDEFINED(moduleValue))
  {
    canary_thread_set_slot(vm->fiber, 0, NULL_VAL);
    return;
  }
    
  ObjModule* module = AS_MODULE(moduleValue);
  ObjList* names = wrenNewList(vm, module->variableNames.count);
  canary_thread_set_slot(vm->fiber, 0, OBJ_VAL(names));

  // Initialize the elements to null in case a collection happens when we
  // allocate the strings below.
  for (int i = 0; i < names->elements.count; i++)
  {
    names->elements.data[i] = NULL_VAL;
  }
  
  for (int i = 0; i < names->elements.count; i++)
  {
    names->elements.data[i] = OBJ_VAL(module->variableNames.data[i]);
  }
}
