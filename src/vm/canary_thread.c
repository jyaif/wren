
#include "wren_vm.h"

#include "canary_thread.h"

void
canary_thread_set_error_str_len(canary_thread_t *thread,
                                const char *error, size_t len) {
  canary_thread_set_error(thread, wrenNewStringLength(thread->vm, error, len));
}
