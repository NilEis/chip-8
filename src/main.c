#include <stdio.h>
#include "SDL3/SDL.h"
#include "audio/beep.h"
#include "audio/notes.h"

int main(int argc, char const **argv)
{
    printf("Hallo\n");
    set_volume(10.0);
    set_frequency(notes_arr[0]);
    beep();
    for (int i = 0; i < notes_arr_len; i++)
    {
        SDL_Delay(64);
        set_frequency(notes_arr[i]);
    }
    for (int i = notes_arr_len - 1; i != 0; i--)
    {
        SDL_Delay(64);
        set_frequency(notes_arr[i]);
    }
    beep_stop();
    return 0;
}