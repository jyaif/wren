
#ifndef CANARY_VALUE_UNIONTAGGING_H
#define CANARY_VALUE_UNIONTAGGING_H

#ifndef CANARY_VALUE_H
#error "Do not include directly. #include \"canary_value.h\" instead."
#endif // CANARY_VALUE_H

#define canary_value_impl_t canary_value_uniontagging_t
typedef struct
{
  canary_type_t type;
  union
  {
    double num;
    void *user_data;
  } as;
} canary_value_uniontagging_t;

#define canary_value_impl_is canary_value_uniontagging_is
static inline bool
canary_value_uniontagging_is(canary_value_uniontagging_t value,
                             canary_value_uniontagging_t other) {
  if (value.type != other.type) return false;
  if (value.type == CANARY_TYPE_DOUBLE) return value.as.num == other.as.num;
  return value.as.user_data == other.as.user_data;
}

#define canary_value_impl_get_type canary_value_uniontagging_get_type
static inline canary_type_t
canary_value_uniontagging_get_type(canary_value_uniontagging_t value) {
  return value.type;
}

#define CANARY_DECLARE_VALUE_UNIONTAGGING_SINGLETON(name, type)                \
  static inline canary_value_uniontagging_t                                    \
  canary_value_uniontagging_##name() {                                         \
    return (canary_value_uniontagging_t){ type, { } };                         \
  }                                                                            \
                                                                               \
  static inline bool                                                           \
  canary_value_uniontagging_is_##name(canary_value_uniontagging_t value) {     \
    return canary_value_uniontagging_is(value, canary_value_uniontagging_##name());\
  }

#define canary_value_impl_is_false canary_value_uniontagging_is_false
#define canary_value_impl_false canary_value_uniontagging_false
CANARY_DECLARE_VALUE_UNIONTAGGING_SINGLETON(false, CANARY_TYPE_BOOL_FALSE)

#define canary_value_impl_is_null canary_value_uniontagging_is_null
#define canary_value_impl_null canary_value_uniontagging_null
CANARY_DECLARE_VALUE_UNIONTAGGING_SINGLETON(null, CANARY_TYPE_NULL)

#define canary_value_impl_is_true canary_value_uniontagging_is_true
#define canary_value_impl_true canary_value_uniontagging_true
CANARY_DECLARE_VALUE_UNIONTAGGING_SINGLETON(true, CANARY_TYPE_BOOL_TRUE)

#define canary_value_impl_is_undefined canary_value_uniontagging_is_undefined
#define canary_value_impl_undefined canary_value_uniontagging_undefined
CANARY_DECLARE_VALUE_UNIONTAGGING_SINGLETON(undefined, CANARY_TYPE_UNDEFINED)

#define canary_value_impl_to_double canary_value_uniontagging_to_double
static inline double
canary_value_uniontagging_to_double(canary_value_uniontagging_t value) {
    return value.as.num;
}

#define canary_value_impl_from_double canary_value_uniontagging_from_double
static inline canary_value_uniontagging_t
canary_value_uniontagging_from_double(double dvalue) {
    canary_value_uniontagging_t value = { CANARY_TYPE_DOUBLE, {  } };
    
    value.as.num = dvalue;
    return value;
}

#define canary_value_impl_to_user_data canary_value_uniontagging_to_user_data
static inline void *
canary_value_uniontagging_to_user_data(canary_value_uniontagging_t value) {
  return value.as.user_data;
}

#define canary_value_impl_from_user_data                                       \
    canary_value_uniontagging_from_user_data
static inline canary_value_uniontagging_t
canary_value_uniontagging_from_user_data(void *pvalue) {
  canary_value_uniontagging_t value = { VAL_OBJ, {  } };
  
  value.as.user_data = pvalue;
  return value;
}

#include "canary_value_generic.h"

#endif // CANARY_VALUE_UNIONTAGGING_H
