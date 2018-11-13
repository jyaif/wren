
#ifndef CANARY_H
#define CANARY_H

#include <stdbool.h>
#include <stddef.h>
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

typedef struct canary_context_t canary_context_t;
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

canary_type_t
canary_get_slot_type(const canary_context_t *context, canary_slot_t src_slot);

static inline bool
canary_is_slot_type(const canary_context_t *context, canary_slot_t src_slot,
                    canary_type_t type);

#define CANARY_DECLARE_BUILTIN_SINGLETON_TYPE(name)                            \
  bool canary_is_slot_##name(const canary_context_t *context,                  \
                             canary_slot_t src_slot);                          \
                                                                               \
  void canary_set_slot_##name(canary_context_t *context, canary_slot_t dst_slot)

CANARY_DECLARE_BUILTIN_SINGLETON_TYPE(undefined);
CANARY_DECLARE_BUILTIN_SINGLETON_TYPE(null);
CANARY_DECLARE_BUILTIN_SINGLETON_TYPE(false);
CANARY_DECLARE_BUILTIN_SINGLETON_TYPE(true);

#define CANARY_DECLARE_BUILTIN_PRIMITIVE_TYPE(name, type)                      \
  bool canary_is_slot_##name(const canary_context_t *context,                  \
                             canary_slot_t src_slot);                          \
                                                                               \
  type canary_get_slot_##name(canary_context_t *context,                       \
                              canary_slot_t src_slot);                         \
                                                                               \
  type canary_set_slot_##name(canary_context_t *context,                       \
                              canary_slot_t dst_slot, type primitive)

CANARY_DECLARE_BUILTIN_PRIMITIVE_TYPE(bool, bool);
CANARY_DECLARE_BUILTIN_PRIMITIVE_TYPE(double, double);

#include "canary_p.h"

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CANARY_H
