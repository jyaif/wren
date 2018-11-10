
#ifndef CANARY_GLOBAL_H
#define CANARY_GLOBAL_H

#include "canary.h"

#include "canary_config.h"

#define CANARY_VERSION_PREREQ(version, expected) ((version) >= (expected))

#define CANARY_C99_VERSION 199901L
#define CANARY_C11_VERSION 201112L

#define CANARY_C_VERSION_PREREQ(expected)                                      \
  CANARY_VERSION_PREREQ(__STDC_VERSION__, (expected))

#define CANARY_CX11_VERSION 201103L
#define CANARY_CX14_VERSION 201402L
#define CANARY_CX17_VERSION 201703L

#define CANARY_CXX_VERSION_PREREQ(expected)                                    \
  CANARY_VERSION_PREREQ(__cplusplus, (expected))

#include "canary_global_gnuc.h"
#include "canary_global_msc.h"

#if !defined(canary_likely)
#define canary_likely(condition)   (condition)
#define canary_unlikely(condition) (condition)
#endif

#if !defined(canary_noreturn)
#if CANARY_CXX_VERSION_PREREQ(CANARY_CX11_VERSION)
#define canary_noreturn [[noreturn]]
#elif CANARY_C_VERSION_PREREQ(CANARY_C11_VERSION)
#include <stdnoreturn.h>
#define canary_noreturn _Noreturn
#else
#define canary_noreturn
#endif
#endif

#if !defined(canary_unreachable)
#define canary_unreachable() do { } while (false)
#endif

// Assertions are used to validate program invariants. They indicate things the
// program expects to be true about its internal state during execution. If an
// assertion fails, there is a bug in Wren.
//
// Assertions add significant overhead, so are only enabled in debug builds.
#ifdef CANARY_DEBUG

#define CANARY_ASSERT(condition, ...)                                          \
  do {                                                                         \
    if (canary_unlikely((condition) == false)) {                               \
      canary_abort(__VA_ARGS__);                                               \
    }                                                                          \
  } while (false)

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define CANARY_UNREACHABLE()                                                   \
  do {                                                                         \
      CANARY_ASSERT(false,                                                     \
                    "[%s:%d] This code should not be reached in %s()\n",       \
                           __FILE__, __LINE__, __func__);                      \
      canary_unreachable();                                                    \
    } while (false)

#else // CANARY_DEBUG

#define CANARY_ASSERT(condition, ...) do { } while (false)

// Tell the compiler that this part of the code will never be reached.
#if !defined(CANARY_UNREACHABLE)
#define CANARY_UNREACHABLE() canary_unreachable()
#endif

#endif // CANARY_DEBUG

canary_noreturn void
canary_abort(const char *reason, ...);

#endif // CANARY_GLOBAL_H
