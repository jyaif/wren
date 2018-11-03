
#include "reset_stack_after_call_abort.h"

void resetStackAfterCallAbortRunTests(WrenVM* vm)
{
  wrenSetSlotCount(vm, 1);
  wrenGetVariable(vm, 0, "./test/api/reset_stack_after_call_abort", "Test");
  WrenHandle* testClass = wrenGetSlotHandle(vm, 0);
  
  WrenHandle* abortFiber = wrenMakeCallHandle(vm, "abortFiber()");
  WrenHandle* afterConstruct = wrenMakeCallHandle(vm, "afterAbort(_,_)");
  
  wrenSetSlotCount(vm, 1);
  wrenSetSlotHandle(vm, 0, testClass);
  wrenCall(vm, abortFiber);

  wrenSetSlotCount(vm, 3);
  wrenSetSlotHandle(vm, 0, testClass);
  wrenSetSlotDouble(vm, 1, 1.0);
  wrenSetSlotDouble(vm, 2, 2.0);
  wrenCall(vm, afterConstruct);
  
  wrenReleaseHandle(vm, testClass);
  wrenReleaseHandle(vm, abortFiber);
  wrenReleaseHandle(vm, afterConstruct);
}
