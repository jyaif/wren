#ifndef io_h
#define io_h

#include "wren.h"

// Frees up any pending resources in use by the IO module.
//
// In particular, this closes down the stdin stream.
void ioShutdown(WrenVM* vm);

void directoryList(WrenVM* vm);
void fileAllocate(WrenVM* vm);
void fileFinalize(void* data);
void fileDelete(WrenVM* vm);
void fileOpen(WrenVM* vm);
void fileSizePath(WrenVM* vm);
void fileClose(WrenVM* vm);
void fileDescriptor(WrenVM* vm);
void fileReadBytes(WrenVM* vm);
void fileRealPath(WrenVM* vm);
void fileSize(WrenVM* vm);
void fileStat(WrenVM* vm);
void fileWriteBytes(WrenVM* vm);
void statPath(WrenVM* vm);
void statBlockCount(WrenVM* vm);
void statBlockSize(WrenVM* vm);
void statDevice(WrenVM* vm);
void statGroup(WrenVM* vm);
void statInode(WrenVM* vm);
void statLinkCount(WrenVM* vm);
void statMode(WrenVM* vm);
void statSize(WrenVM* vm);
void statSpecialDevice(WrenVM* vm);
void statUser(WrenVM* vm);
void statIsDirectory(WrenVM* vm);
void statIsFile(WrenVM* vm);
void stdinIsRaw(WrenVM* vm);
void stdinIsRawSet(WrenVM* vm);
void stdinIsTerminal(WrenVM* vm);
void stdinReadStart(WrenVM* vm);
void stdinReadStop(WrenVM* vm);
void stdoutFlush(WrenVM* vm);

#endif
