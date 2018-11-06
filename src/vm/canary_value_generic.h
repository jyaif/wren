
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

#endif // CANARY_VALUE_GENERIC_H
