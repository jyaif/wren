
#ifndef CANARY_VALUE_NANTAGGING_H
#define CANARY_VALUE_NANTAGGING_H

#ifndef CANARY_VALUE_H
#error "Do not include directly. #include \"canary_value.h\" instead."
#endif // CANARY_VALUE_H

#include "canary_double.h"

#include <inttypes.h>

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
// a NaN. With all of those "-", its obvious there are a *lot* of different
// bit patterns that all mean the same thing. NaN tagging takes advantage of
// this. We'll use those available bit patterns to represent things other than
// numbers without giving up any valid numeric values.
//
// NaN values come in two flavors: "signalling" and "quiet". The former are
// intended to halt execution, while the latter just flow through arithmetic
// operations silently.
//
// Since non valid values will be encoded in them the signalling ones will be
// used. The fact that it halts execution enforce the bugs, if arithmetic is
// attempted on them.
//
// NOTE: Quiet ones could be used here instead, but they suffer of the same
//       restrictions, since some values are reserved by the cpu manufacturers
//       (See CANARY_DOUBLE_INTEL_X86_QNAN_FLOATING_POINT_INDEFINITE_BITS).
//
// Signalling NaNs are indicated by clearing the highest mantissa bit:
//
//             v--Highest mantissa bit
// -[NaN      ]0---------------------------------------------------
//
// and the rest of the mantissa must be not null else it does represent an
// infinite number.
//
// That leaves all of the remaining mantissa values to encode our data.
//
// For now the sign bit is reserved for future use.
//
// To discriminate different values we will use 2 bits on top of the mantissa.
//
//               v--Type of encoding
// 0[NaN      ]1tt[encoded value                                  ]
//
// The different type of encodings are the following:
// - 0: Encode a singleton, encoded values MUST be greater than 0 since it is
//      reserved by the ieee754 to encode infinite numbers.
// - 1: Reserved for future use.
// - 2: Reserved for future use.
// - 3: Encode a user data pointer.
//
// NOTE: There are 49 bits are left to encode a pointer address.
//       It should be enought room to fit a pointer even for 64-bit CPU,
//       since currently only uses 48 bits addresses.
//
// Ta-da, double precision numbers, and a bunch of encoded values and pointers,
// all stuffed into a single 64-bit sequence. Even better, we don't have to
// do any masking or work to extract number values: they are unmodified. This
// means math on numbers is fast.

#define canary_value_impl_t canary_value_nantagging_t
typedef canary_double_bits_t canary_value_nantagging_t;
typedef uint64_t canary_value_nantagging_bits_t;

typedef uint64_t canary_value_nantagging_singleton_tag_t;

#define CANARY_VALUE_TYPE_MASK                    (UINT64_C(0xfff6000000000000))
#define CANARY_VALUE_TYPE_SINGLETON_BITS          (UINT64_C(0x7ff0000000000000))
#define CANARY_VALUE_TYPE_USER_DATA_BITS          (UINT64_C(0x7ff6000000000000))

#define CANARY_VALUE_TYPE_MIN_BITS                (UINT64_C(0x7ff0000000000001))
#define CANARY_VALUE_TYPE_MAX_BITS                (UINT64_C(0x7ff7ffffffffffff))

#define CANARY_VALUE_SINGLETON_MIN_BITS           (UINT64_C(0x7ff0000000000001))
#define CANARY_VALUE_SINGLETON_MAX_BITS           (UINT64_C(0x7ff7ffffffffffff))

// Invalid tag 0 conflicts with CANARY_DOUBLE_INF_BITS
#define CANARY_VALUE_SINGLETON_NULL_TAG           (UINT64_C(0x0000000000000001))
#define CANARY_VALUE_SINGLETON_FALSE_TAG          (UINT64_C(0x0000000000000002))
#define CANARY_VALUE_SINGLETON_TRUE_TAG           (UINT64_C(0x0000000000000003))
#define CANARY_VALUE_SINGLETON_UNDEFINED_TAG      (UINT64_C(0x0000000000000004))

static inline canary_value_nantagging_t
canary_value_nantagging_from_bits(canary_value_nantagging_bits_t bits) {
  return (canary_value_nantagging_t) { bits };
}

static inline canary_value_nantagging_bits_t
canary_value_nantagging_to_bits(canary_value_nantagging_t value) {
  return value.bits64;
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

static inline bool
canary_value_nantagging_is_singleton(canary_value_nantagging_t value) {
  canary_value_nantagging_bits_t bits = canary_value_nantagging_to_bits(value);
  
  return bits >= CANARY_VALUE_SINGLETON_MIN_BITS &&
         bits <= CANARY_VALUE_SINGLETON_MAX_BITS;
}

static inline canary_value_nantagging_singleton_tag_t
canary_value_nantagging_get_singleton_tag(canary_value_nantagging_t value) {
  canary_value_nantagging_bits_t bits = canary_value_nantagging_to_bits(value);
  CANARY_ASSERT(canary_value_nantagging_is_singleton(value),
                "Value does not hold a 'singleton'.");
  
  return bits & ~CANARY_VALUE_TYPE_MASK;
}

static inline canary_value_nantagging_t
canary_value_nantagging_singleton(canary_value_nantagging_singleton_tag_t tag) {
  canary_value_nantagging_t value =
      canary_value_nantagging_from_bits(CANARY_VALUE_TYPE_SINGLETON_BITS | tag);
  CANARY_ASSERT(canary_value_nantagging_get_singleton_tag(value) == tag,
      "Value conversion failed for 'singleton' with tag '0x" PRIx64 "'.", tag);
  
  return value;
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
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(false,
                                          CANARY_VALUE_SINGLETON_FALSE_TAG)

#define canary_value_impl_is_null canary_value_nantagging_is_null
#define canary_value_impl_null canary_value_nantagging_null
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(null,
                                          CANARY_VALUE_SINGLETON_NULL_TAG)

#define canary_value_impl_is_true canary_value_nantagging_is_true
#define canary_value_impl_true canary_value_nantagging_true
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(true,
                                          CANARY_VALUE_SINGLETON_TRUE_TAG)

#define canary_value_impl_is_undefined canary_value_nantagging_is_undefined
#define canary_value_impl_undefined canary_value_nantagging_undefined
CANARY_DECLARE_VALUE_NANTAGGING_SINGLETON(undefined,
                                          CANARY_VALUE_SINGLETON_UNDEFINED_TAG)

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
  return !canary_double_is_snan(value.num);
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
  canary_value_nantagging_bits_t bits = canary_value_nantagging_to_bits(value);
  
  return (bits & CANARY_VALUE_TYPE_MASK) == (CANARY_VALUE_TYPE_USER_DATA_BITS);
}

#define canary_value_impl_to_user_data canary_value_nantagging_to_user_data
static inline void *
canary_value_nantagging_to_user_data(canary_value_nantagging_t value) {
  canary_value_nantagging_bits_t bits = canary_value_nantagging_to_bits(value);
  
  return (void *)(uintptr_t)(bits & ~CANARY_VALUE_TYPE_MASK);
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
      CANARY_VALUE_TYPE_USER_DATA_BITS | (uint64_t)(uintptr_t)(pvalue));
}

#define canary_value_impl_get_type canary_value_nantagging_get_type
static inline canary_type_t
canary_value_nantagging_get_type(canary_value_nantagging_t value) {
  if (canary_value_nantagging_is_double(value)) return CANARY_TYPE_DOUBLE;
  if (canary_value_nantagging_is_user_data(value)) return VAL_OBJ;

  if (canary_value_nantagging_is_singleton(value)) {
    switch (canary_value_nantagging_get_singleton_tag(value)) {
      case CANARY_VALUE_SINGLETON_FALSE_TAG:     return CANARY_TYPE_BOOL_FALSE;
      case CANARY_VALUE_SINGLETON_NULL_TAG:      return CANARY_TYPE_NULL;
      case CANARY_VALUE_SINGLETON_TRUE_TAG:      return CANARY_TYPE_BOOL_TRUE;
      case CANARY_VALUE_SINGLETON_UNDEFINED_TAG: return CANARY_TYPE_UNDEFINED;
    }
  }
  CANARY_UNREACHABLE();
  return CANARY_TYPE_UNDEFINED;
}

#include "canary_value_generic.h"

#endif // CANARY_VALUE_NANTAGGING_H
