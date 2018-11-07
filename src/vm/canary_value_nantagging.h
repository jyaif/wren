
#ifndef CANARY_VALUE_NANTAGGING_H
#define CANARY_VALUE_NANTAGGING_H

#ifndef CANARY_VALUE_H
#error "Do not include directly. #include \"canary_value.h\" instead."
#endif // CANARY_VALUE_H

#include "canary_double.h"

// An IEEE 754 double-precision float is a 64-bit value with bits laid out like:
//
// 1 Sign bit
// | 11 Exponent bits
// | |          52 Mantissa (i.e. fraction) bits
// | |          |
// S[Exponent-][Mantissa------------------------------------------]
//
// The details of how these are used to represent numbers aren't really
// relevant here as long we don't interfere with them. The important bit is NaN.
//
// An IEEE double can represent a few magical values like NaN ("not a number"),
// Infinity, and -Infinity. A NaN is any value where all exponent bits are set:
//
//  v--NaN bits
// -11111111111----------------------------------------------------
//
// Here, "-" means "doesn't matter". Any bit sequence that matches the above is
// a NaN. With all of those "-", it obvious there are a *lot* of different
// bit patterns that all mean the same thing. NaN tagging takes advantage of
// this. We'll use those available bit patterns to represent things other than
// numbers without giving up any valid numeric values.
//
// NaN values come in two flavors: "signalling" and "quiet". The former are
// intended to halt execution, while the latter just flow through arithmetic
// operations silently. We want the latter. Quiet NaNs are indicated by setting
// the highest mantissa bit:
//
//             v--Highest mantissa bit
// -[NaN      ]1---------------------------------------------------
//
// If all of the NaN bits are set, it's not a number. Otherwise, it is.
// That leaves all of the remaining bits as available for us to play with. We
// stuff a few different kinds of things here: special singleton values like
// "true", "false", and "null", and pointers to objects allocated on the heap.
// We'll use the sign bit to distinguish singleton values from pointers. If
// it's set, it's a pointer.
//
// v--Pointer or singleton?
// S[NaN      ]1---------------------------------------------------
//
// For singleton values, we just enumerate the different values. We'll use the
// low bits of the mantissa for that, and only need a few:
//
//                                                 3 Type bits--v
// 0[NaN      ]1------------------------------------------------[T]
//
// For pointers, we are left with 51 bits of mantissa to store an address.
// That's more than enough room for a 32-bit address. Even 64-bit machines
// only actually use 48 bits for addresses, so we've got plenty. We just stuff
// the address right into the mantissa.
//
// Ta-da, double precision numbers, pointers, and a bunch of singleton values,
// all stuffed into a single 64-bit sequence. Even better, we don't have to
// do any masking or work to extract number values: they are unmodified. This
// means math on numbers is fast.

#define canary_value_impl_t canary_value_nantagging_t
typedef struct {
  uint64_t bits;
} canary_value_nantagging_t;

// A mask that selects the sign bit.
#define SIGN_BIT ((uint64_t)1 << 63)

// The bits that must be set to indicate a quiet NaN.
#define QNAN ((uint64_t)0x7ffc000000000000)

// Masks out the tag bits used to identify the singleton value.
#define MASK_TAG (7)

// Tag values for the different singleton values.
#define TAG_NAN       (0)
#define TAG_NULL      (1)
#define TAG_FALSE     (2)
#define TAG_TRUE      (3)
#define TAG_UNDEFINED (4)
#define TAG_UNUSED2   (5)
#define TAG_UNUSED3   (6)
#define TAG_UNUSED4   (7)

// Gets the singleton type tag for a Value (which must be a singleton).
#define GET_TAG(value) ((int)(canary_value_nantagging_to_bits(value) &         \
                              MASK_TAG))

static inline canary_value_nantagging_t
canary_value_nantagging_from_bits(uint64_t bits) {
  return (canary_value_nantagging_t) { bits };
}

static inline uint64_t
canary_value_nantagging_to_bits(canary_value_nantagging_t value) {
  return value.bits;
}

#define canary_value_impl_is canary_value_nantagging_is
static inline bool
canary_value_nantagging_is(canary_value_nantagging_t value,
                           canary_value_nantagging_t other) {
  // Value types have unique bit representations and we compare object types
  // by identity (i.e. pointer), so all we need to do is compare the bits.
  return canary_value_nantagging_to_bits(value) ==
         canary_value_nantagging_to_bits(other);
}

#define canary_value_impl_singleton canary_value_nantagging_singleton
static inline canary_value_nantagging_t
canary_value_nantagging_singleton(canary_valuetype_t type) {
  int tag = 0;
  switch (type) {
    case VAL_FALSE:     tag = TAG_FALSE;     break;
    case VAL_NULL:      tag = TAG_NULL;      break;
    case VAL_TRUE:      tag = TAG_TRUE;      break;
    case VAL_UNDEFINED: tag = TAG_UNDEFINED; break;
    default:            UNREACHABLE();
  }
  return canary_value_nantagging_from_bits((uint64_t)(QNAN | tag));
}

#define CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(name, type)                  \
  static inline canary_value_nantagging_t                                      \
  canary_value_nantagging_##name() {                                           \
    return canary_value_nantagging_singleton(type);                            \
  }                                                                            \
                                                                               \
  static inline bool                                                           \
  canary_value_nantagging_is_##name(canary_value_nantagging_t value) {         \
    return canary_value_nantagging_is(value, canary_value_nantagging_##name());\
  }

#define canary_value_impl_is_false canary_value_nantagging_is_false
#define canary_value_impl_false canary_value_nantagging_false
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(false, VAL_FALSE)

#define canary_value_impl_is_null canary_value_nantagging_is_null
#define canary_value_impl_null canary_value_nantagging_null
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(null, VAL_NULL)

#define canary_value_impl_is_true canary_value_nantagging_is_true
#define canary_value_impl_true canary_value_nantagging_true
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(true, VAL_TRUE)

#define canary_value_impl_is_undefined canary_value_nantagging_is_undefined
#define canary_value_impl_undefined canary_value_nantagging_undefined
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(undefined, VAL_UNDEFINED)

#define canary_value_impl_is_bool canary_value_nantagging_is_bool
static inline bool
canary_value_nantagging_is_bool(canary_value_nantagging_t value) {
  return canary_value_nantagging_is(value, canary_value_nantagging_true()) ||
         canary_value_nantagging_is(value, canary_value_nantagging_false());
}

#define canary_value_impl_to_bool canary_value_nantagging_to_bool
static inline bool
canary_value_nantagging_to_bool(canary_value_nantagging_t value) {
  return canary_value_nantagging_is(value, canary_value_nantagging_true());
}

#define canary_value_impl_from_bool canary_value_nantagging_from_bool
static inline canary_value_nantagging_t
canary_value_nantagging_from_bool(bool bvalue) {
  return bvalue ? canary_value_nantagging_true() :
                  canary_value_nantagging_false();
}

#define canary_value_impl_is_double canary_value_nantagging_is_double
static inline bool
canary_value_nantagging_is_double(canary_value_nantagging_t value) {
  // If the NaN bits are set, it's not a number.
  return (canary_value_nantagging_to_bits(value) & QNAN) != QNAN;
}

#define canary_value_impl_to_double canary_value_nantagging_to_double
static inline double
canary_value_nantagging_to_double(canary_value_nantagging_t value) {
  return canary_double_from_bits(canary_value_nantagging_to_bits(value));
}

#define canary_value_impl_from_double canary_value_nantagging_from_double
static inline canary_value_nantagging_t
canary_value_nantagging_from_double(double value) {
  return canary_value_nantagging_from_bits(canary_double_to_bits(value));
}

#define canary_value_impl_is_user_data canary_value_nantagging_is_user_data
static inline bool
canary_value_nantagging_is_user_data(canary_value_nantagging_t value) {
  return (canary_value_nantagging_to_bits(value) & (QNAN | SIGN_BIT)) ==
         (QNAN | SIGN_BIT);
}

#define canary_value_impl_to_user_data canary_value_nantagging_to_user_data
static inline void *
canary_value_nantagging_to_user_data(canary_value_nantagging_t value) {
  return (void *)(uintptr_t)(canary_value_nantagging_to_bits(value) &
                             ~(SIGN_BIT | QNAN));
}

#define canary_value_impl_from_user_data canary_value_nantagging_from_user_data
static inline canary_value_nantagging_t
canary_value_nantagging_from_user_data(void *pvalue) {
  // The triple casting is necessary here to satisfy some compilers:
  // 1. (uintptr_t) Convert the pointer to a number of the right size.
  // 2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
  // 3. Or in the bits to make a tagged Nan.
  // 4. Cast to a typedef'd value.
  return canary_value_nantagging_from_bits(
      (SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(pvalue)));
}

#define canary_value_impl_get_type canary_value_nantagging_get_type
static inline canary_valuetype_t
canary_value_nantagging_get_type(canary_value_nantagging_t value) {
  if (canary_value_nantagging_is_double(value)) return VAL_NUM;
  if (canary_value_nantagging_is_user_data(value)) return VAL_OBJ;

  switch (GET_TAG(value))
  {
    case TAG_FALSE:     return VAL_FALSE;
    case TAG_NAN:       return VAL_NUM;
    case TAG_NULL:      return VAL_NULL;
    case TAG_TRUE:      return VAL_TRUE;
    case TAG_UNDEFINED: return VAL_UNDEFINED;
  }
  UNREACHABLE();
  return VAL_UNDEFINED;
}

#include "canary_value_generic.h"

#endif // CANARY_VALUE_NANTAGGING_H
