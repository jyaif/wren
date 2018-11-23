
#include "wren_primitive.h"

#include <math.h>
#include <stdlib.h>

// Validates that [value] is an integer within `[0, count)`. Also allows
// negative indices which map backwards from the end. Returns the valid positive
// index value. If invalid, reports an error and returns `UINT32_MAX`.
static uint32_t validateIndexValue(canary_thread_t *thread, uint32_t count,
                                   double value, const char* argName)
{
  if (!validateIntValue(thread, value, argName)) return UINT32_MAX;
  
  // Negative indices count from the end.
  if (value < 0) value = count + value;
  
  // Check bounds.
  if (value >= 0 && value < count) return (uint32_t)value;
  
  canary_thread_set_error_str_format(thread, "$ out of bounds.", argName);
  return UINT32_MAX;
}

bool validateFn(canary_thread_t *thread, Value arg, const char* argName)
{
  if (IS_CLOSURE(arg)) return true;
  
  canary_thread_set_error_str_format(thread, "$ must be a function.", argName);
  return false;
}

bool validateNum(canary_thread_t *thread, Value arg, const char* argName)
{
  if (IS_NUM(arg)) return true;
  RETURN_ERROR_FMT("$ must be a number.", argName);
}

bool validateIntValue(canary_thread_t *thread, double value,
                      const char* argName)
{
  if (trunc(value) == value) return true;
  RETURN_ERROR_FMT("$ must be an integer.", argName);
}

bool validateInt(canary_thread_t *thread, Value arg, const char* argName)
{
  // Make sure it's a number first.
  if (!validateNum(thread, arg, argName)) return false;
  return validateIntValue(thread, AS_NUM(arg), argName);
}

bool validateKey(canary_thread_t *thread, Value arg)
{
  if (IS_BOOL(arg) || IS_CLASS(arg) || IS_NULL(arg) ||
      IS_NUM(arg) || IS_RANGE(arg) || IS_STRING(arg))
  {
    return true;
  }

  RETURN_ERROR("Key must be a value type.");
}

uint32_t validateIndex(canary_thread_t *thread, Value arg, uint32_t count,
                       const char* argName)
{
  if (!validateNum(thread, arg, argName)) return UINT32_MAX;
  return validateIndexValue(thread, count, AS_NUM(arg), argName);
}

bool validateString(canary_thread_t *thread, Value arg, const char* argName)
{
  if (IS_STRING(arg)) return true;
  RETURN_ERROR_FMT("$ must be a string.", argName);
}

uint32_t calculateRange(canary_thread_t *thread, ObjRange* range,
                        uint32_t* length, int* step)
{
  *step = 0;

  // Edge case: an empty range is allowed at the end of a sequence. This way,
  // list[0..-1] and list[0...list.count] can be used to copy a list even when
  // empty.
  if (range->from == *length &&
      range->to == (range->isInclusive ? -1.0 : (double)*length))
  {
    *length = 0;
    return 0;
  }

  uint32_t from = validateIndexValue(thread, *length, range->from,
                                     "Range start");
  if (from == UINT32_MAX) return UINT32_MAX;

  // Bounds check the end manually to handle exclusive ranges.
  double value = range->to;
  if (!validateIntValue(thread, value, "Range end")) return UINT32_MAX;

  // Negative indices count from the end.
  if (value < 0) value = *length + value;

  // Convert the exclusive range to an inclusive one.
  if (!range->isInclusive)
  {
    // An exclusive range with the same start and end points is empty.
    if (value == from)
    {
      *length = 0;
      return from;
    }

    // Shift the endpoint to make it inclusive, handling both increasing and
    // decreasing ranges.
    value += value >= from ? -1 : 1;
  }

  // Check bounds.
  if (value < 0 || value >= *length)
  {
    canary_thread_set_error_str(thread, "Range end out of bounds.");
    return UINT32_MAX;
  }

  uint32_t to = (uint32_t)value;
  *length = abs((int)(from - to)) + 1;
  *step = from < to ? 1 : -1;
  return from;
}
