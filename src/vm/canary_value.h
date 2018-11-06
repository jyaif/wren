
#ifndef CANARY_VALUE_H
#define CANARY_VALUE_H

#include "canary_type.h"

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

static inline bool
canary_valuetype_is_singleton(canary_valuetype_t type) {
  switch (type) {
    case VAL_FALSE:
    case VAL_NULL:
    case VAL_TRUE:
    case VAL_UNDEFINED:
      return true;
    default:
      return false;
  }
  return false;
}

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

#define canary_value_is_singleton(value, type)                                 \
    canary_value_has_type((value), (type))

static inline canary_value_t
canary_value_singleton(canary_valuetype_t type) {
  ASSERT(canary_valuetype_is_singleton(type), "Invalid singleton type.");
  
  return canary_value_impl_singleton(type);
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
  ASSERT(canary_value_is_##name(value), "Value must hold a bool.");            \
                                                                               \
  return canary_value_impl_to_##name(value);                                   \
}                                                                              \
                                                                               \
static inline canary_value_t                                                   \
canary_value_from_##name(type native) {                                        \
  canary_value_t value = canary_value_impl_from_##name(native);                \
  ASSERT(canary_##name##_is(canary_value_to_##name(value), native),            \
         "Value conversion failed for "#name".");                              \
                                                                               \
  return value;                                                                \
}

CANARY_DEFINE_VALUE(bool, bool)
CANARY_DEFINE_VALUE(double, double)

#endif // CANARY_VALUE_H
