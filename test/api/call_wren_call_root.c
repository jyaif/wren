
#include "call_wren_call_root.h"

#include "vm.h"

#include <stdio.h>

void callWrenCallRootRunTests(WrenVM* vm)
{
  wrenEnsureSlots(vm, 1);
  wrenGetVariable(vm, 0, "./test/api/call_wren_call_root", "Test");
  WrenHandle* testClass = wrenGetSlotHandle(vm, 0);

  WrenHandle* run = wrenMakeCallHandle(vm, "run()");

  wrenEnsureSlots(vm, 1);
  wrenSetSlotHandle(vm, 0, testClass);
  WrenInterpretResult result = wrenCall(vm, run);
  if (result == WREN_RESULT_RUNTIME_ERROR)
  {
    setExitCode(70);
  }
  else
  {
    printf("Missing runtime error.\n");
  }

  wrenReleaseHandle(vm, testClass);
  wrenReleaseHandle(vm, run);
}
