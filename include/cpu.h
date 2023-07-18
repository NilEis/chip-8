#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void cpu_init(bool SUPER_CHIP, uint64_t Hz);

uint16_t cpu_load(const char *path);

void cpu_disassemble(int start, int stop);

uint64_t cpu_tick(uint64_t *execution_time);

bool cpu_stopped(void);

void cpu_stop(void);

#endif // CPU_H