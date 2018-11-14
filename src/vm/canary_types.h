
#ifndef CANARY_TYPES_H
#define CANARY_TYPES_H

#include "canary_global.h"

#include <string.h>

typedef struct sObjFiber canary_thread_t;
typedef struct WrenVM canary_vm_t;

#define CANARY_DEFINE_TRIVIALLY_COMPARABLE_TYPE(name, type)                    \
  static inline bool                                                           \
  canary_##name##_equals(type self, type other) {                              \
    return self == other;                                                      \
  }                                                                            \
                                                                               \
  static inline bool                                                           \
  canary_##name##_is(type self, type other) {                                  \
    return memcmp(&self, &other, sizeof(type)) == 0;                           \
  }

#define CANARY_DECLARE_TRIVIALLY_COMPARABLE_TYPE(name, type)                   \
  /* Nothing to do here */

CANARY_DEFINE_TRIVIALLY_COMPARABLE_TYPE(bool, bool)
CANARY_DEFINE_TRIVIALLY_COMPARABLE_TYPE(double, double)

#endif // CANARY_TYPES_H
