#include <stdio.h>
#include <inttypes.h>
#include "SDL3/SDL.h"
#include "cpu.h"

#define HZ 500

int main(int argc, char const **argv)
{
    printf("Hallo\n");
    cpu_init(false, HZ);
    uint64_t st = 0;
    for (int i = 0; i < 4 * HZ; i++)
    {
        st += cpu_tick();
    }
    printf("Total execution time: %" PRIu64 " ms\n", (st/1000000));
    cpu_stop();
    return 0;
}