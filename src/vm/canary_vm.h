
#ifndef CANARY_VM_H
#define CANARY_VM_H

#include "canary_types.h"

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

canary_vm_t *
canary_vm_bootstrap(canary_realloc_fn_t realloc_fn, void *user_data);

void *
canary_vm_realloc(canary_vm_t *vm, void *ptr, size_t size);

static inline void *
canary_vm_malloc(canary_vm_t *vm, size_t size) {
  return canary_vm_realloc(vm, NULL, size);
}

static inline size_t
canary_vm_malloced_size(void *ptr) {
  canary_memory_chunk_t *mc = canary_memory_chunk_from_ptr(ptr);
  
  return mc != NULL ? mc->size : 0;
}

static inline void
canary_vm_free(canary_vm_t *vm, void *ptr) {
  canary_vm_realloc(vm, ptr, 0);
}

size_t
canary_vm_malloced(const canary_vm_t *vm);

#endif // CANARY_VM_H
