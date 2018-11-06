
#ifndef CANARY_VALUE_H
#define CANARY_VALUE_H

typedef enum
{
  VAL_FALSE,
  VAL_NULL,
  VAL_NUM,
  VAL_TRUE,
  VAL_UNDEFINED,
  VAL_OBJ
} canary_valuetype_t;

#if WREN_NAN_TAGGING
#include "canary_value_nantagging.h"
#else
#include "canary_value_uniontagging.h"
#endif

typedef canary_value_impl_t canary_value_t;

#endif // CANARY_VALUE_H
