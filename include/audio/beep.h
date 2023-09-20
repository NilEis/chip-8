#ifndef BEEP_H
#define BEEP_H

#include "audio/notes.h"
#ifndef __EMSCRIPTEN__

void beep_init(void);

extern void (*beep)(void);

void beep_stop();

void beep_set_frequency(double frequency);

void beep_cleanup(void);

void beep_set_volume(double volume);
#endif

#endif // BEEP_H