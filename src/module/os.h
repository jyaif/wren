#ifndef process_h
#define process_h

#include "wren.h"

// Stores the command line arguments passed to the CLI.
void osSetArguments(int argc, const char* argv[]);

void platformIsPosix(canary_context_t *context);
void platformName(canary_context_t *context);
void processAllArguments(canary_context_t *context);

#endif
