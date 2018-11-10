
#include "canary_global.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
canary_abort(const char *reason, ...) {
  va_list ap;
  
  va_start(ap, reason);
  vfprintf(stderr, reason, ap);
  va_end(ap);
  abort();
}
