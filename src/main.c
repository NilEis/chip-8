#include <stdio.h>
#include "SDL3/SDL.h"
#include "cpu.h"

int main(int argc, char const **argv)
{
    printf("Hallo\n");
    cpu_init(false,60);
    SDL_Delay(5000);
    cpu_stop();
    return 0;
}