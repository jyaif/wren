
#ifndef CANARY_VALUE_H
#define CANARY_VALUE_H

#include "canary_global.h"

#include "canary_types.h"

// First one in the list of OBJ
#define VAL_OBJ CANARY_TYPE_CLASS

#if CANARY_VALUE_FLAVOUR == CANARY_VALUE_FLAVOUR_NANTAGGING
#include "canary_value_nantagging.h"
#elif CANARY_VALUE_FLAVOUR == CANARY_VALUE_FLAVOUR_UNIONTAGGING
#include "canary_value_uniontagging.h"
#else // CANARY_VALUE_FLAVOUR
#error "Unupported CANARY_VALUE_FLAVOUR."
#endif // CANARY_VALUE_FLAVOUR

typedef canary_value_impl_t canary_value_t;

CANARY_DEFINE_TRIVIALLY_COMPARABLE_TYPE(user_data, void *)

// Returns true if [value] and [other] are strictly the same value.
//
// This is identity for object values, and value equality for unboxed values.
static inline bool
canary_value_is(canary_value_t value, canary_value_t other) {
  return canary_value_impl_is(value, other);
}

static inline canary_type_t
canary_value_get_type(canary_value_t value) {
  return canary_value_impl_get_type(value);
}

static inline bool
canary_value_has_type(canary_value_t value, canary_type_t type) {
  return canary_value_impl_has_type(value, type);
}

#define CANARY_DECLARE_VALUE_SINGLETON(name)                                   \
  static inline bool                                                           \
  canary_value_is_##name(canary_value_t value) {                               \
    return canary_value_impl_is_##name(value);                                 \
  }                                                                            \
                                                                               \
  static inline canary_value_t                                                 \
  canary_value_##name() {                                                      \
    return canary_value_impl_##name();                                         \
  }

CANARY_DECLARE_VALUE_SINGLETON(false)
CANARY_DECLARE_VALUE_SINGLETON(null)
CANARY_DECLARE_VALUE_SINGLETON(true)
CANARY_DECLARE_VALUE_SINGLETON(undefined)

#define CANARY_DEFINE_VALUE(name, type)                                        \
static inline bool                                                             \
canary_value_is_##name(canary_value_t value) {                                 \
  return canary_value_impl_is_##name(value);                                   \
}                                                                              \
                                                                               \
static inline type                                                             \
canary_value_to_##name(canary_value_t value) {                                 \
  CANARY_ASSERT(canary_value_is_##name(value),                                 \
                "Value does not hold '"#name"' type.");                        \
                                                                               \
  return canary_value_impl_to_##name(value);                                   \
}                                                                              \
                                                                               \
static inline canary_value_t                                                   \
canary_value_from_##name(type native) {                                        \
  canary_value_t value = canary_value_impl_from_##name(native);                \
  CANARY_ASSERT(canary_##name##_is(canary_value_to_##name(value), native),     \
                "Value conversion failed for '"#name"' type.");                \
                                                                               \
  return value;                                                                \
}

CANARY_DEFINE_VALUE(bool, bool)
CANARY_DEFINE_VALUE(double, double)
CANARY_DEFINE_VALUE(user_data, void *)

#endif // CANARY_VALUE_H
