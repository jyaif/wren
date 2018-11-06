
#ifndef CANARY_VALUE_UNIONTAGGING_H
#define CANARY_VALUE_UNIONTAGGING_H

#ifndef CANARY_VALUE_H
#error "Do not include directly. #include \"canary_value.h\" instead."
#endif // CANARY_VALUE_H

#define canary_value_impl_t canary_value_uniontagging_t
typedef struct
{
  canary_valuetype_t type;
  union
  {
    double num;
    Obj* obj;
  } as;
} canary_value_uniontagging_t;

// Value -> 0 or 1.
#define AS_BOOL(value) ((value).type == VAL_TRUE)

// Value -> Obj*.
#define AS_OBJ(v) ((v).as.obj)

// Determines if [value] is a garbage-collected object or not.
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define IS_NUM(value)       ((value).type == VAL_NUM)

#define canary_value_impl_is canary_value_uniontagging_is
static inline bool
canary_value_uniontagging_is(canary_value_uniontagging_t value,
                             canary_value_uniontagging_t other) {
  if (value.type != other.type) return false;
  if (value.type == VAL_NUM) return value.as.num == other.as.num;
  return value.as.obj == other.as.obj;
}

#define canary_value_impl_get_type canary_value_uniontagging_get_type
static inline canary_valuetype_t
canary_value_uniontagging_get_type(canary_value_uniontagging_t value) {
  return value.type;
}

#define canary_value_impl_singleton canary_value_uniontagging_singleton
static inline canary_value_uniontagging_t
canary_value_uniontagging_singleton(canary_valuetype_t type) {
    return (canary_value_uniontagging_t){ type , { } };
}

#include "canary_value_generic.h"

#endif // CANARY_VALUE_UNIONTAGGING_H
