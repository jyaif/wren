
#include "error.h"

#include <string.h>

static void runtimeError(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenSetSlotCount(vm, 1);
  wrenSetSlotString(vm, 0, "Error!");
  wrenAbortFiber(vm, 0);
}

WrenForeignMethodFn errorBindMethod(const char* signature)
{
  if (strcmp(signature, "static Error.runtimeError") == 0) return runtimeError;

  return NULL;
}
