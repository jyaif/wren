#ifndef wren_value_h
#define wren_value_h

#include "canary.h"

#include "wren_common.h"
#include "wren_utils.h"

// This defines the built-in types and their core representations in memory.
// Since Wren is dynamically typed, any variable can hold a value of any type,
// and the type can change at runtime. Implementing this efficiently is
// critical for performance.
//
// The main type exposed by this is [Value]. A C variable of that type is a
// storage location that can hold any Wren value. The stack, module variables,
// and instance fields are all implemented in C as variables of type Value.
//
// The built-in types for booleans, numbers, and null are unboxed: their value
// is stored directly in the Value, and copying a Value copies the value. Other
// types--classes, instances of classes, functions, lists, and strings--are all
// reference types. They are stored on the heap and the Value just stores a
// pointer to it. Copying the Value copies a reference to the same object. The
// Wren implementation calls these "Obj", or objects, though to a user, all
// values are objects.
//
// There is also a special singleton value "undefined". It is used internally
// but never appears as a real value to a user. It has two uses:
//
// - It is used to identify module variables that have been implicitly declared
//   by use in a forward reference but not yet explicitly declared. These only
//   exist during compilation and do not appear at runtime.
//
// - It is used to represent unused map entries in an ObjMap.
//
// There are two supported Value representations. The main one uses a technique
// called "NaN tagging" (explained in detail below) to store a number, any of
// the value types, or a pointer, all inside one double-precision floating
// point number. A larger, slower, Value type that uses a struct to store these
// is also supported, and is useful for debugging the VM.
//
// The representation is controlled by the `WREN_NAN_TAGGING` define. If that's
// defined, Nan tagging is used.

// These macros cast a Value to one of the specific object types. These do *not*
// perform any validation, so must only be used after the Value has been
// ensured to be the right type.
#define AS_BOOL(value)      (canary_value_to_bool(value))       // boolean
#define AS_NUM(value)       (canary_value_to_double(value))     // double
#define AS_OBJ(value)       ((Obj*)canary_value_to_user_data(value))
                                                                // Any Obj___*

#define AS_CLASS(value)     ((ObjClass*)AS_OBJ(value))          // ObjClass*
#define AS_CLOSURE(value)   ((ObjClosure*)AS_OBJ(value))        // ObjClosure*
#define AS_FIBER(v)         ((ObjFiber*)AS_OBJ(v))              // ObjFiber*
#define AS_FN(value)        ((ObjFn*)AS_OBJ(value))             // ObjFn*
#define AS_FOREIGN(v)       ((ObjForeign*)AS_OBJ(v))            // ObjForeign*
#define AS_INSTANCE(value)  ((ObjInstance*)AS_OBJ(value))       // ObjInstance*
#define AS_LIST(value)      ((ObjList*)AS_OBJ(value))           // ObjList*
#define AS_MAP(value)       ((ObjMap*)AS_OBJ(value))            // ObjMap*
#define AS_MODULE(value)    ((ObjModule*)AS_OBJ(value))         // ObjModule*
#define AS_RANGE(v)         ((ObjRange*)AS_OBJ(v))              // ObjRange*
#define AS_STRING(v)        ((ObjString*)AS_OBJ(v))             // ObjString*
#define AS_CSTRING(v)       (AS_STRING(v)->value)               // const char*

// These macros promote a primitive C value to a full Wren Value. There are
// more defined below that are specific to the Nan tagged or other
// representation.

// Singleton values.
#define FALSE_VAL     (canary_value_false())
#define NULL_VAL      (canary_value_null())
#define TRUE_VAL      (canary_value_true())
#define UNDEFINED_VAL (canary_value_undefined())

#define BOOL_VAL(boolean) (canary_value_from_bool(boolean))     // boolean
#define NUM_VAL(num)      (canary_value_from_double(num))       // double
#define OBJ_VAL(obj)      (canary_value_from_user_data((Obj*)(obj)))
                                                                // Any Obj___*

// These perform type tests on a Value, returning `true` if the Value is of the
// given type.

// Singleton values.
#define IS_FALSE(value)     (canary_value_is_false(value))
#define IS_NULL(value)      (canary_value_is_null(value))
#define IS_TRUE(value)      (canary_value_is_true(value))
#define IS_UNDEFINED(value) (canary_value_is_undefined(value))

#define IS_BOOL(value) (canary_value_is_bool(value))            // Bool
#define IS_NUM(value)  (canary_value_is_double(value))          // double
#define IS_OBJ(value)  (canary_value_is_user_data(value))       // Any Obj___*

#define IS_CLASS(value) (wrenIsObjType(value, OBJ_CLASS))       // ObjClass
#define IS_CLOSURE(value) (wrenIsObjType(value, OBJ_CLOSURE))   // ObjClosure
#define IS_FIBER(value) (wrenIsObjType(value, OBJ_FIBER))       // ObjFiber
#define IS_FN(value) (wrenIsObjType(value, OBJ_FN))             // ObjFn
#define IS_FOREIGN(value) (wrenIsObjType(value, OBJ_FOREIGN))   // ObjForeign
#define IS_INSTANCE(value) (wrenIsObjType(value, OBJ_INSTANCE)) // ObjInstance
#define IS_LIST(value) (wrenIsObjType(value, OBJ_LIST))         // ObjList
#define IS_RANGE(value) (wrenIsObjType(value, OBJ_RANGE))       // ObjRange
#define IS_STRING(value) (wrenIsObjType(value, OBJ_STRING))     // ObjString

// Creates a new string object from [text], which should be a bare C string
// literal. This determines the length of the string automatically at compile
// time based on the size of the character array (-1 for the terminating '\0').
#define CONST_STRING(vm, text) wrenNewStringLength((vm), (text), sizeof(text) - 1)

// Identifies which specific type a heap-allocated object is.
#define OBJ_CLASS    CANARY_TYPE_CLASS
#define OBJ_CLOSURE  CANARY_TYPE_CLOSURE
#define OBJ_FIBER    CANARY_TYPE_FIBER
#define OBJ_FN       CANARY_TYPE_FN
#define OBJ_FOREIGN  CANARY_TYPE_FOREIGN
#define OBJ_INSTANCE CANARY_TYPE_INSTANCE
#define OBJ_LIST     CANARY_TYPE_LIST
#define OBJ_MAP      CANARY_TYPE_MAP
#define OBJ_MODULE   CANARY_TYPE_MODULE
#define OBJ_RANGE    CANARY_TYPE_RANGE
#define OBJ_STRING   CANARY_TYPE_STRING
#define OBJ_UPVALUE  CANARY_TYPE_UPVALUE
typedef canary_type_t ObjType;

typedef struct sObjClass ObjClass;

// Base struct for all heap-allocated objects.
typedef struct sObj Obj;
struct sObj
{
  ObjType type;
  bool isDark;

  // The object's class.
  ObjClass* classObj;

  // The next object in the linked list of all currently allocated objects.
  struct sObj* next;
};

#include "canary_value.h"

typedef canary_value_t Value;
typedef canary_type_t ValueType;

DECLARE_BUFFER(Value, Value);

// A heap-allocated string object.
struct sObjString
{
  Obj obj;

  // Number of bytes in the string, not including the null terminator.
  uint32_t length;

  // The hash value of the string's contents.
  uint32_t hash;

  // Inline array of the string's bytes followed by a null terminator.
  char value[FLEXIBLE_ARRAY];
};

// The dynamically allocated data structure for a variable that has been used
// by a closure. Whenever a function accesses a variable declared in an
// enclosing function, it will get to it through this.
//
// An upvalue can be either "closed" or "open". An open upvalue points directly
// to a [Value] that is still stored on the fiber's stack because the local
// variable is still in scope in the function where it's declared.
//
// When that local variable goes out of scope, the upvalue pointing to it will
// be closed. When that happens, the value gets copied off the stack into the
// upvalue itself. That way, it can have a longer lifetime than the stack
// variable.
typedef struct sObjUpvalue
{
  // The object header. Note that upvalues have this because they are garbage
  // collected, but they are not first class Wren objects.
  Obj obj;

  // Pointer to the variable this upvalue is referencing.
  Value* value;

  // If the upvalue is closed (i.e. the local variable it was pointing too has
  // been popped off the stack) then the closed-over value will be hoisted out
  // of the stack into here. [value] will then be changed to point to this.
  Value closed;

  // Open upvalues are stored in a linked list by the fiber. This points to the
  // next upvalue in that list.
  struct sObjUpvalue* next;
} ObjUpvalue;

// The type of a primitive function.
//
// Primitives are similiar to foreign functions, but have more direct access to
// VM internals. It is passed the arguments in [args]. If it returns a value,
// it places it in `args[0]` and returns `true`. If it causes a runtime error
// or modifies the running fiber, it returns `false`.
typedef bool (*Primitive)(WrenVM* vm, Value* args);

// TODO: See if it's actually a perf improvement to have this in a separate
// struct instead of in ObjFn.
// Stores debugging information for a function used for things like stack
// traces.
typedef struct
{
  // The name of the function. Heap allocated and owned by the FnDebug.
  char* name;

  // An array of line numbers. There is one element in this array for each
  // bytecode in the function's bytecode array. The value of that element is
  // the line in the source code that generated that instruction.
  IntBuffer sourceLines;
} FnDebug;

// A loaded module and the top-level variables it defines.
//
// While this is an Obj and is managed by the GC, it never appears as a
// first-class object in Wren.
typedef struct
{
  Obj obj;

  // The currently defined top-level variables.
  ValueBuffer variables;

  // Symbol table for the names of all module variables. Indexes here directly
  // correspond to entries in [variables].
  SymbolTable variableNames;

  // The name of the module.
  ObjString* name;
} ObjModule;

// A function object. It wraps and owns the bytecode and other debug information
// for a callable chunk of code.
//
// Function objects are not passed around and invoked directly. Instead, they
// are always referenced by an [ObjClosure] which is the real first-class
// representation of a function. This isn't strictly necessary if they function
// has no upvalues, but lets the rest of the VM assume all called objects will
// be closures.
typedef struct
{
  Obj obj;
  
  ByteBuffer code;
  ValueBuffer constants;
  
  // The module where this function was defined.
  ObjModule* module;

  // The maximum number of stack slots this function may use.
  int maxSlots;
  
  // The number of upvalues this function closes over.
  int numUpvalues;
  
  // The number of parameters this function expects. Used to ensure that .call
  // handles a mismatch between number of parameters and arguments. This will
  // only be set for fns, and not ObjFns that represent methods or scripts.
  int arity;
  FnDebug* debug;
} ObjFn;

// An instance of a first-class function and the environment it has closed over.
// Unlike [ObjFn], this has captured the upvalues that the function accesses.
typedef struct
{
  Obj obj;

  // The function that this closure is an instance of.
  ObjFn* fn;

  // The upvalues this function has closed over.
  ObjUpvalue* upvalues[FLEXIBLE_ARRAY];
} ObjClosure;

typedef struct
{
  // Pointer to the current (really next-to-be-executed) instruction in the
  // function's bytecode.
  uint8_t* ip;
  
  // The closure being executed.
  ObjClosure* closure;
  
  // Pointer to the first stack slot used by this call frame. This will contain
  // the receiver, followed by the function's parameters, then local variables
  // and temporaries.
  Value* stackStart;
} CallFrame;

// Tracks how this fiber has been invoked, aside from the ways that can be
// detected from the state of other fields in the fiber.
typedef enum
{
  // The fiber is being run from another fiber using a call to `try()`.
  FIBER_TRY,
  
  // The fiber was directly invoked by `runInterpreter()`. This means it's the
  // initial fiber used by a call to `wrenCall()` or `wrenInterpret()`.
  FIBER_ROOT,
  
  // The fiber is invoked some other way. If [caller] is `NULL` then the fiber
  // was invoked using `call()`. If [numFrames] is zero, then the fiber has
  // finished running and is done. If [numFrames] is one and that frame's `ip`
  // points to the first byte of code, the fiber has not been started yet.
  FIBER_OTHER,
} FiberState;

typedef struct sObjFiber
{
  Obj obj;
  
  // The vm the fiber is belonging to.
  WrenVM* vm;
  
  // The stack of value slots. This is used for holding local variables and
  // temporaries while the fiber is executing. It heap-allocated and grown as
  // needed.
  Value* stack;
  
  // A pointer to the first value of the stack frame.
  Value *stack_base;
  
  // A pointer to one past the top-most value on the stack.
  Value* stackTop;
  
  // The number of allocated slots in the stack array.
  size_t stackCapacity;
  
  // The stack of call frames. This is a dynamic array that grows as needed but
  // never shrinks.
  CallFrame* frames;
  
  // The number of frames currently in use in [frames].
  size_t numFrames;
  
  // The number of [frames] allocated.
  size_t frameCapacity;
  
  // Pointer to the first node in the linked list of open upvalues that are
  // pointing to values still on the stack. The head of the list will be the
  // upvalue closest to the top of the stack, and then the list works downwards.
  ObjUpvalue* openUpvalues;
  
  // The fiber that ran this one. If this fiber is yielded, control will resume
  // to this one. May be `NULL`.
  struct sObjFiber* caller;
  
  // If the fiber failed because of a runtime error, this will contain the
  // error object. Otherwise, it will be null.
  Value error;
  
  FiberState state;
} ObjFiber;

typedef enum
{
  // A primitive method implemented in C in the VM. Unlike foreign methods,
  // this can directly manipulate the fiber's stack.
  METHOD_PRIMITIVE,

  // A externally-defined C method.
  METHOD_FOREIGN,

  // A normal user-defined method.
  METHOD_BLOCK,
  
  // No method for the given symbol.
  METHOD_NONE
} MethodType;

typedef struct
{
  MethodType type;

  // The method function itself. The [type] determines which field of the union
  // is used.
  union
  {
    Primitive primitive;
    WrenForeignMethodFn foreign;
    ObjClosure* closure;
  } as;
} Method;

DECLARE_BUFFER(Method, Method);

struct sObjClass
{
  Obj obj;
  ObjClass* superclass;

  // The number of fields needed for an instance of this class, including all
  // of its superclass fields.
  int numFields;

  // The table of methods that are defined in or inherited by this class.
  // Methods are called by symbol, and the symbol directly maps to an index in
  // this table. This makes method calls fast at the expense of empty cells in
  // the list for methods the class doesn't support.
  //
  // You can think of it as a hash table that never has collisions but has a
  // really low load factor. Since methods are pretty small (just a type and a
  // pointer), this should be a worthwhile trade-off.
  MethodBuffer methods;

  // The name of the class.
  ObjString* name;
};

typedef struct
{
  Obj obj;
  uint8_t data[FLEXIBLE_ARRAY];
} ObjForeign;

typedef struct
{
  Obj obj;
  Value fields[FLEXIBLE_ARRAY];
} ObjInstance;

typedef struct
{
  Obj obj;

  // The elements in the list.
  ValueBuffer elements;
} ObjList;

typedef struct
{
  // The entry's key, or UNDEFINED_VAL if the entry is not in use.
  Value key;

  // The value associated with the key. If the key is UNDEFINED_VAL, this will
  // be false to indicate an open available entry or true to indicate a
  // tombstone -- an entry that was previously in use but was then deleted.
  Value value;
} MapEntry;

// A hash table mapping keys to values.
//
// We use something very simple: open addressing with linear probing. The hash
// table is an array of entries. Each entry is a key-value pair. If the key is
// the special UNDEFINED_VAL, it indicates no value is currently in that slot.
// Otherwise, it's a valid key, and the value is the value associated with it.
//
// When entries are added, the array is dynamically scaled by GROW_FACTOR to
// keep the number of filled slots under MAP_LOAD_PERCENT. Likewise, if the map
// gets empty enough, it will be resized to a smaller array. When this happens,
// all existing entries are rehashed and re-added to the new array.
//
// When an entry is removed, its slot is replaced with a "tombstone". This is an
// entry whose key is UNDEFINED_VAL and whose value is TRUE_VAL. When probing
// for a key, we will continue past tombstones, because the desired key may be
// found after them if the key that was removed was part of a prior collision.
// When the array gets resized, all tombstones are discarded.
typedef struct
{
  Obj obj;

  // The number of entries allocated.
  uint32_t capacity;

  // The number of entries in the map.
  uint32_t count;

  // Pointer to a contiguous array of [capacity] entries.
  MapEntry* entries;
} ObjMap;

typedef struct
{
  Obj obj;

  // The beginning of the range.
  double from;

  // The end of the range. May be greater or less than [from].
  double to;

  // True if [to] is included in the range.
  bool isInclusive;
} ObjRange;

// A union to let us reinterpret a double as raw bits and back.
typedef union
{
  uint64_t bits64;
  uint32_t bits32[2];
  double num;
} DoubleBits;

#include "canary_thread.h"

// Creates a new "raw" class. It has no metaclass or superclass whatsoever.
// This is only used for bootstrapping the initial Object and Class classes,
// which are a little special.
ObjClass* wrenNewSingleClass(WrenVM* vm, int numFields, ObjString* name);

// Makes [superclass] the superclass of [subclass], and causes subclass to
// inherit its methods. This should be called before any methods are defined
// on subclass.
void wrenBindSuperclass(WrenVM* vm, ObjClass* subclass, ObjClass* superclass);

// Creates a new class object as well as its associated metaclass.
ObjClass* wrenNewClass(WrenVM* vm, ObjClass* superclass, int numFields,
                       ObjString* name);

void wrenBindMethod(WrenVM* vm, ObjClass* classObj, int symbol, Method method);

// Creates a new closure object that invokes [fn]. Allocates room for its
// upvalues, but assumes outside code will populate it.
ObjClosure* wrenNewClosure(WrenVM* vm, ObjFn* fn);

// Creates a new fiber object that will invoke [closure].
ObjFiber* wrenNewFiber(WrenVM* vm, ObjClosure* closure);

static inline bool
wrenFiberHasError(ObjFiber* fiber) {
  return !IS_NULL(fiber->error);
}

static inline Value
wrenFiberGetError(ObjFiber* fiber) {
  return fiber->error;
}

static inline void
wrenFiberSetError(ObjFiber* fiber, Value error) {
  if (wrenFiberHasError(fiber)) return; // Do not clobber previous error.
  
  fiber->error = error;
}

// Adds a new [CallFrame] to [fiber] invoking [closure] whose stack starts at
// [stackStart].
static inline void wrenAppendCallFrame(ObjFiber* fiber,
                                       ObjClosure* closure, Value* stackStart)
{
  // The caller should have ensured we already have enough capacity.
  ASSERT(fiber->frameCapacity > fiber->numFrames, "No memory for call frame.");
  
  CallFrame* frame = &fiber->frames[fiber->numFrames++];
  frame->stackStart = stackStart;
  frame->closure = closure;
  frame->ip = closure->fn->code.data;
}

// Ensures [fiber]'s stack has at least [needed] slots.
void _wrenEnsureStack(ObjFiber* fiber, size_t needed);

static inline void wrenEnsureStack(ObjFiber* fiber, size_t needed)
{
  if (canary_thread_get_stack_capacity(fiber) >= needed) return;
  
  _wrenEnsureStack(fiber, needed);
}

ObjForeign* wrenNewForeign(WrenVM* vm, ObjClass* classObj, size_t size);

// Creates a new empty function. Before being used, it must have code,
// constants, etc. added to it.
ObjFn* wrenNewFunction(WrenVM* vm, ObjModule* module, int maxSlots);

void wrenFunctionBindName(WrenVM* vm, ObjFn* fn, const char* name, int length);

// Creates a new instance of the given [classObj].
Value wrenNewInstance(WrenVM* vm, ObjClass* classObj);

// Creates a new list with [numElements] elements (which are left
// uninitialized.)
ObjList* wrenNewList(WrenVM* vm, uint32_t numElements);

// Inserts [value] in [list] at [index], shifting down the other elements.
void wrenListInsert(WrenVM* vm, ObjList* list, Value value, uint32_t index);

// Removes and returns the item at [index] from [list].
Value wrenListRemoveAt(WrenVM* vm, ObjList* list, uint32_t index);

// Creates a new empty map.
ObjMap* wrenNewMap(WrenVM* vm);

// Looks up [key] in [map]. If found, returns the value. Otherwise, returns
// `UNDEFINED_VAL`.
Value wrenMapGet(ObjMap* map, Value key);

// Associates [key] with [value] in [map].
void wrenMapSet(WrenVM* vm, ObjMap* map, Value key, Value value);

void wrenMapClear(WrenVM* vm, ObjMap* map);

// Removes [key] from [map], if present. Returns the value for the key if found
// or `NULL_VAL` otherwise.
Value wrenMapRemoveKey(WrenVM* vm, ObjMap* map, Value key);

// Creates a new module.
ObjModule* wrenNewModule(WrenVM* vm, ObjString* name);

// Creates a new range from [from] to [to].
Value wrenNewRange(WrenVM* vm, double from, double to, bool isInclusive);

// Creates a new string object and copies [text] into it.
//
// [text] must be non-NULL.
Value wrenNewString(WrenVM* vm, const char* text);

// Creates a new string object of [length] and copies [text] into it.
//
// [text] may be NULL if [length] is zero.
Value wrenNewStringLength(WrenVM* vm, const char* text, size_t length);

// Creates a new string object by taking a range of characters from [source].
// The range starts at [start], contains [count] bytes, and increments by
// [step].
Value wrenNewStringFromRange(WrenVM* vm, ObjString* source, int start,
                             uint32_t count, int step);

// Produces a string representation of [value].
Value wrenNumToString(WrenVM* vm, double value);

// Creates a new formatted string from [format] and any additional arguments
// used in the format string.
//
// This is a very restricted flavor of formatting, intended only for internal
// use by the VM. Two formatting characters are supported, each of which reads
// the next argument as a certain type:
//
// $ - A C string.
// @ - A Wren string object.
Value wrenStringFormat(WrenVM* vm, const char* format, ...);

// Creates a new string containing the UTF-8 encoding of [value].
Value wrenStringFromCodePoint(WrenVM* vm, int value);

// Creates a new string containing the code point in [string] starting at byte
// [index]. If [index] points into the middle of a UTF-8 sequence, returns an
// empty string.
Value wrenStringCodePointAt(WrenVM* vm, ObjString* string, uint32_t index);

// Search for the first occurence of [needle] within [haystack] and returns its
// zero-based offset. Returns `UINT32_MAX` if [haystack] does not contain
// [needle].
uint32_t wrenStringFind(ObjString* haystack, ObjString* needle,
                        uint32_t startIndex);

// Returns true if [a] and [b] represent the same string.
static inline bool wrenStringEqualsCString(const ObjString* a,
                                           const char* b, size_t length)
{
  return a->length == length && memcmp(a->value, b, length) == 0;
}

// Creates a new open upvalue pointing to [value] on the stack.
ObjUpvalue* wrenNewUpvalue(WrenVM* vm, Value* value);

// Mark [obj] as reachable and still in use. This should only be called
// during the sweep phase of a garbage collection.
void wrenGrayObj(WrenVM* vm, Obj* obj);

// Mark [value] as reachable and still in use. This should only be called
// during the sweep phase of a garbage collection.
void wrenGrayValue(WrenVM* vm, Value value);

// Mark [values] as reachable and still in use. This should only be called
// during the sweep phase of a garbage collection.
void wrenGrayValues(WrenVM* vm, Value *values, size_t size);

// Mark the values in [buffer] as reachable and still in use. This should only
// be called during the sweep phase of a garbage collection.
void wrenGrayBuffer(WrenVM* vm, ValueBuffer* buffer);

// Processes every object in the gray stack until all reachable objects have
// been marked. After that, all objects are either white (freeable) or black
// (in use and fully traversed).
void wrenBlackenObjects(WrenVM* vm);

// Releases all memory owned by [obj], including [obj] itself.
void wrenFreeObj(WrenVM* vm, Obj* obj);

// Returns the class of [value].
//
// Unlike wrenGetClassInline in wren_vm.h, this is not inlined. Inlining helps
// performance (significantly) in some cases, but degrades it in others. The
// ones used by the implementation were chosen to give the best results in the
// benchmarks.
ObjClass* wrenGetClass(WrenVM* vm, Value value);

// Returns true if [a] and [b] are equivalent. Immutable values (null, bools,
// numbers, ranges, and strings) are equal if they have the same data. All
// other values are equal if they are identical objects.
bool wrenValuesEqual(Value a, Value b);

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool wrenIsObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
