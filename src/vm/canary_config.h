
#ifndef CANARY_CONFIG_H
#define CANARY_CONFIG_H


//#define CANARY_DEBUG


// Define the number of default number of frames available for a created thread.
#define CANARY_THREAD_DEFAULT_FRAME_COUNT_DEFAULT 8

//#define CANARY_THREAD_DEFAULT_FRAME_COUNT 8


// Define the number of default number of slots available for a created thread.
#define CANARY_THREAD_DEFAULT_SLOT_COUNT_DEFAULT 64

//#define CANARY_THREAD_DEFAULT_SLOT_COUNT 64


// The portable interpretor uses a portable switch dispatch.
#define CANARY_THREAD_INTERPRETER_FLAVOR_SWITCH 1

// The the interpreter loop uses computed gotos. See this for more:
// http://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/Labels-as-Values.html
// Enabling this speeds up the main dispatch loop a bit, but requires compiler
// support.
#define CANARY_THREAD_INTERPRETER_FLAVOR_GOTO   2

#define CANARY_THREAD_INTERPRETER_FLAVOR_DEFAULT                               \
    CANARY_THREAD_INTERPRETER_FLAVOR_GOTO

//#define CANARY_THREAD_INTERPRETER_FLAVOR CANARY_THREAD_INTERPRETER_FLAVOR_GOTO


// These flags let you control some details of the interpreter's implementation.
// Usually they trade-off a bit of portability for speed. They default to the
// most efficient behavior.

// If true, then Wren uses a NaN-tagged double for its core value
// representation. Otherwise, it uses a larger more conventional struct. The
// former is significantly faster and more compact. The latter is useful for
// debugging and may be more portable.
//
// Defaults to on.
#define CANARY_VALUE_FLAVOUR_NANTAGGING   1
#define CANARY_VALUE_FLAVOUR_UNIONTAGGING 2
#define CANARY_VALUE_FLAVOUR_DEFAULT      CANARY_VALUE_FLAVOUR_NANTAGGING

//#define CANARY_VALUE_FLAVOUR CANARY_VALUE_FLAVOUR_NANTAGGING


// End of configuration

#if !(CANARY_DEBUG) &&                                                         \
    defined(DEBUG)
  #define CANARY_DEBUG
#endif // CANARY_DEBUG

#ifndef CANARY_THREAD_DEFAULT_FRAME_COUNT
  #define CANARY_THREAD_DEFAULT_FRAME_COUNT                                    \
      CANARY_THREAD_DEFAULT_FRAME_COUNT_DEFAULT
#endif // CANARY_THREAD_DEFAULT_FRAME_COUNT

#ifndef CANARY_THREAD_DEFAULT_SLOT_COUNT
  #define CANARY_THREAD_DEFAULT_SLOT_COUNT                                     \
      CANARY_THREAD_DEFAULT_SLOT_COUNT_DEFAULT
#endif // CANARY_THREAD_DEFAULT_SLOT_COUNT

#ifndef CANARY_THREAD_INTERPRETER_FLAVOR
  #define CANARY_THREAD_INTERPRETER_FLAVOR                                     \
      CANARY_THREAD_INTERPRETER_FLAVOR_DEFAULT
#endif // CANARY_THREAD_INTERPRETER_FLAVOR

#ifndef CANARY_VALUE_FLAVOUR
  #define CANARY_VALUE_FLAVOUR                                                 \
      CANARY_VALUE_FLAVOUR_DEFAULT
#endif // CANARY_VALUE_FLAVOUR

#endif // CANARY_CONFIG_H
