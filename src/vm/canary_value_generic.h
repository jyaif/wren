
#ifndef CANARY_VALUE_GENERIC_H
#define CANARY_VALUE_GENERIC_H

#ifndef CANARY_VALUE_H
#error "Do not include directly. #include \"canary_value.h\" instead."
#endif // CANARY_VALUE_H

#ifndef canary_value_impl_is
static inline bool
canary_value_impl_is(canary_value_impl_t value, canary_value_impl_t other) {
  return memcmp(&value, &other, sizeof(value));
}
#endif // canary_value_impl_is

#ifndef canary_value_impl_has_type
static inline bool
canary_value_impl_has_type(canary_value_impl_t value, canary_valuetype_t type) {
  return canary_value_impl_get_type(value) == type;
}
#endif //canary_value_impl_has_type

#define canary_value_impl_is_singleton(value, type)                            \
    canary_value_impl_has_type((value), (type))

#ifndef canary_value_impl_is_false
static inline bool
canary_value_impl_is_false(canary_value_impl_t value) {
  return canary_value_impl_is_singleton(value, VAL_FALSE);
}
#endif // canary_value_impl_is_false

#ifndef canary_value_impl_false
static inline canary_value_impl_t
canary_value_impl_false() {
  return canary_value_impl_singleton(VAL_FALSE);
}
#endif // canary_value_impl_false

#ifndef canary_value_impl_is_null
static inline bool
canary_value_impl_is_null(canary_value_impl_t value) {
  return canary_value_impl_is_singleton(value, VAL_NULL);
}
#endif // canary_value_impl_is_null

#ifndef canary_value_impl_null
static inline canary_value_impl_t
canary_value_impl_null() {
  return canary_value_impl_singleton(VAL_NULL);
}
#endif // canary_value_impl_null

#ifndef canary_value_impl_is_true
static inline bool
canary_value_impl_is_true(canary_value_impl_t value) {
  return canary_value_impl_is_singleton(value, VAL_TRUE);
}
#endif // canary_value_impl_is_true

#ifndef canary_value_impl_true
static inline canary_value_impl_t
canary_value_impl_true() {
  return canary_value_impl_singleton(VAL_TRUE);
}
#endif // canary_value_impl_true

#ifndef canary_value_impl_is_undefined
static inline bool
canary_value_impl_is_undefined(canary_value_impl_t value) {
  return canary_value_impl_is_singleton(value, VAL_UNDEFINED);
}
#endif // canary_value_impl_is_undefined

#ifndef canary_value_impl_undefined
static inline canary_value_impl_t
canary_value_impl_undefined() {
  return canary_value_impl_singleton(VAL_UNDEFINED);
}
#endif // canary_value_impl_undefined

#ifndef canary_value_impl_is_bool
static inline bool
canary_value_impl_is_bool(canary_value_impl_t value) {
  canary_valuetype_t type = canary_value_impl_get_type(value);
  
  return type == VAL_TRUE || type == VAL_FALSE;
}
#endif // canary_value_impl_is_bool

#ifndef canary_value_impl_to_bool
static inline bool
canary_value_impl_to_bool(canary_value_impl_t value) {
  return canary_value_impl_get_type(value) == VAL_TRUE;
}
#endif // canary_value_impl_to_bool

#ifndef canary_value_impl_from_bool
static inline canary_value_impl_t
canary_value_impl_from_bool(bool bvalue) {
  return canary_value_impl_singleton(bvalue ? VAL_TRUE : VAL_FALSE);
}
#endif // canary_value_impl_from_bool

#ifndef canary_value_impl_is_double

static inline bool
canary_value_impl_is_double(canary_value_impl_t value) {
  canary_valuetype_t type = canary_value_impl_get_type(value);
  
  return type == VAL_NUM;
}

#endif // canary_value_impl_is_double

#ifndef canary_value_impl_is_user_data

static inline bool
canary_value_impl_is_user_data(canary_value_impl_t value) {
  canary_valuetype_t type = canary_value_impl_get_type(value);
  
  return type == VAL_OBJ;
}

#endif // canary_value_impl_is_user_data

#endif // CANARY_VALUE_GENERIC_H
