
#ifndef CANARY_THREAD_H
#define CANARY_THREAD_H

#include "canary_value.h"

typedef struct sObjClosure ObjClosure;

canary_thread_t *
canary_thread_new(canary_vm_t *vm, ObjClosure* closure);

// The global object memory associated to this thread.
static inline canary_vm_t *
canary_thread_get_vm(canary_thread_t *thread) {
  return thread->vm;
}

// The global object memory associated to this thread.
static inline const canary_vm_t *
canary_thread_get_vm_const(const canary_thread_t *thread) {
  return thread->vm;
}

// The number of allocated slots in the stack array.
static inline size_t
canary_thread_get_stack_capacity(const canary_thread_t *thread) {
  return thread->stackCapacity;
}

// Ensures [thread]'s stack has at least [needed] slots.
void
canary_thread_ensure_stack_capacity(canary_thread_t *thread, size_t needed);

// The number of used slots in the stack array.
static inline size_t
canary_thread_get_stack_size(const canary_thread_t *thread) {
  return thread->stackTop - thread->stack;
}

// The number of allocated slots in the frame array.
static inline canary_slot_t
canary_thread_get_frame_size(const canary_thread_t *thread) {
  return thread->stackTop - thread->stack_base;
}

void
canary_thread_set_frame_size(canary_thread_t *fiber, canary_slot_t numSlots);

// Ensures that [slot] is a valid index into the API's stack of slots.
static inline void
canary_thread_validate_slot(const canary_thread_t* thread, canary_slot_t slot) {
  CANARY_ASSERT(slot < (canary_slot_t)canary_thread_get_frame_size(thread),
                "Not that many slots.");
}

// Restores [value] from [srcSlot] in the foreign call stack.
static inline canary_value_t
canary_thread_get_slot(const canary_thread_t* thread, canary_slot_t srcSlot) {
  canary_thread_validate_slot(thread, srcSlot);
  return thread->stack_base[srcSlot];
}

// Stores [value] in [dstSlot] in the foreign call stack.
static inline canary_value_t
canary_thread_set_slot(canary_thread_t* thread, canary_slot_t dstSlot,
                       canary_value_t value) {
  canary_thread_validate_slot(thread, dstSlot);
  return thread->stack_base[dstSlot] = value;
}

static inline canary_type_t
canary_thread_get_slot_type(const canary_thread_t *thread,
                            canary_slot_t src_slot) {
  canary_value_t value = canary_thread_get_slot(thread, src_slot);

  return canary_value_get_type(value);
}

#define CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(name)                             \
  static inline bool                                                           \
  canary_thread_is_slot_##name(const canary_thread_t *thread,                  \
                             canary_slot_t src_slot) {                         \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_is_##name(value);                                      \
  }                                                                            \
                                                                               \
  static inline void                                                           \
  canary_thread_set_slot_##name(canary_thread_t *thread,                       \
                              canary_slot_t dst_slot) {                        \
    canary_thread_set_slot(thread, dst_slot, canary_value_##name());           \
  }

CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(undefined)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(null)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(false)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(true)

#undef CANARY_DEFINE_BUILTIN_SINGLETON_TYPE

#define CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(name, type)                       \
  static inline bool                                                           \
  canary_thread_is_slot_##name(const canary_thread_t *thread,                  \
                               canary_slot_t src_slot) {                       \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_is_##name(value);                                      \
  }                                                                            \
                                                                               \
  static inline type                                                           \
  canary_thread_get_slot_##name(canary_thread_t *thread,                       \
                                canary_slot_t src_slot) {                      \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_to_##name(value);                                      \
  }                                                                            \
                                                                               \
  static inline type                                                           \
  canary_thread_set_slot_##name(canary_thread_t *thread,                       \
                                canary_slot_t dst_slot, type primitive) {      \
    canary_thread_set_slot(thread, dst_slot,                                   \
                           canary_value_from_##name(primitive));               \
    return primitive;                                                          \
  }

CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(bool, bool)
CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(double, double)

#undef CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE

static inline bool
canary_thread_has_error(const canary_thread_t *thread) {
  return !IS_NULL(thread->error);
}

static inline canary_value_t
canary_thread_get_error(const canary_thread_t *thread) {
  return thread->error;
}

static inline void
canary_thread_set_error(canary_thread_t *thread, canary_value_t error) {
  if (canary_thread_has_error(thread)) return; // Do not clobber previous error.
  
  thread->error = error;
}

// TODO: inline
void
canary_thread_set_error_str_len(canary_thread_t *thread,
                                const char *error, size_t len);

static inline void
canary_thread_set_error_str(canary_thread_t *thread, const char *error) {
  canary_thread_set_error_str_len(thread, error, strlen(error));
}

#define canary_thread_set_error_str_format(thread, ...)                        \
  do {                                                                         \
    canary_thread_set_error((thread),                                          \
                            wrenStringFormat((thread)->vm, __VA_ARGS__));      \
  } while(false)

static inline size_t
canary_thread_get_frame_stack_capacity(const canary_thread_t *thread) {
  return thread->frameCapacity;
}

void
canary_thread_ensure_frame_stack_capacity(canary_thread_t *thread,
                                          size_t needed);

// Pushes [closure] onto [thread]'s callstack to invoke it. Expects [numArgs]
// arguments (including the receiver) to be on the top of the stack already.
void
canary_thread_call_function(canary_thread_t *thread, ObjClosure* closure,
                            canary_slot_t numArgs);

#endif // CANARY_THREAD_H
