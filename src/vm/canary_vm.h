
#ifndef CANARY_VM_H
#define CANARY_VM_H

#include "canary_types.h"

canary_vm_t *
canary_vm_bootstrap(canary_realloc_fn_t realloc_fn, void *user_data);

void *
canary_vm_realloc(canary_vm_t *vm, void *ptr, size_t size);

static inline void *
canary_vm_malloc(canary_vm_t *vm, size_t size) {
  return canary_vm_realloc(vm, NULL, size);
}

static inline void
canary_vm_free(canary_vm_t *vm, void *ptr) {
  canary_vm_realloc(vm, ptr, 0);
}

size_t
canary_vm_malloced(const canary_vm_t *vm);

#endif // CANARY_VM_H
