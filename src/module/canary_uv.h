
#ifndef CANARY_UV_H
#define CANARY_UV_H

#include "wren.h"

#include "uv.h"

static inline WrenVM *
canary_uv_loop_get_vm(uv_loop_t *loop) {
  return (WrenVM *)loop->data;
}

static inline void
canary_uv_loop_set_vm(uv_loop_t *loop, WrenVM *vm) {
  loop->data = vm;
}

static inline WrenVM *
canary_uv_fs_get_vm(uv_fs_t *fs) {
  return canary_uv_loop_get_vm(fs->loop);
}

#endif // CANARY_UV_H
