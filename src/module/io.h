#ifndef io_h
#define io_h

#include "wren.h"

// Frees up any pending resources in use by the IO module.
//
// In particular, this closes down the stdin stream.
void ioShutdown(WrenVM* vm);

void directoryList(canary_context_t *context);
void fileAllocate(canary_context_t *context);
void fileFinalize(void* data);
void fileDelete(canary_context_t *context);
void fileOpen(canary_context_t *context);
void fileSizePath(canary_context_t *context);
void fileClose(canary_context_t *context);
void fileDescriptor(canary_context_t *context);
void fileReadBytes(canary_context_t *context);
void fileRealPath(canary_context_t *context);
void fileSize(canary_context_t *context);
void fileStat(canary_context_t *context);
void fileWriteBytes(canary_context_t *context);
void statPath(canary_context_t *context);
void statBlockCount(canary_context_t *context);
void statBlockSize(canary_context_t *context);
void statDevice(canary_context_t *context);
void statGroup(canary_context_t *context);
void statInode(canary_context_t *context);
void statLinkCount(canary_context_t *context);
void statMode(canary_context_t *context);
void statSize(canary_context_t *context);
void statSpecialDevice(canary_context_t *context);
void statUser(canary_context_t *context);
void statIsDirectory(canary_context_t *context);
void statIsFile(canary_context_t *context);
void stdinIsRaw(canary_context_t *context);
void stdinIsRawSet(canary_context_t *context);
void stdinIsTerminal(canary_context_t *context);
void stdinReadStart(canary_context_t *context);
void stdinReadStop(canary_context_t *context);
void stdoutFlush(canary_context_t *context);

#endif
