#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

void cpu_init(bool SUPER_CHIP, uint64_t Hz);

void cpu_stop(void);

#endif // CPU_H