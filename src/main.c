#include <stdio.h>
#include "SDL3/SDL.h"
#include "cpu.h"

#define HZ 1000

int main(int argc, char const **argv)
{
    printf("Hallo\n");
    cpu_init(false, HZ);
    for (int i = 0; i < 4 * HZ; i++)
    {
        cpu_tick();
    }
    cpu_stop();
    return 0;
}