#include <stdio.h>
#include <inttypes.h>
#include "SDL3/SDL.h"
#include "cpu.h"

#define HZ 1000

int main(int argc, char const **argv)
{
    cpu_init(false, HZ);
    if (argc >= 2)
    {
        uint16_t size = cpu_load(argv[1]);
        cpu_disassemble(0x200, 0x200 + size);
    }
    uint64_t st = 0;
    uint64_t theoretical_time = 0;
    while (!cpu_stopped())
    {
        uint64_t tmp = 0;
        st += cpu_tick(&tmp);
        theoretical_time += tmp;
    }
    printf("Total execution time:\nTotal time:     %" PRIu64 " ms\nInterpret time: %" PRIu64 " ms\n", (st / 1000000),(theoretical_time / 1000000));
    cpu_stop();
    return 0;
}