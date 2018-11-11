
#ifndef CANARY_H
#define CANARY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// The semantic version number components.
#define CANARY_VERSION_MAJOR 0
#define CANARY_VERSION_MINOR 0
#define CANARY_VERSION_PATCH 0

#define CANARY_STRINGIFY(str) #str

// A human-friendly string representation of the version.
#define CANARY_VERSION_STRING                                                  \
  CANARY_STRINGIFY(CANARY_VERSION_MAJOR) "."                                   \
  CANARY_STRINGIFY(CANARY_VERSION_MINOR) "."                                   \
  CANARY_STRINGIFY(CANARY_VERSION_PATCH)

#define CANARY_MAKE_VERSION(major, minor, patch)                               \
  ((major) * 10000 + (minor) * 100 + (patch))

// A monotonically increasing numeric representation of the version number. Use
// this if you want to do range checks over versions.
#define CANARY_VERSION                                                         \
  (CANARY_MAKE_VERSION(CANARY_VERSION_MAJOR,                                   \
                       CANARY_VERSION_MINOR,                                   \
                       CANARY_VERSION_PATCH))

typedef uint8_t canary_slot_t;

typedef enum {
  // The object is of a type that isn't accessible by the C API.
  CANARY_TYPE_UNKNOWN        =   0,
  
  CANARY_TYPE_UNDEFINED      =   1,
  CANARY_TYPE_NULL           =   2,
//  CANARY_TYPE_BOOL           =   3,
  CANARY_TYPE_DOUBLE         =   4,
  
  CANARY_TYPE_BOOL_FALSE     =  16,
  CANARY_TYPE_BOOL_TRUE      =  17,
  
  CANARY_TYPE_CLASS          =  32,
  CANARY_TYPE_CLOSURE        =  33,
  CANARY_TYPE_FIBER          =  34,
  CANARY_TYPE_FN             =  35,
  CANARY_TYPE_FOREIGN        =  36,
  CANARY_TYPE_INSTANCE       =  37,
  CANARY_TYPE_LIST           =  38,
  CANARY_TYPE_MAP            =  39,
  CANARY_TYPE_MODULE         =  40,
  CANARY_TYPE_RANGE          =  41,
  CANARY_TYPE_STRING         =  42,
  CANARY_TYPE_UPVALUE        =  43,
} canary_type_t;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CANARY_H
