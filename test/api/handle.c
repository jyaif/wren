
#include "handle.h"

#include <string.h>

static WrenHandle* handle;

static void setValue(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  handle = wrenGetSlotHandle(vm, 1);
}

static void getValue(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenSetSlotHandle(vm, 0, handle);
  wrenReleaseHandle(vm, handle);
}

WrenForeignMethodFn handleBindMethod(const char* signature)
{
  if (strcmp(signature, "static Handle.value=(_)") == 0) return setValue;
  if (strcmp(signature, "static Handle.value") == 0) return getValue;

  return NULL;
}
