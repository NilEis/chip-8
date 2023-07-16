#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

void cpu_init(bool SUPER_CHIP, uint64_t Hz);

uint64_t cpu_tick(void);

void cpu_stop(void);

#endif // CPU_H