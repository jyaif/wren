
#include "get_variable.h"

#include <string.h>

static void beforeDefined(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenGetVariable(vm, 0, "./test/api/get_variable", "A");
}

static void afterDefined(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenGetVariable(vm, 0, "./test/api/get_variable", "A");
}

static void afterAssigned(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenGetVariable(vm, 0, "./test/api/get_variable", "A");
}

static void otherSlot(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenSetSlotCount(vm, 3);
  wrenGetVariable(vm, 2, "./test/api/get_variable", "B");
  
  // Move it into return position.
  const char* string = wrenGetSlotString(vm, 2);
  wrenSetSlotString(vm, 0, string);
}

static void otherModule(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenGetVariable(vm, 0, "./test/api/get_variable_module", "Variable");
}

WrenForeignMethodFn getVariableBindMethod(const char* signature)
{
  if (strcmp(signature, "static GetVariable.beforeDefined()") == 0) return beforeDefined;
  if (strcmp(signature, "static GetVariable.afterDefined()") == 0) return afterDefined;
  if (strcmp(signature, "static GetVariable.afterAssigned()") == 0) return afterAssigned;
  if (strcmp(signature, "static GetVariable.otherSlot()") == 0) return otherSlot;
  if (strcmp(signature, "static GetVariable.otherModule()") == 0) return otherModule;

  return NULL;
}
