
#ifndef CANARY_DOUBLE_H
#define CANARY_DOUBLE_H

#include "canary_types.h"

// Helper for IEEE 754 double precision numbers.

#define CANARY_DOUBLE_SIGN_MASK                   (UINT64_C(0x8000000000000000))
#define CANARY_DOUBLE_EXPONENT_MASK               (UINT64_C(0x7ff0000000000000))
#define CANARY_DOUBLE_FRACTION_MASK               (UINT64_C(0x000fffffffffffff))

#define CANARY_DOUBLE_SIGN_BIT                    (UINT64_C(0x8000000000000000))
#define CANARY_DOUBLE_INF_BITS                    (UINT64_C(0x7ff0000000000000))
#define CANARY_DOUBLE_SNAN_MIN_BITS               (UINT64_C(0x7ff0000000000001))
#define CANARY_DOUBLE_SNAN_MAX_BITS               (UINT64_C(0x7ff7ffffffffffff))
#define CANARY_DOUBLE_QNAN_MIN_BITS               (UINT64_C(0x7ff8000000000000))
#define CANARY_DOUBLE_QNAN_MAX_BITS               (UINT64_C(0x7fffffffffffffff))

#define CANARY_DOUBLE_NAN_MIN_BITS                 (CANARY_DOUBLE_SNAN_MIN_BITS)
#define CANARY_DOUBLE_NAN_MAX_BITS                 (CANARY_DOUBLE_QNAN_MAX_BITS)

// Some wellknown values

#define CANARY_DOUBLE_INFINITY (canary_double_from_bits(CANARY_DOUBLE_INF_BITS))

// See Intel 64 and IA-32 Architectures Software Developer's Manual Volume 1
// at chapter 4.8.3.7 / table 4.3 for QNaN Floating-Point Indefinite.
#define CANARY_DOUBLE_QNAN_FLOATING_POINT_INDEFINITE                           \
                                   (canary_double_from_bits(0xfff8000000000000))

// A union to let us reinterpret a double as raw bits and back.
typedef union
{
  uint64_t bits64;
  uint32_t bits32[2];
  double num;
} canary_double_bits_t;

typedef enum {
  CANARY_DOUBLETYPE_NORMAL,
  CANARY_DOUBLETYPE_SNAN,
  CANARY_DOUBLETYPE_QNAN,
  CANARY_DOUBLETYPE_INFINITE,
  CANARY_DOUBLETYPE_ZERO,
  CANARY_DOUBLETYPE_SUBNORMAL,
} canary_doubletype_t;

static inline double
canary_double_from_bits(uint64_t value) {
  canary_double_bits_t bits;
  
  bits.bits64 = value;
  return bits.num;
}

static inline uint64_t
canary_double_to_bits(double value) {
  canary_double_bits_t bits;
  
  bits.num = value;
  return bits.bits64;
}

static inline canary_doubletype_t
canary_double_classify(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  if ((i & CANARY_DOUBLE_EXPONENT_MASK) == CANARY_DOUBLE_EXPONENT_MASK) {
    i &= CANARY_DOUBLE_FRACTION_MASK;
    if (i == 0) {
      return CANARY_DOUBLETYPE_INFINITE;
    }
    if (i <= UINT64_C(0x0007ffffffffffff)) {
      return CANARY_DOUBLETYPE_SNAN;
    }
    return CANARY_DOUBLETYPE_QNAN;
  }
  else {
    if ((i & CANARY_DOUBLE_EXPONENT_MASK) == UINT64_C(0)) {
      i &= CANARY_DOUBLE_FRACTION_MASK;
      if (i == 0) {
        return CANARY_DOUBLETYPE_ZERO;
      }
      return CANARY_DOUBLETYPE_SUBNORMAL;
    }
    return CANARY_DOUBLETYPE_NORMAL;
  }
}

static inline bool
canary_double_is_finite(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return (i & CANARY_DOUBLE_EXPONENT_MASK) != CANARY_DOUBLE_EXPONENT_MASK;
}

static inline bool
canary_double_is_infinite(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return i == CANARY_DOUBLE_INF_BITS;
}

static inline bool
canary_double_is_nan(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return i >= CANARY_DOUBLE_NAN_MIN_BITS &&
         i <= CANARY_DOUBLE_NAN_MAX_BITS;
}

static inline bool
canary_double_is_negative(double d) {
  uint64_t i = canary_double_to_bits(d) & CANARY_DOUBLE_SIGN_MASK;
  
  return i != UINT64_C(0);
}

static inline bool
canary_double_is_normal(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return (i & CANARY_DOUBLE_EXPONENT_MASK) != CANARY_DOUBLE_EXPONENT_MASK &&
         (i & CANARY_DOUBLE_EXPONENT_MASK) != UINT64_C(0);
}

static inline bool
canary_double_is_qnan(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return i >= CANARY_DOUBLE_QNAN_MIN_BITS &&
         i <= CANARY_DOUBLE_QNAN_MAX_BITS;
}

static inline bool
canary_double_is_snan(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return i >= CANARY_DOUBLE_SNAN_MIN_BITS &&
         i <= CANARY_DOUBLE_SNAN_MAX_BITS;
}

static inline bool
canary_double_is_subnormal(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return (i & CANARY_DOUBLE_EXPONENT_MASK) == UINT64_C(0) &&
         (i & CANARY_DOUBLE_FRACTION_MASK) != UINT64_C(0);
}

static inline bool
canary_double_is_zero(double d) {
  uint64_t i = canary_double_to_bits(d) & ~CANARY_DOUBLE_SIGN_MASK;
  
  return i == UINT64_C(0);
}

#endif // CANARY_DOUBLE_H
