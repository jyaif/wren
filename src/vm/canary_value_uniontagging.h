
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
    void *user_data;
  } as;
} canary_value_uniontagging_t;

#define canary_value_impl_is canary_value_uniontagging_is
static inline bool
canary_value_uniontagging_is(canary_value_uniontagging_t value,
                             canary_value_uniontagging_t other) {
  if (value.type != other.type) return false;
  if (value.type == VAL_NUM) return value.as.num == other.as.num;
  return value.as.user_data == other.as.user_data;
}

#define canary_value_impl_get_type canary_value_uniontagging_get_type
static inline canary_valuetype_t
canary_value_uniontagging_get_type(canary_value_uniontagging_t value) {
  return value.type;
}

#define canary_value_impl_singleton canary_value_uniontagging_singleton
static inline canary_value_uniontagging_t
canary_value_uniontagging_singleton(canary_valuetype_t type) {
    return (canary_value_uniontagging_t){ type, { } };
}

#define canary_value_impl_to_double canary_value_uniontagging_to_double
static inline double
canary_value_uniontagging_to_double(canary_value_uniontagging_t value) {
    return value.as.num;
}

#define canary_value_impl_from_double canary_value_uniontagging_from_double
static inline canary_value_uniontagging_t
canary_value_uniontagging_from_double(double dvalue) {
    canary_value_uniontagging_t value = { VAL_NUM, {  } };
    
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
