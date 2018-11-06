
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

#endif // CANARY_VALUE_GENERIC_H
