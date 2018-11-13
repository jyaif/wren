
#include "wren_value.h"
#include "canary_api.h"

#include "canary_thread.h"

canary_type_t
canary_get_slot_type(const canary_context_t *context, canary_slot_t src_slot) {
  const canary_thread_t *thread = canary_context_to_thread_const(context);
  canary_value_t value = canary_thread_get_slot(thread, src_slot);

  return canary_value_get_type(value);
}

#define CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(name)                             \
  bool canary_is_slot_##name(const canary_context_t *context,                  \
                             canary_slot_t src_slot) {                         \
    const canary_thread_t *thread = canary_context_to_thread_const(context);   \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_is_##name(value);                                      \
  }                                                                            \
                                                                               \
  void canary_set_slot_##name(canary_context_t *context,                       \
                              canary_slot_t dst_slot) {                        \
    canary_thread_t *thread = canary_context_to_thread(context);               \
                                                                               \
    canary_thread_set_slot(thread, dst_slot, canary_value_##name());           \
  }

CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(undefined)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(null)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(false)
CANARY_DEFINE_BUILTIN_SINGLETON_TYPE(true)

#define CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(name, type)                       \
  bool canary_is_slot_##name(const canary_context_t *context,                  \
                             canary_slot_t src_slot) {                         \
    const canary_thread_t *thread = canary_context_to_thread_const(context);   \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_is_##name(value);                                      \
  }                                                                            \
                                                                               \
  type canary_get_slot_##name(canary_context_t *context,                       \
                              canary_slot_t src_slot) {                        \
    canary_thread_t *thread = canary_context_to_thread(context);               \
    canary_value_t value = canary_thread_get_slot(thread, src_slot);           \
                                                                               \
    return canary_value_to_##name(value);                                      \
  }                                                                            \
                                                                               \
  type canary_set_slot_##name(canary_context_t *context,                       \
                              canary_slot_t dst_slot, type primitive) {        \
    canary_thread_t *thread = canary_context_to_thread(context);               \
                                                                               \
    canary_thread_set_slot(thread, dst_slot,                                   \
                           canary_value_from_##name(primitive));               \
    return primitive;                                                          \
  }

CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(bool, bool)
CANARY_DEFINE_BUILTIN_PRIMITIVE_TYPE(double, double)
