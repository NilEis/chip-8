#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "defines.h"

typedef struct
{
    bool updated;
    pthread_t thread;
    pthread_mutex_t lock;
    uint8_t data[CHIP_SUPER_DISPLAY_WIDTH * CHIP_SUPER_DISPLAY_HEIGHT];
    uint8_t width;
    uint8_t height;
} screen_t;

void screen_init(bool super, int width, int height);
void screen_draw(void);
void screen_update(screen_t *screen);
void screen_stop(void);

#endif // SCREEN_H