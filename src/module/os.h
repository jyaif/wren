#ifndef process_h
#define process_h

#include "wren.h"

// Stores the command line arguments passed to the CLI.
void osSetArguments(int argc, const char* argv[]);

void platformIsPosix(WrenVM* vm);
void platformName(WrenVM* vm);
void processAllArguments(WrenVM* vm);

#endif
