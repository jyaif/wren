
#include "foreign_class.h"

#include "vm.h"

#include <stdio.h>
#include <string.h>

static int finalized = 0;

static void apiFinalized(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  wrenSetSlotDouble(vm, 0, finalized);
}

static void counterAllocate(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double* value = (double*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(double));
  *value = 0;
}

static void counterIncrement(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double* value = (double*)wrenGetSlotForeign(vm, 0);
  double increment = wrenGetSlotDouble(vm, 1);

  *value += increment;
}

static void counterValue(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double value = *(double*)wrenGetSlotForeign(vm, 0);
  wrenSetSlotDouble(vm, 0, value);
}

static void pointAllocate(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double* coordinates = (double*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(double[3]));

  // This gets called by both constructors, so sniff the slot count to see
  // which one was invoked.
  if (wrenGetSlotCount(vm) == 1)
  {
    coordinates[0] = 0.0;
    coordinates[1] = 0.0;
    coordinates[2] = 0.0;
  }
  else
  {
    coordinates[0] = wrenGetSlotDouble(vm, 1);
    coordinates[1] = wrenGetSlotDouble(vm, 2);
    coordinates[2] = wrenGetSlotDouble(vm, 3);
  }
}

static void pointTranslate(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double* coordinates = (double*)wrenGetSlotForeign(vm, 0);
  coordinates[0] += wrenGetSlotDouble(vm, 1);
  coordinates[1] += wrenGetSlotDouble(vm, 2);
  coordinates[2] += wrenGetSlotDouble(vm, 3);
}

static void pointToString(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  double* coordinates = (double*)wrenGetSlotForeign(vm, 0);
  char result[100];
  sprintf(result, "(%g, %g, %g)",
      coordinates[0], coordinates[1], coordinates[2]);
  wrenSetSlotString(vm, 0, result);
}

static void resourceAllocate(canary_context_t *context)
{
  WrenVM *vm = wrenVMFromContext(context);
  int* value = (int*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
  *value = 123;
}

static void resourceFinalize(void* data)
{
  // Make sure we get the right data back.
  int* value = (int*)data;
  if (*value != 123) setExitCode(1);
  
  finalized++;
}

WrenForeignMethodFn foreignClassBindMethod(const char* signature)
{
  if (strcmp(signature, "static ForeignClass.finalized") == 0) return apiFinalized;
  if (strcmp(signature, "Counter.increment(_)") == 0) return counterIncrement;
  if (strcmp(signature, "Counter.value") == 0) return counterValue;
  if (strcmp(signature, "Point.translate(_,_,_)") == 0) return pointTranslate;
  if (strcmp(signature, "Point.toString") == 0) return pointToString;

  return NULL;
}

void foreignClassBindClass(
    const char* className, WrenForeignClassMethods* methods)
{
  if (strcmp(className, "Counter") == 0)
  {
    methods->allocate = counterAllocate;
    return;
  }

  if (strcmp(className, "Point") == 0)
  {
    methods->allocate = pointAllocate;
    return;
  }

  if (strcmp(className, "Resource") == 0)
  {
    methods->allocate = resourceAllocate;
    methods->finalize = resourceFinalize;
    return;
  }
}
