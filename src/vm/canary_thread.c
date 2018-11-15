
#include "wren_vm.h"

#include "canary_thread.h"

#include "canary_api.h"

#include "wren_debug.h"

// Adds a new [CallFrame] to [thread] invoking [closure] whose stack starts at
// [stackStart].
static void
canary_thread_push_frame(canary_thread_t *thread, ObjClosure* closure,
                         canary_value_t* stackStart);

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
    canary_thread_push_frame(thread, closure, thread->stack);

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
    for (size_t i = 0; i < canary_thread_get_frame_stack_size(thread); i++)
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

static void
_canary_thread_ensure_frame_stack_capacity(canary_thread_t *thread,
                                          size_t needed) {
  CANARY_ASSERT(canary_thread_get_frame_stack_capacity(thread) < needed,
                "Use canary_thread_get_frame_stack_capacity instead.");
  
  size_t max = thread->frameCapacity * 2;
  thread->frames = (CallFrame*)
      canary_vm_realloc(thread->vm, thread->frames, sizeof(CallFrame) * max);
  thread->frameCapacity = max;
}

void
canary_thread_ensure_frame_stack_capacity(canary_thread_t *thread,
                                          size_t needed) {
  if (canary_thread_get_frame_stack_capacity(thread) >= needed) return;
  
  _canary_thread_ensure_frame_stack_capacity(thread, needed);
}

void
canary_thread_push_frame(canary_thread_t *thread, ObjClosure* closure,
                         canary_value_t* stackStart) {
  // The caller should have ensured we already have enough capacity.
  ASSERT(canary_thread_get_stack_capacity(thread) >
         canary_thread_get_frame_stack_size(thread),
         "No memory for call frame.");
  
  CallFrame* frame = &thread->frames[thread->numFrames++];
  frame->stackStart = stackStart;
  frame->closure = closure;
  frame->ip = closure->fn->code.data;
}

void
canary_thread_call_function(canary_thread_t *thread, ObjClosure* closure,
                            canary_slot_t numArgs) {
  // Grow the call frame array if needed.
  canary_thread_ensure_frame_stack_capacity(
      thread, canary_thread_get_frame_stack_size(thread) + 1);
  
  // Grow the stack if needed.
  size_t stackSize = canary_thread_get_stack_size(thread);
  size_t needed = stackSize + closure->fn->maxSlots;
  canary_thread_ensure_stack_capacity(thread, needed);
  
  canary_thread_push_frame(thread, closure, thread->stackTop - numArgs);
}

// Captures the local variable [local] into an [Upvalue]. If that local is
// already in an upvalue, the existing one will be used. (This is important to
// ensure that multiple closures closing over the same variable actually see
// the same variable.) Otherwise, it will create a new open upvalue and add it
// the fiber's list of upvalues.
static ObjUpvalue* captureUpvalue(ObjFiber* fiber, Value* local)
{
  // If there are no open upvalues at all, we must need a new one.
  if (fiber->openUpvalues == NULL)
  {
    fiber->openUpvalues = wrenNewUpvalue(fiber->vm, local);
    return fiber->openUpvalues;
  }

  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = fiber->openUpvalues;

  // Walk towards the bottom of the stack until we find a previously existing
  // upvalue or pass where it should be.
  while (upvalue != NULL && upvalue->value > local)
  {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  // Found an existing upvalue for this local.
  if (upvalue != NULL && upvalue->value == local) return upvalue;

  // We've walked past this local on the stack, so there must not be an
  // upvalue for it already. Make a new one and link it in in the right
  // place to keep the list sorted.
  ObjUpvalue* createdUpvalue = wrenNewUpvalue(fiber->vm, local);
  if (prevUpvalue == NULL)
  {
    // The new one is the first one in the list.
    fiber->openUpvalues = createdUpvalue;
  }
  else
  {
    prevUpvalue->next = createdUpvalue;
  }

  createdUpvalue->next = upvalue;
  return createdUpvalue;
}

// Closes any open upvates that have been created for stack slots at [last] and
// above.
static void closeUpvalues(ObjFiber* fiber, Value* last)
{
  while (fiber->openUpvalues != NULL &&
         fiber->openUpvalues->value >= last)
  {
    ObjUpvalue* upvalue = fiber->openUpvalues;

    // Move the value into the upvalue itself and point the upvalue to it.
    upvalue->closed = *upvalue->value;
    upvalue->value = &upvalue->closed;

    // Remove it from the open upvalue list.
    fiber->openUpvalues = upvalue->next;
  }
}

static void callForeign(ObjFiber* fiber,
                        WrenForeignMethodFn foreign, int numArgs)
{
  canary_context_t *context = canary_context_from_thread(fiber);
  // Save the current state so we can restore it when done.
  WrenVM* vm = fiber->vm;
  bool old_is_api_call = vm->is_api_call;
  vm->is_api_call = true;
  ptrdiff_t old_stack_base_diff = fiber->stack_base - fiber->stack;
  fiber->stack_base = fiber->stackTop - numArgs;

  foreign(context);

  // Discard the stack slots for the arguments and temporaries but leave one
  // for the result.
  fiber->stackTop = fiber->stack_base + 1;
  fiber->stack_base = fiber->stack + old_stack_base_diff;
  vm->is_api_call = old_is_api_call;
}

// Handles the current fiber having aborted because of an error.
//
// Walks the call chain of fibers, aborting each one until it hits a fiber that
// handles the error. If none do, tells the VM to stop.
static void runtimeError(WrenVM* vm)
{
  ASSERT(canary_thread_has_error(vm->fiber),
         "Should only call this after an error.");

  ObjFiber* current = vm->fiber;
  Value error = canary_thread_get_error(current);
  
  while (current != NULL)
  {
    // Every fiber along the call chain gets aborted with the same error.
    canary_thread_set_error(current, error);

    // If the caller ran this fiber using "try", give it the error and stop.
    if (current->state == FIBER_TRY)
    {
      // Make the caller's try method return the error message.
      current->caller->stackTop[-1] = canary_thread_get_error(vm->fiber);
      vm->fiber = current->caller;
      return;
    }
    
    // Otherwise, unhook the caller since we will never resume and return to it.
    ObjFiber* caller = current->caller;
    current->caller = NULL;
    current = caller;
  }

  // If we got here, nothing caught the error, so show the stack trace.
  wrenDebugPrintStackTrace(vm);
  vm->fiber = NULL;
  vm->is_api_call = false;
}

// Aborts the current fiber with an appropriate method not found error for a
// method with [symbol] on [classObj].
static void methodNotFound(WrenVM* vm, ObjClass* classObj, int symbol)
{
  canary_thread_t *thread = vm->fiber;
  
  canary_thread_set_error_str_format(thread, "@ does not implement '$'.",
      OBJ_VAL(classObj->name), vm->methodNames.data[symbol]->value);
}

WrenInterpretResult
canary_thread_interpret(canary_thread_t *fiber) {
  WrenVM *vm = fiber->vm;
  
  // Remember the current fiber so we can find it if a GC happens.
  vm->fiber = fiber;
  fiber->state = FIBER_ROOT;

  // Hoist these into local variables. They are accessed frequently in the loop
  // but assigned less frequently. Keeping them in locals and updating them when
  // a call frame has been pushed or popped gives a large speed boost.
  register CallFrame* frame;
  register Value* stackStart;
  register uint8_t* ip;
  register ObjFn* fn;

  // These macros are designed to only be invoked within this function.
  #define PUSH(value)  (*fiber->stackTop++ = value)
  #define POP()        (*(--fiber->stackTop))
  #define DROP()       (fiber->stackTop--)
  #define PEEK()       (*(fiber->stackTop - 1))
  #define PEEK2()      (*(fiber->stackTop - 2))
  #define READ_BYTE()  (*ip++)
  #define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

  // Use this before a CallFrame is pushed to store the local variables back
  // into the current one.
  #define STORE_FRAME() frame->ip = ip

  // Use this after a CallFrame has been pushed or popped to refresh the local
  // variables.
  #define LOAD_FRAME()                                 \
      frame = &fiber->frames[fiber->numFrames - 1];    \
      stackStart = frame->stackStart;                  \
      ip = frame->ip;                                  \
      fn = frame->closure->fn;

  // Terminates the current fiber with error string [error]. If another calling
  // fiber is willing to catch the error, transfers control to it, otherwise
  // exits the interpreter.
  #define RUNTIME_ERROR()                                         \
      do                                                          \
      {                                                           \
        STORE_FRAME();                                            \
        runtimeError(vm);                                         \
        if (vm->fiber == NULL) return WREN_RESULT_RUNTIME_ERROR;  \
        fiber = vm->fiber;                                        \
        LOAD_FRAME();                                             \
        DISPATCH();                                               \
      }                                                           \
      while (false)

  #define DEBUG_CURRENT_FRAME_IS_IN_SYNC()                                     \
    do {                                                                       \
      ASSERT(fiber == vm->fiber, "Fiber out of sync.");                        \
      ASSERT(frame == &fiber->frames[fiber->numFrames - 1], "Frame out of sync."); \
      ASSERT(stackStart == frame->stackStart, "StackStart out of sync.");      \
      ASSERT(fn == frame->closure->fn, "Fn out of sync.");                     \
    } while (false)
  
  #if WREN_DEBUG_TRACE_INSTRUCTIONS
    // Prints the stack and instruction before each instruction is executed.
    #define DEBUG_TRACE_INSTRUCTIONS()                            \
        do                                                        \
        {                                                         \
          wrenDumpStack(fiber);                                   \
          wrenDumpInstruction(vm, fn, (int)(ip - fn->code.data)); \
        }                                                         \
        while (false)
  #else
    #define DEBUG_TRACE_INSTRUCTIONS() do { } while (false)
  #endif

  #if CANARY_THREAD_INTERPRETER_FLAVOR == CANARY_THREAD_INTERPRETER_FLAVOR_GOTO

  static void* dispatchTable[] = {
    #define OPCODE(name, _) &&code_##name,
    #include "wren_opcodes.h"
    #undef OPCODE
  };

  #define INTERPRET_LOOP    DISPATCH();
  #define CASE_CODE(name)   code_##name

  #define DISPATCH()                                            \
      do                                                        \
      {                                                         \
        DEBUG_CURRENT_FRAME_IS_IN_SYNC();                                      \
        DEBUG_TRACE_INSTRUCTIONS();                             \
        goto *dispatchTable[instruction = (Code)READ_BYTE()];   \
      }                                                         \
      while (false)

  #elif CANARY_THREAD_INTERPRETER_FLAVOR ==                                    \
      CANARY_THREAD_INTERPRETER_FLAVOR_SWITCH

  #define INTERPRET_LOOP                                        \
      loop:                                                     \
        DEBUG_CURRENT_FRAME_IS_IN_SYNC();                                      \
        DEBUG_TRACE_INSTRUCTIONS();                             \
        switch (instruction = (Code)READ_BYTE())

  #define CASE_CODE(name)  case CODE_##name
  #define DISPATCH()       goto loop

  #else // CANARY_THREAD_INTERPRETER_FLAVOR
  #error "Unupported CANARY_THREAD_INTERPRETER_FLAVOR."
  #endif // CANARY_THREAD_INTERPRETER_FLAVOR

  LOAD_FRAME();

  Code instruction;
  INTERPRET_LOOP
  {
    CASE_CODE(LOAD_LOCAL_0):
    CASE_CODE(LOAD_LOCAL_1):
    CASE_CODE(LOAD_LOCAL_2):
    CASE_CODE(LOAD_LOCAL_3):
    CASE_CODE(LOAD_LOCAL_4):
    CASE_CODE(LOAD_LOCAL_5):
    CASE_CODE(LOAD_LOCAL_6):
    CASE_CODE(LOAD_LOCAL_7):
    CASE_CODE(LOAD_LOCAL_8):
      PUSH(stackStart[instruction - CODE_LOAD_LOCAL_0]);
      DISPATCH();

    CASE_CODE(LOAD_LOCAL):
      PUSH(stackStart[READ_BYTE()]);
      DISPATCH();

    CASE_CODE(LOAD_FIELD_THIS):
    {
      uint8_t field = READ_BYTE();
      Value receiver = stackStart[0];
      ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
      ObjInstance* instance = AS_INSTANCE(receiver);
      ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
      PUSH(instance->fields[field]);
      DISPATCH();
    }

    CASE_CODE(POP):   DROP(); DISPATCH();
    CASE_CODE(NULL):  PUSH(NULL_VAL); DISPATCH();
    CASE_CODE(FALSE): PUSH(FALSE_VAL); DISPATCH();
    CASE_CODE(TRUE):  PUSH(TRUE_VAL); DISPATCH();

    CASE_CODE(STORE_LOCAL):
      stackStart[READ_BYTE()] = PEEK();
      DISPATCH();

    CASE_CODE(CONSTANT):
      PUSH(fn->constants.data[READ_SHORT()]);
      DISPATCH();

    {
      // The opcodes for doing method and superclass calls share a lot of code.
      // However, doing an if() test in the middle of the instruction sequence
      // to handle the bit that is special to super calls makes the non-super
      // call path noticeably slower.
      //
      // Instead, we do this old school using an explicit goto to share code for
      // everything at the tail end of the call-handling code that is the same
      // between normal and superclass calls.
      int numArgs;
      int symbol;

      Value* args;
      ObjClass* classObj;

      Method* method;

    CASE_CODE(CALL_0):
    CASE_CODE(CALL_1):
    CASE_CODE(CALL_2):
    CASE_CODE(CALL_3):
    CASE_CODE(CALL_4):
    CASE_CODE(CALL_5):
    CASE_CODE(CALL_6):
    CASE_CODE(CALL_7):
    CASE_CODE(CALL_8):
    CASE_CODE(CALL_9):
    CASE_CODE(CALL_10):
    CASE_CODE(CALL_11):
    CASE_CODE(CALL_12):
    CASE_CODE(CALL_13):
    CASE_CODE(CALL_14):
    CASE_CODE(CALL_15):
    CASE_CODE(CALL_16):
      // Add one for the implicit receiver argument.
      numArgs = instruction - CODE_CALL_0 + 1;
      symbol = READ_SHORT();

      // The receiver is the first argument.
      args = fiber->stackTop - numArgs;
      classObj = wrenGetClassInline(vm, args[0]);
      goto completeCall;

    CASE_CODE(SUPER_0):
    CASE_CODE(SUPER_1):
    CASE_CODE(SUPER_2):
    CASE_CODE(SUPER_3):
    CASE_CODE(SUPER_4):
    CASE_CODE(SUPER_5):
    CASE_CODE(SUPER_6):
    CASE_CODE(SUPER_7):
    CASE_CODE(SUPER_8):
    CASE_CODE(SUPER_9):
    CASE_CODE(SUPER_10):
    CASE_CODE(SUPER_11):
    CASE_CODE(SUPER_12):
    CASE_CODE(SUPER_13):
    CASE_CODE(SUPER_14):
    CASE_CODE(SUPER_15):
    CASE_CODE(SUPER_16):
      // Add one for the implicit receiver argument.
      numArgs = instruction - CODE_SUPER_0 + 1;
      symbol = READ_SHORT();

      // The receiver is the first argument.
      args = fiber->stackTop - numArgs;

      // The superclass is stored in a constant.
      classObj = AS_CLASS(fn->constants.data[READ_SHORT()]);
      goto completeCall;

    completeCall:
      // If the class's method table doesn't include the symbol, bail.
      if (symbol >= classObj->methods.count ||
          (method = &classObj->methods.data[symbol])->type == METHOD_NONE)
      {
        methodNotFound(vm, classObj, symbol);
        RUNTIME_ERROR();
      }

      switch (method->type)
      {
        case METHOD_PRIMITIVE:
          if (method->as.primitive(fiber, args))
          {
            // The result is now in the first arg slot. Discard the other
            // stack slots.
            fiber->stackTop -= numArgs - 1;
          } else {
            // An error, fiber switch, or call frame change occurred.
            STORE_FRAME();

            // If we don't have a fiber to switch to, stop interpreting.
            fiber = vm->fiber;
            if (fiber == NULL) return WREN_RESULT_SUCCESS;
            if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
            LOAD_FRAME();
          }
          break;

        case METHOD_FOREIGN:
          STORE_FRAME();
          callForeign(fiber, method->as.foreign, numArgs);
          if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
          LOAD_FRAME();
          break;

        case METHOD_BLOCK:
          STORE_FRAME();
          canary_thread_call_function(fiber, method->as.closure, numArgs);
          LOAD_FRAME();
          break;

        case METHOD_NONE:
          UNREACHABLE();
          break;
      }
      DISPATCH();
    }

    CASE_CODE(LOAD_UPVALUE):
    {
      ObjUpvalue** upvalues = frame->closure->upvalues;
      PUSH(*upvalues[READ_BYTE()]->value);
      DISPATCH();
    }

    CASE_CODE(STORE_UPVALUE):
    {
      ObjUpvalue** upvalues = frame->closure->upvalues;
      *upvalues[READ_BYTE()]->value = PEEK();
      DISPATCH();
    }

    CASE_CODE(LOAD_MODULE_VAR):
      PUSH(fn->module->variables.data[READ_SHORT()]);
      DISPATCH();

    CASE_CODE(STORE_MODULE_VAR):
      fn->module->variables.data[READ_SHORT()] = PEEK();
      DISPATCH();

    CASE_CODE(STORE_FIELD_THIS):
    {
      uint8_t field = READ_BYTE();
      Value receiver = stackStart[0];
      ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
      ObjInstance* instance = AS_INSTANCE(receiver);
      ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
      instance->fields[field] = PEEK();
      DISPATCH();
    }

    CASE_CODE(LOAD_FIELD):
    {
      uint8_t field = READ_BYTE();
      Value receiver = POP();
      ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
      ObjInstance* instance = AS_INSTANCE(receiver);
      ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
      PUSH(instance->fields[field]);
      DISPATCH();
    }

    CASE_CODE(STORE_FIELD):
    {
      uint8_t field = READ_BYTE();
      Value receiver = POP();
      ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
      ObjInstance* instance = AS_INSTANCE(receiver);
      ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
      instance->fields[field] = PEEK();
      DISPATCH();
    }

    CASE_CODE(JUMP):
    {
      uint16_t offset = READ_SHORT();
      ip += offset;
      DISPATCH();
    }

    CASE_CODE(LOOP):
    {
      // Jump back to the top of the loop.
      uint16_t offset = READ_SHORT();
      ip -= offset;
      DISPATCH();
    }

    CASE_CODE(JUMP_IF):
    {
      uint16_t offset = READ_SHORT();
      Value condition = POP();

      if (IS_FALSE(condition) || IS_NULL(condition)) ip += offset;
      DISPATCH();
    }

    CASE_CODE(AND):
    {
      uint16_t offset = READ_SHORT();
      Value condition = PEEK();

      if (IS_FALSE(condition) || IS_NULL(condition))
      {
        // Short-circuit the right hand side.
        ip += offset;
      }
      else
      {
        // Discard the condition and evaluate the right hand side.
        DROP();
      }
      DISPATCH();
    }

    CASE_CODE(OR):
    {
      uint16_t offset = READ_SHORT();
      Value condition = PEEK();

      if (IS_FALSE(condition) || IS_NULL(condition))
      {
        // Discard the condition and evaluate the right hand side.
        DROP();
      }
      else
      {
        // Short-circuit the right hand side.
        ip += offset;
      }
      DISPATCH();
    }

    CASE_CODE(CLOSE_UPVALUE):
      // Close the upvalue for the local if we have one.
      closeUpvalues(fiber, fiber->stackTop - 1);
      DROP();
      DISPATCH();

    CASE_CODE(RETURN):
    {
      Value result = POP();
      fiber->numFrames--;

      // Close any upvalues still in scope.
      closeUpvalues(fiber, stackStart);

      // Store the result of the block in the first slot, which is where the
      // caller expects it.
      stackStart[0] = result;
      
      // Discard the stack slots for the call frame (leaving one slot for the
      // result).
      fiber->stackTop = stackStart + 1;
      
      // If the fiber is complete, end it.
      if (fiber->numFrames == 0)
      {
        // See if there's another fiber to return to. If not, we're done.
        if (fiber->caller == NULL)
        {
          return WREN_RESULT_SUCCESS;
        }
        
        ObjFiber* resumingFiber = fiber->caller;
        fiber->caller = NULL;
        fiber = resumingFiber;
        vm->fiber = resumingFiber;
        
        // Store the result in the resuming fiber.
        fiber->stackTop[-1] = result;
      }
      
      LOAD_FRAME();
      DISPATCH();
    }

    CASE_CODE(CONSTRUCT):
      ASSERT(IS_CLASS(stackStart[0]), "'this' should be a class.");
      stackStart[0] = wrenNewInstance(vm, AS_CLASS(stackStart[0]));
      DISPATCH();

    CASE_CODE(FOREIGN_CONSTRUCT):
      ASSERT(IS_CLASS(stackStart[0]), "'this' should be a class.");
      createForeign(fiber, stackStart);
      DISPATCH();

    CASE_CODE(CLOSURE):
    {
      // Create the closure and push it on the stack before creating upvalues
      // so that it doesn't get collected.
      ObjFn* function = AS_FN(fn->constants.data[READ_SHORT()]);
      ObjClosure* closure = wrenNewClosure(vm, function);
      PUSH(OBJ_VAL(closure));

      // Capture upvalues, if any.
      for (int i = 0; i < function->numUpvalues; i++)
      {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal)
        {
          // Make an new upvalue to close over the parent's local variable.
          closure->upvalues[i] = captureUpvalue(fiber,
                                                frame->stackStart + index);
        }
        else
        {
          // Use the same upvalue as the current call frame.
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      DISPATCH();
    }

    CASE_CODE(CLASS):
    {
      createClass(vm, READ_BYTE(), NULL);
      if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
      DISPATCH();
    }

    CASE_CODE(FOREIGN_CLASS):
    {
      createClass(vm, -1, fn->module);
      if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
      DISPATCH();
    }

    CASE_CODE(METHOD_INSTANCE):
    CASE_CODE(METHOD_STATIC):
    {
      uint16_t symbol = READ_SHORT();
      ObjClass* classObj = AS_CLASS(PEEK());
      Value method = PEEK2();
      bindMethod(vm, instruction, symbol, fn->module, classObj, method);
      if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
      DROP();
      DROP();
      DISPATCH();
    }
    
    CASE_CODE(END_MODULE):
    {
      vm->lastModule = fn->module;
      PUSH(NULL_VAL);
      DISPATCH();
    }
    
    CASE_CODE(IMPORT_MODULE):
    {
      // Make a slot on the stack for the module's fiber to place the return
      // value. It will be popped after this fiber is resumed. Store the
      // imported module's closure in the slot in case a GC happens when
      // invoking the closure.
      PUSH(importModule(vm, fn->constants.data[READ_SHORT()]));
      if (canary_thread_has_error(fiber)) RUNTIME_ERROR();
      
      // If we get a closure, call it to execute the module body.
      if (IS_CLOSURE(PEEK()))
      {
        STORE_FRAME();
        ObjClosure* closure = AS_CLOSURE(PEEK());
        canary_thread_call_function(fiber, closure, 1);
        LOAD_FRAME();
      }
      else
      {
        // The module has already been loaded. Remember it so we can import
        // variables from it if needed.
        vm->lastModule = AS_MODULE(PEEK());
      }

      DISPATCH();
    }
    
    CASE_CODE(IMPORT_VARIABLE):
    {
      Value variable = fn->constants.data[READ_SHORT()];
      ASSERT(vm->lastModule != NULL, "Should have already imported module.");
      Value result = getModuleVariable(vm, vm->lastModule, variable);
      if (canary_thread_has_error(fiber)) RUNTIME_ERROR();

      PUSH(result);
      DISPATCH();
    }

    CASE_CODE(END):
      // A CODE_END should always be preceded by a CODE_RETURN. If we get here,
      // the compiler generated wrong code.
      UNREACHABLE();
  }

  // We should only exit this function from an explicit return from CODE_RETURN
  // or a runtime error.
  UNREACHABLE();
  return WREN_RESULT_RUNTIME_ERROR;

  #undef READ_BYTE
  #undef READ_SHORT
}
