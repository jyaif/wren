
#ifndef CANARY_API_H
#define CANARY_API_H

#include "canary.h"
#include "canary_vm.h"

static inline canary_context_t *
canary_context_from_thread(canary_thread_t *thread) {
  return (canary_context_t *)(void *)thread;
}

static inline const canary_context_t *
canary_context_from_thread_const(const canary_thread_t *thread) {
  return (const canary_context_t *)(const void *)thread;
}

static inline canary_thread_t *
canary_context_to_thread(canary_context_t *context) {
  return (canary_thread_t *)(void *)context;
}

static inline const canary_thread_t *
canary_context_to_thread_const(const canary_context_t *context) {
  return (const canary_thread_t *)(const void *)context;
}

static inline canary_vm_t *
canary_context_to_vm(canary_context_t *context) {
  canary_thread_t *thread = canary_context_to_thread(context);
  
  return thread != NULL ? canary_thread_get_vm(thread) : NULL;
}

static inline const canary_vm_t *
canary_context_to_vm_const(const canary_context_t *context) {
  const canary_thread_t *thread = canary_context_to_thread_const(context);
  
  return thread != NULL ? canary_thread_get_vm_const(thread) : NULL;
}

#endif // CANARY_API_H
