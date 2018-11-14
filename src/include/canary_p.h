
#ifndef CANARY_P_H
#define CANARY_P_H

#ifndef CANARY_H
#error "Do not include directly. #include \"canary.h\" instead."
#endif // CANARY_H

typedef struct canary_memory_chunk_t canary_memory_chunk_t;

struct canary_memory_chunk_t {
  canary_memory_chunk_t *next;
  size_t size;
};

static inline canary_memory_chunk_t *
canary_memory_chunk_from_ptr(void *ptr) {
  return ptr != NULL ? &((canary_memory_chunk_t *)ptr)[-1] : NULL;
}

static inline void *
canary_memory_chunk_to_ptr(canary_memory_chunk_t *mc) {
  return mc != NULL ? (void *)(&mc[1]) : NULL;
}

static inline bool
canary_is_slot_type(const canary_context_t *context, canary_slot_t src_slot,
                    canary_type_t type) {
  return canary_get_slot_type(context, src_slot) == type;
}

static inline size_t
canary_malloced_size(void *ptr) {
  canary_memory_chunk_t *mc = canary_memory_chunk_from_ptr(ptr);
  
  return mc != NULL ? mc->size : 0;
}

#endif // CANARY_P_H
