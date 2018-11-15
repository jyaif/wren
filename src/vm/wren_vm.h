#ifndef wren_vm_h
#define wren_vm_h

#include "wren_compiler.h"

#include "canary_vm.h"

// The maximum number of temporary objects that can be made visible to the GC
// at one time.
#define WREN_MAX_TEMP_ROOTS 5

typedef enum
{
  #define OPCODE(name, _) CODE_##name,
  #include "wren_opcodes.h"
  #undef OPCODE
} Code;

// A handle to a value, basically just a linked list of extra GC roots.
//
// Note that even non-heap-allocated values can be stored here.
struct WrenHandle
{
  Value value;

  WrenHandle* prev;
  WrenHandle* next;
};

struct WrenVM
{
  ObjClass* boolClass;
  ObjClass* classClass;
  ObjClass* fiberClass;
  ObjClass* fnClass;
  ObjClass* listClass;
  ObjClass* mapClass;
  ObjClass* nullClass;
  ObjClass* numClass;
  ObjClass* objectClass;
  ObjClass* rangeClass;
  ObjClass* stringClass;

  // The fiber that is currently running.
  ObjFiber* fiber;

  // The loaded modules. Each key is an ObjString (except for the main module,
  // whose key is null) for the module's name and the value is the ObjModule
  // for the module.
  ObjMap* modules;
  
  // The most recently imported module. More specifically, the module whose
  // code has most recently finished executing.
  //
  // Not treated like a GC root since the module is already in [modules].
  ObjModule* lastModule;

  // Memory management data:

  // The number of bytes that are known to be currently allocated. Includes all
  // memory that was proven live after the last GC, as well as any new bytes
  // that were allocated since then. Does *not* include bytes for objects that
  // were freed since the last GC.
  size_t bytesAllocated;

  // The number of total allocated bytes that will trigger the next GC.
  size_t nextGC;

  // The first object in the linked list of all currently allocated objects.
  Obj* first;

  // The "gray" set for the garbage collector. This is the stack of unprocessed
  // objects while a garbage collection pass is in process.
  Obj** gray;
  int grayCount;
  int grayCapacity;

  // The list of temporary roots. This is for temporary or new objects that are
  // not otherwise reachable but should not be collected.
  //
  // They are organized as a stack of pointers stored in this array. This
  // implies that temporary roots need to have stack semantics: only the most
  // recently pushed object can be released.
  Obj* tempRoots[WREN_MAX_TEMP_ROOTS];

  int numTempRoots;
  
  // Pointer to the first node in the linked list of active handles or NULL if
  // there are none.
  WrenHandle* handles;
  
  // Indicate if the C api is in use.
  //
  // If not in a foreign method, this is initially false. If the user requests
  // slots by calling wrenEnsureSlots(), a fiber is created and this is set to
  // true.
  bool is_api_call;
  
  WrenConfiguration config;
  
  // Compiler and debugger data:

  // The compiler that is currently compiling code. This is used so that heap
  // allocated objects used by the compiler can be found if a GC is kicked off
  // in the middle of a compile.
  Compiler* compiler;

  // There is a single global symbol table for all method names on all classes.
  // Method calls are dispatched directly by index in this table.
  SymbolTable methodNames;
};

// Creates a new [WrenHandle] for [value].
WrenHandle* wrenMakeHandle(WrenVM* vm, Value value);

// Executes [source] in the context of [module].
WrenInterpretResult wrenInterpretInModule(WrenVM* vm, const char* module,
                                          const char* source);

// Compile [source] in the context of [module] and wrap in a fiber that can
// execute it.
//
// Returns NULL if a compile error occurred.
ObjClosure* wrenCompileSource(WrenVM* vm, const char* module,
                              const char* source, bool isExpression,
                              bool printErrors);

// Looks up a variable from a previously-loaded module.
//
// Aborts the current fiber if the module or variable could not be found.
Value wrenGetModuleVariable(WrenVM* vm, Value moduleName, Value variableName);

// Returns the value of the module-level variable named [name] in the main
// module.
Value wrenFindVariable(WrenVM* vm, ObjModule* module, const char* name);

// Adds a new implicitly declared top-level variable named [name] to [module]
// based on a use site occurring on [line].
//
// Does not check to see if a variable with that name is already declared or
// defined. Returns the symbol for the new variable or -2 if there are too many
// variables defined.
int wrenDeclareVariable(WrenVM* vm, ObjModule* module, const char* name,
                        size_t length, int line);

// Adds a new top-level variable named [name] to [module].
//
// Returns the symbol for the new variable, -1 if a variable with the given name
// is already defined, or -2 if there are too many variables defined.
int wrenDefineVariable(WrenVM* vm, ObjModule* module, const char* name,
                       size_t length, Value value);

// Pushes [closure] onto [fiber]'s callstack to invoke it. Expects [numArgs]
// arguments (including the receiver) to be on the top of the stack already.
static inline void wrenCallFunction(ObjFiber* fiber,
                                    ObjClosure* closure, WrenSlot numArgs)
{
  // Grow the call frame array if needed.
  if (fiber->numFrames + 1 > fiber->frameCapacity)
  {
    int max = fiber->frameCapacity * 2;
    fiber->frames = (CallFrame*)
        canary_vm_realloc(fiber->vm, fiber->frames, sizeof(CallFrame) * max);
    fiber->frameCapacity = max;
  }
  
  // Grow the stack if needed.
  size_t stackSize = canary_thread_get_stack_size(fiber);
  size_t needed = stackSize + closure->fn->maxSlots;
  canary_thread_ensure_stack_capacity(fiber, needed);
  
  wrenAppendCallFrame(fiber, closure, fiber->stackTop - numArgs);
}

// Marks [obj] as a GC root so that it doesn't get collected.
void wrenPushRoot(WrenVM* vm, Obj* obj);

// Removes the most recently pushed temporary root.
void wrenPopRoot(WrenVM* vm);

// Returns the class of [value].
//
// Defined here instead of in wren_value.h because it's critical that this be
// inlined. That means it must be defined in the header, but the wren_value.h
// header doesn't have a full definitely of WrenVM yet.
static inline ObjClass* wrenGetClassInline(WrenVM* vm, Value value)
{
#if WREN_NAN_TAGGING
  // Don't use canary_value_get_type here for performance reasons.
  if (IS_NUM(value)) return vm->numClass;
  if (IS_OBJ(value)) return AS_OBJ(value)->classObj;

  if (canary_value_nantagging_is_singleton(value)) {
    switch (canary_value_nantagging_get_singleton_tag(value)) {
      case CANARY_VALUE_SINGLETON_FALSE_TAG:     return vm->boolClass;
      case CANARY_VALUE_SINGLETON_NULL_TAG:      return vm->nullClass;
      case CANARY_VALUE_SINGLETON_TRUE_TAG:      return vm->boolClass;
      case CANARY_VALUE_SINGLETON_UNDEFINED_TAG: UNREACHABLE(); break;
    }
  }
#else
  switch (canary_value_get_type(value))
  {
    case CANARY_TYPE_BOOL_FALSE: return vm->boolClass;
    case CANARY_TYPE_NULL:       return vm->nullClass;
    case CANARY_TYPE_DOUBLE:     return vm->numClass;
    case CANARY_TYPE_BOOL_TRUE:  return vm->boolClass;
    case VAL_OBJ:                return AS_OBJ(value)->classObj;
    case CANARY_TYPE_UNDEFINED:  UNREACHABLE();
    
    default:                     UNREACHABLE();
  }
#endif

  UNREACHABLE();
  return NULL;
}

#endif
