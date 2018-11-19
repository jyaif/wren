
#include "wren_value.h"
#include "canary_api.h"

#include "canary_thread.h"

canary_context_t *
canary_bootstrap(canary_realloc_fn_t realloc_fn, void *user_data) {
  return NULL;
}

canary_type_t
canary_get_slot_type(const canary_context_t *context, canary_slot_t src_slot) {
  const canary_thread_t *thread = canary_context_to_thread_const(context);
  canary_value_t value = canary_thread_get_slot(thread, src_slot);

  return canary_value_get_type(value);
}

#define CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(name)                             \
  bool                                                                         \
  canary_is_slot_##name(const canary_context_t *context,                       \
                        canary_slot_t src_slot) {                              \
    const canary_thread_t *thread = canary_context_to_thread_const(context);   \
                                                                               \
    return canary_thread_is_slot_##name(thread, src_slot);                     \
  }                                                                            \
                                                                               \
  void canary_set_slot_##name(canary_context_t *context,                       \
                              canary_slot_t dst_slot) {                        \
    canary_thread_t *thread = canary_context_to_thread(context);               \
                                                                               \
    canary_thread_set_slot_##name(thread, dst_slot);                           \
  }

CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(undefined)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(null)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(false)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(true)

#undef CANARY_DEFINE_BUILTIN_SINGLETON_TYPE

#define CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(name, type)                       \
  bool                                                                         \
  canary_is_slot_##name(const canary_context_t *context,                       \
                        canary_slot_t src_slot) {                              \
    const canary_thread_t *thread = canary_context_to_thread_const(context);   \
                                                                               \
    return canary_thread_is_slot_##name(thread, src_slot);                     \
  }                                                                            \
                                                                               \
  type                                                                         \
  canary_get_slot_##name(canary_context_t *context, canary_slot_t src_slot) {  \
    canary_thread_t *thread = canary_context_to_thread(context);               \
                                                                               \
    return canary_thread_get_slot_##name(thread, src_slot);                    \
  }                                                                            \
                                                                               \
  type                                                                         \
  canary_set_slot_##name(canary_context_t *context,                            \
                         canary_slot_t dst_slot, type primitive) {             \
    canary_thread_t *thread = canary_context_to_thread(context);               \
                                                                               \
    return canary_thread_set_slot_##name(thread, dst_slot, primitive);         \
  }

CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(bool, bool)
CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(double, double)

#undef CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE

// Test if the current context has an error.
bool
canary_has_error(const canary_context_t *context) {
  const canary_thread_t *thread = canary_context_to_thread_const(context);
  
  return canary_thread_has_error(thread);
}

// Gets the error from the [src_context] into [context] [dst_slot] slot.
void
canary_get_error(canary_context_t *context, canary_slot_t dst_slot,
                 const canary_context_t *src_context) {
  canary_thread_t *thread = canary_context_to_thread(context);
  const canary_thread_t *src_thread =
       canary_context_to_thread_const(src_context);
  canary_value_t error = canary_thread_get_error(src_thread);
  
  canary_thread_set_slot(thread, dst_slot, error);
}

// Sets the [context] error form [src_slot] slot.
void
canary_set_error(canary_context_t *context, canary_slot_t src_slot) {
  canary_thread_t *thread = canary_context_to_thread(context);
  canary_value_t error = canary_thread_get_slot(thread, src_slot);
  
  canary_thread_set_error(thread, error);
}

void *
canary_realloc(canary_context_t *context, void *ptr, size_t size) {
  canary_vm_t *vm = canary_context_to_vm(context);
  
  return canary_vm_realloc(vm, ptr, size);
}

void *
canary_malloc(canary_context_t *context, size_t size) {
  canary_vm_t *vm = canary_context_to_vm(context);
  
  return canary_vm_malloc(vm, size);
}

void
canary_free(canary_context_t *context, void *ptr) {
  canary_vm_t *vm = canary_context_to_vm(context);
  
  canary_vm_free(vm, ptr);
}

size_t
canary_malloced(const canary_context_t *context) {
  const canary_vm_t *vm = canary_context_to_vm_const(context);
  
  return canary_vm_malloced(vm);
}

size_t
canary_malloced_size(void *ptr) {
  return canary_vm_malloced_size(ptr);
}
