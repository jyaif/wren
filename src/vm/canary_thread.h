
#ifndef CANARY_THREAD_H
#define CANARY_THREAD_H

#include "canary_value.h"

// The global object memory associated to this thread.
static inline canary_objectmemory_t *
canary_thread_get_object_memory(canary_thread_t *thread) {
  return thread->vm;
}

// The number of allocated slots in the frame array.
static inline canary_slot_t
canary_thread_get_frame_size(const canary_thread_t *thread) {
  return thread->stackTop - thread->stack_base;
}

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

// The number of allocated slots in the stack array.
static inline size_t
canary_thread_get_stack_capacity(const canary_thread_t *thread) {
  return thread->stackCapacity;
}

// The number of used slots in the stack array.
static inline size_t
canary_thread_get_stack_size(const canary_thread_t *thread) {
  return thread->stackTop - thread->stack;
}

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

#endif // CANARY_THREAD_H
