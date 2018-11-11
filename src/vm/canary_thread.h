
#ifndef CANARY_THREAD_H
#define CANARY_THREAD_H

#include "canary_value.h"

// The number of allocated slots in the frame array.
static inline canary_slot_t
canary_thread_get_frame_size(const canary_thread_t *thread) {
  return thread->stackTop - thread->stack_base;
}

// Ensures that [slot] is a valid index into the API's stack of slots.
static inline void
canary_thread_validate_slot(const canary_thread_t* thread, canary_slot_t slot) {
  ASSERT(slot < (canary_slot_t)canary_thread_get_frame_size(thread),
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

#endif // CANARY_THREAD_H
