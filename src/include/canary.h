
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

// A generic allocation function that handles all explicit memory management.
// It's used like so:
//
// - To allocate new memory, [memory] is NULL and [newSize] is the desired
//   size. It should return the allocated memory or NULL on failure.
//
// - To attempt to grow an existing allocation, [memory] is the memory, and
//   [newSize] is the desired size. It should return [memory] if it was able to
//   grow it in place, or a new pointer if it had to move it.
//
// - To shrink memory, [memory] and [newSize] are the same as above but it will
//   always return [memory].
//
// - To free memory, [memory] will be the memory to free and [newSize] will be
//   zero. It should return NULL.
typedef void* (*canary_realloc_fn_t)(void *user_data,
                                     void *memory, size_t newSize);

canary_context_t *
canary_bootstrap(canary_realloc_fn_t realloc_fn, void *user_data);

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

// Test if the current context has an error.
bool
canary_has_error(const canary_context_t *context);

// Gets the error from the [src_context] into [context] [dst_slot] slot.
void
canary_get_error(canary_context_t *context, canary_slot_t dst_slot,
                 const canary_context_t *src_context);

// Sets the [context] error form [src_slot] slot.
void
canary_set_error(canary_context_t *context, canary_slot_t src_slot);

void *
canary_realloc(canary_context_t *context, void *ptr, size_t size);

void *
canary_malloc(canary_context_t *context, size_t size);

size_t
canary_malloced_size(void *ptr);

void
canary_free(canary_context_t *context, void *ptr);

size_t
canary_malloced(const canary_context_t *context);

#include "canary_p.h"

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CANARY_H
