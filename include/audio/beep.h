#ifndef BEEP_H
#define BEEP_H

extern void (*beep)(void);

void beep_stop();

void set_frequency(double frequency);

void set_volume(double volume);

#endif // BEEP_H