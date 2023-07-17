#include <stdio.h>
#include <inttypes.h>
#include "SDL3/SDL.h"
#include "cpu.h"

#define HZ 1000

int main(int argc, char const **argv)
{
    printf("Hallo\n");
    cpu_init(false, HZ);
    uint64_t st = 0;
    uint64_t ret = 1;
    for (int i = 0; i < 1000 && ret != 0; i++)
    {
        ret = cpu_tick();
        st += ret;
    }
    printf("Total execution time: %" PRIu64 " ms\n", (st / 1000000));
    cpu_stop();
    return 0;
}