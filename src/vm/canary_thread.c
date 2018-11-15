
#include "wren_vm.h"

#include "canary_thread.h"

void
canary_thread_set_error_str_len(canary_thread_t *thread,
                                const char *error, size_t len) {
  canary_thread_set_error(thread, wrenNewStringLength(thread->vm, error, len));
}

canary_thread_t *
canary_thread_new(canary_vm_t *vm, ObjClosure* closure)
{
  // Allocate the arrays before the thread in case it triggers a GC.
  const size_t frame_capacity = CANARY_THREAD_DEFAULT_FRAME_COUNT;
  CallFrame* frames = ALLOCATE_ARRAY(vm, CallFrame, frame_capacity);
  
  // Add one slot for the unused implicit receiver slot that the compiler
  // assumes all functions have.
  const size_t stack_capacity = closure == NULL
      ? CANARY_THREAD_DEFAULT_SLOT_COUNT
      : wrenPowerOf2Ceil(closure->fn->maxSlots + 1);
  canary_value_t *stack_begin =
      ALLOCATE_ARRAY(vm, canary_value_t, stack_capacity);
  
  canary_thread_t* thread = ALLOCATE(vm, canary_thread_t);
  wrenInitObj(vm, &thread->obj, OBJ_FIBER, vm->fiberClass);
  
  thread->vm = vm;
  
  thread->stack = stack_begin;
  thread->stack_base = stack_begin;
  thread->stackTop = stack_begin;
  thread->stackCapacity = stack_capacity;

  thread->frames = frames;
  thread->frameCapacity = frame_capacity;
  thread->numFrames = 0;

  thread->openUpvalues = NULL;
  thread->caller = NULL;
  thread->error = NULL_VAL;
  thread->state = FIBER_OTHER;
  
  if (closure != NULL)
  {
    // Initialize the first call frame.
    wrenAppendCallFrame(thread, closure, thread->stack);

    // The first slot always holds the closure.
    thread->stackTop[0] = OBJ_VAL(closure);
    thread->stackTop++;
  }
  
  return thread;
}

static void
_canary_thread_ensure_stack_capacity(canary_thread_t *thread, size_t needed)
{
  CANARY_ASSERT(canary_thread_get_stack_capacity(thread) < needed,
                "Use canary_thread_ensure_stack_capacity instead.");
  
  size_t new_stack_capacity = wrenPowerOf2Ceil(needed);
  
  Value* oldStack = thread->stack;
  thread->stack = (Value*)canary_vm_realloc(thread->vm, thread->stack,
                                           sizeof(Value) * new_stack_capacity);
  thread->stackCapacity = new_stack_capacity;
  
  ptrdiff_t stack_diff = thread->stack - oldStack;
  
  // If the reallocation moves the stack, then we need to recalculate every
  // pointer that points into the old stack to into the same relative distance
  // in the new stack. We have to be a little careful about how these are
  // calculated because pointer subtraction is only well-defined within a
  // single array, hence the slightly redundant-looking arithmetic below.
  if (stack_diff != 0)
  {
    // Stack pointer for each call frame.
    for (size_t i = 0; i < thread->numFrames; i++)
    {
      CallFrame* frame = &thread->frames[i];
      frame->stackStart += stack_diff;
    }
    
    // Open upvalues.
    for (ObjUpvalue* upvalue = thread->openUpvalues;
         upvalue != NULL;
         upvalue = upvalue->next)
    {
      upvalue->value += stack_diff;
    }
    
    thread->stack_base += stack_diff;
    thread->stackTop += stack_diff;
  }
}

void
canary_thread_ensure_stack_capacity(canary_thread_t *thread, size_t needed)
 {
  if (canary_thread_get_stack_capacity(thread) >= needed) return;
  
  _canary_thread_ensure_stack_capacity(thread, needed);
}

void
canary_thread_set_frame_size(canary_thread_t *thread, canary_slot_t numSlots) {
  size_t old_stack_size = (size_t)(thread->stack_base - thread->stack);
  size_t new_stack_size = old_stack_size + numSlots;
  
  // Grow the stack if needed.
  canary_thread_ensure_stack_capacity(thread, new_stack_size);
  
  canary_value_t *old_stack_top = thread->stackTop;
  canary_value_t *new_stack_top = &thread->stack_base[numSlots];
  
  // Reset the growing stack
  for (; old_stack_top < new_stack_top; old_stack_top++) {
    *old_stack_top = NULL_VAL;
  }
  
  thread->stackTop = new_stack_top;
}
