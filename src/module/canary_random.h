
#ifndef CANARY_RANDOM_H
#define CANARY_RANDOM_H

#include "wren.h"

void randomAllocate(WrenVM* vm);

void randomSeed0(WrenVM* vm);
void randomSeed1(WrenVM* vm);
void randomSeed16(WrenVM* vm);

void randomFloat(WrenVM* vm);
void randomInt0(WrenVM* vm);

#endif // CANARY_RANDOM_H
