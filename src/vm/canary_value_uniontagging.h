
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

#define IS_FALSE(value)     ((value).type == VAL_FALSE)
#define IS_NULL(value)      ((value).type == VAL_NULL)
#define IS_NUM(value)       ((value).type == VAL_NUM)
#define IS_UNDEFINED(value) ((value).type == VAL_UNDEFINED)

// Singleton values.
#define FALSE_VAL     ((Value){ VAL_FALSE, { 0 } })
#define NULL_VAL      ((Value){ VAL_NULL, { 0 } })
#define TRUE_VAL      ((Value){ VAL_TRUE, { 0 } })
#define UNDEFINED_VAL ((Value){ VAL_UNDEFINED, { 0 } })

#include "canary_value_generic.h"

#endif // CANARY_VALUE_UNIONTAGGING_H
