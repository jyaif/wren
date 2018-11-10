
#ifndef CANARY_GLOBAL_GNUC_H
#define CANARY_GLOBAL_GNUC_H

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define CANARY_GNUC_PREREQ(major, minor, patchlevel)                           \
  CANARY_VERSION_PREREQ(                                                       \
    CANARY_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__),        \
    CANARY_MAKE_VERSION((major), (minor), (patchlevel)))
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define V8_GNUC_PREREQ(major, minor, patchlevel)                               \
  CANARY_VERSION_PREREQ(                                                       \
    CANARY_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__),        \
     CANARY_MAKE_VERSION((major), (minor), 0))
#else
#define CANARY_GNUC_PREREQ(major, minor, patchlevel) 0
#endif

#if !defined(canary_unreachable) &&                                            \
    CANARY_GNUC_PREREQ(4, 4, 5)
#define canary_unreachable() __builtin_unreachable()
#endif

#if !defined(canary_likely) &&                                                 \
    CANARY_GNUC_PREREQ(4, 4, 5) // FIXME: Search version
#define canary_likely(condition)   __builtin_expect(!!(condition), 1)
#define canary_unlikely(condition) __builtin_expect(!!(condition), 0)
#endif

#endif // CANARY_GLOBAL_GNUC_H
