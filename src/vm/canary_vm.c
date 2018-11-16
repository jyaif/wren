
#include "canary_vm.h"

#include "wren_vm.h"

#include <sys/types.h>
#include <stdlib.h>

// The behavior of realloc() when the size is 0 is implementation defined. It
// may return a non-NULL pointer which must not be dereferenced but nevertheless
// should be freed. To prevent that, we avoid calling realloc() with a zero
// size.
static void *
canary_default_realloc_fn(void *user_data, void *ptr, size_t newSize) {
  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, newSize);
}

static void *
_canary_vm_realloc(canary_realloc_fn_t realloc_fn, void *user_data,
                  void *ptr, size_t size) {
  canary_memory_chunk_t * mc = canary_memory_chunk_from_ptr(ptr);
  size_t mc_size = size != 0 ? sizeof(canary_memory_chunk_t) + size : 0;
  
  mc = (canary_memory_chunk_t *)realloc_fn(user_data, mc, mc_size);
  if (mc != NULL) {
    mc->next = NULL;
    mc->size = size;
  }
  return canary_memory_chunk_to_ptr(mc);
}

canary_vm_t *
canary_vm_bootstrap(canary_realloc_fn_t realloc_fn, void *user_data) {
  if (realloc_fn == NULL) {
    realloc_fn = canary_default_realloc_fn;
  }
  
  canary_vm_t *vm = (canary_vm_t *)
      _canary_vm_realloc(realloc_fn, user_data, NULL, sizeof(*vm));
  
  return vm;
}

void *
canary_vm_realloc(canary_vm_t *vm, void *ptr, size_t size) {
  const size_t old_size = canary_vm_malloced_size(ptr);
  const ssize_t diff_size = size - old_size;
  
#if WREN_DEBUG_TRACE_MEMORY
  // Explicit cast because size_t has different sizes on 32-bit and 64-bit and
  // we need a consistent type for the format string.
  printf("reallocate %p %lu -> %lu\n",
         memory, (unsigned long)old_size, (unsigned long)size);
#endif
  
  // If new bytes are being allocated, add them to the total count. If objects
  // are being completely deallocated, we don't track that (since we don't
  // track the original size). Instead, that will be handled while marking
  // during the next GC.
  vm->bytesAllocated += diff_size;
  
#if WREN_DEBUG_GC_STRESS
  // Since collecting calls this function to free things, make sure we don't
  // recurse.
  if (size > 0) wrenCollectGarbage(vm);
#else
  if (size > 0 && vm->bytesAllocated > vm->nextGC) wrenCollectGarbage(vm);
#endif
  
  ptr = _canary_vm_realloc(vm->config.reallocateFn, vm->config.userData,
                           ptr, size);
  
  // Check if realloc returned a NULL failure.
  if (canary_unlikely(ptr != NULL && size == 0)) {
    // Failed to realloc, restore the allocated size.
    vm->bytesAllocated -= diff_size;
  }
  return ptr;
}

size_t
canary_vm_malloced(const canary_vm_t *vm) {
  return vm->bytesAllocated;
}
