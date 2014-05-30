#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

void init_rand(uint32_t x);
uint32_t rand_cmwc(void);

#endif // RANDOM_H
