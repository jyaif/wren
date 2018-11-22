#ifndef wren_common_h
#define wren_common_h

#include "canary_config.h"

// This header contains macros and defines used across the entire Wren
// implementation. In particular, it contains "configuration" defines that
// control how Wren works. Some of these are only used while hacking on Wren
// itself.
//
// This header is *not* intended to be included by code outside of Wren itself.

// Wren pervasively uses the C99 integer types (uint16_t, etc.) along with some
// of the associated limit constants (UINT32_MAX, etc.). The constants are not
// part of standard C++, so aren't included by default by C++ compilers when you
// include <stdint> unless __STDC_LIMIT_MACROS is defined.
#define __STDC_LIMIT_MACROS
#include <stdint.h>

// These flags let you control some details of the interpreter's implementation.
// Usually they trade-off a bit of portability for speed. They default to the
// most efficient behavior.

// If true, then Wren uses a NaN-tagged double for its core value
// representation. Otherwise, it uses a larger more conventional struct. The
// former is significantly faster and more compact. The latter is useful for
// debugging and may be more portable.
//
// Defaults to on.
#define WREN_NAN_TAGGING                                                       \
  (CANARY_VALUE_FLAVOUR == CANARY_VALUE_FLAVOUR_NANTAGGING)

// These flags are useful for debugging and hacking on Wren itself. They are not
// intended to be used for production code. They default to off.

// Set this to true to stress test the GC. It will perform a collection before
// every allocation. This is useful to ensure that memory is always correctly
// reachable.
#define WREN_DEBUG_GC_STRESS 0

// Set this to true to log memory operations as they occur.
#define WREN_DEBUG_TRACE_MEMORY 0

// Set this to true to log garbage collections as they occur.
#define WREN_DEBUG_TRACE_GC 0

// Set this to true to print out the compiled bytecode of each function.
#define WREN_DEBUG_DUMP_COMPILED_CODE 0

// Set this to trace each instruction as it's executed.
#define WREN_DEBUG_TRACE_INSTRUCTIONS 0

// The maximum number of module-level variables that may be defined at one time.
// This limitation comes from the 16 bits used for the arguments to
// `CODE_LOAD_MODULE_VAR` and `CODE_STORE_MODULE_VAR`.
#define MAX_MODULE_VARS 65536

// The maximum number of arguments that can be passed to a method. Note that
// this limitation is hardcoded in other places in the VM, in particular, the
// `CODE_CALL_XX` instructions assume a certain maximum number.
#define MAX_PARAMETERS 16

// The maximum name of a method, not including the signature. This is an
// arbitrary but enforced maximum just so we know how long the method name
// strings need to be in the parser.
#define MAX_METHOD_NAME 64

// The maximum length of a method signature. Signatures look like:
//
//     foo        // Getter.
//     foo()      // No-argument method.
//     foo(_)     // One-argument method.
//     foo(_,_)   // Two-argument method.
//     init foo() // Constructor initializer.
//
// The maximum signature length takes into account the longest method name, the
// maximum number of parameters with separators between them, "init ", and "()".
#define MAX_METHOD_SIGNATURE (MAX_METHOD_NAME + (MAX_PARAMETERS * 2) + 6)

// The maximum length of an identifier. The only real reason for this limitation
// is so that error messages mentioning variables can be stack allocated.
#define MAX_VARIABLE_NAME 64

// The maximum number of fields a class can have, including inherited fields.
// This is explicit in the bytecode since `CODE_CLASS` and `CODE_SUBCLASS` take
// a single byte for the number of fields. Note that it's 255 and not 256
// because creating a class takes the *number* of fields, not the *highest
// field index*.
#define MAX_FIELDS 255

// Use the VM's allocator to allocate an object of [type].
#define ALLOCATE(vm, type) \
    ((type*)canary_vm_malloc(vm, sizeof(type)))

// Use the VM's allocator to allocate an object of [mainType] containing a
// flexible array of [count] objects of [arrayType].
#define ALLOCATE_FLEX(vm, mainType, arrayType, count) \
    ((mainType*)canary_vm_malloc(vm, sizeof(mainType) +                        \
                                     sizeof(arrayType) * (count)))

// Use the VM's allocator to allocate an array of [count] elements of [type].
#define ALLOCATE_ARRAY(vm, type, count) \
    ((type*)canary_vm_malloc(vm, sizeof(type) * (count)))

// Use the VM's allocator to free the previously allocated memory at [pointer].
#define DEALLOCATE(vm, pointer) canary_vm_free(vm, pointer)

// This is used to clearly mark flexible-sized arrays that appear at the end of
// some dynamically-allocated structs, known as the "struct hack".
#if __STDC_VERSION__ >= 199901L
  // In C99, a flexible array member is just "[]".
  #define FLEXIBLE_ARRAY
#else
  // Elsewhere, use a zero-sized array. It's technically undefined behavior,
  // but works reliably in most known compilers.
  #define FLEXIBLE_ARRAY 0
#endif

#define ASSERT      CANARY_ASSERT
#define UNREACHABLE CANARY_UNREACHABLE

#endif
