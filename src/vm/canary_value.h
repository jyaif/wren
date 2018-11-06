
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

// Returns true if [value] and [other] are strictly the same value.
//
// This is identity for object values, and value equality for unboxed values.
static inline bool
canary_value_is(canary_value_t value, canary_value_t other) {
  return canary_value_impl_is(value, other);
}

static inline canary_valuetype_t
canary_value_get_type(canary_value_t value) {
  return canary_value_impl_get_type(value);
}

static inline bool
canary_value_has_type(canary_value_t value, canary_valuetype_t type) {
  return canary_value_impl_has_type(value, type);
}

#endif // CANARY_VALUE_H
