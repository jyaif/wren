
#ifndef CANARY_GLOBAL_MSC_H
#define CANARY_GLOBAL_MSC_H

// See https://en.wikipedia.org/wiki/Microsoft_Visual_C++ for numbering
#define CANARY_MSC_VER_VS_2013_V12_0 1800

#define CANARY_MSC_PREREQ(expected) CANARY_VERSION_PREREQ(_MSC_VER, (version))

#if defined(_MSC_VER) &&                                                       \
    (CANARY_THREAD_INTERPRETER_FLAVOR == CANARY_THREAD_INTERPRETER_FLAVOR_GOTO)
#warning "CANARY_THREAD_INTERPRETER_FLAVOR_GOTO not supported with MS compiler."
#warning "Switching to CANARY_THREAD_INTERPRETER_FLAVOR_SWITH flavor."
#undef CANARY_THREAD_INTERPRETER_FLAVOR
#define CANARY_THREAD_INTERPRETER_FLAVOR CANARY_THREAD_INTERPRETER_FLAVOR_SWITH
#endif

// The Microsoft compiler does not support the "inline" modifier when compiling
// as plain C.
#if defined(_MSC_VER) &&                                                       \
    !defined(__cplusplus)
  #define inline _inline
#endif


#if CANARY_MSC_PREREQ(CANARY_MSC_VER_VS_2013_V12_0) &&                         \
    !defined(canary_unreachable)
#define canary_unreachable() __builtin_unreachable()

#endif

#endif // CANARY_GLOBAL_MSC_H
