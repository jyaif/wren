
#ifndef CANARY_RANDOM_H
#define CANARY_RANDOM_H

#include "wren.h"

void randomAllocate(canary_context_t *context);

void randomSeed0(canary_context_t *context);
void randomSeed1(canary_context_t *context);
void randomSeed16(canary_context_t *context);

void randomFloat(canary_context_t *context);
void randomInt0(canary_context_t *context);

#endif // CANARY_RANDOM_H
