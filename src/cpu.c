#include "cpu.h"
#include "SDL3/SDL.h"
#include "defines.h"
#include "audio/beep.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

typedef struct
{
    pthread_t thread;
    pthread_mutex_t lock;
    uint8_t t;
} cpu_timer_t;

struct
{
    uint64_t ns_per_tick;
    uint8_t mem[CHIP_MEM_SIZE];
    struct
    {
        uint16_t sp;
        uint16_t mem[64];
    } stack;
    uint32_t *screen;
    uint16_t pc;
    uint16_t i;
    cpu_timer_t delay_timer;
    struct
    {
        bool on;
        cpu_timer_t timer;
    } sound_timer;
    uint8_t regs[16];
    bool running;
} chip_8 = {0};

static void *cpu_delay_timer(void *args);
static void *cpu_sound_timer(void *args);
static uint64_t get_ns(void);

void cpu_init(bool SUPER_CHIP, uint64_t Hz)
{

    chip_8.ns_per_tick = 1000000000 / Hz;

    beep_init();
    beep_set_volume(0.75);
    beep_set_frequency(G_SHARP_4);

    pthread_mutex_init(&(chip_8.delay_timer.lock), NULL);
    pthread_mutex_init(&(chip_8.sound_timer.timer.lock), NULL);

    if (SUPER_CHIP)
    {
        chip_8.screen = calloc(4 * 2, sizeof(chip_8.screen[0]));
    }
    else
    {
        chip_8.screen = calloc(2 * 1, sizeof(chip_8.screen[0]));
    }

    memset(chip_8.mem, 0, sizeof(chip_8.mem));
    memset(&(chip_8.stack), 0, sizeof(chip_8.stack));
    memset(&(chip_8.regs), 0, sizeof(chip_8.regs));

    chip_8.sound_timer.timer.t = 60;

    chip_8.running = true;

    pthread_create(&(chip_8.delay_timer.thread), NULL, cpu_delay_timer, NULL);
    pthread_create(&(chip_8.sound_timer.timer.thread), NULL, cpu_sound_timer, NULL);
}

void cpu_tick(void)
{
    uint64_t start_time = get_ns();

    uint16_t fetched = (chip_8.mem[chip_8.pc] << 8) | chip_8.mem[chip_8.pc + 1];
    chip_8.pc += 2;
    switch (fetched & 0xF000)
    {
    case 0:

        break;

    default:
        break;
    }

    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = get_ns() - start_time;
    t.tv_nsec = (t.tv_nsec < chip_8.ns_per_tick) * (chip_8.ns_per_tick - t.tv_nsec);
    uint64_t st = t.tv_nsec;
    nanosleep(&t, NULL);
}

static void *cpu_delay_timer(void *args)
{
    while (chip_8.running)
    {
        pthread_mutex_lock(&(chip_8.delay_timer.lock));
        chip_8.delay_timer.t = (chip_8.delay_timer.t != 0) * (chip_8.delay_timer.t - 1);
        pthread_mutex_unlock(&(chip_8.delay_timer.lock));
        SDL_Delay(16);
    }
}

static void *cpu_sound_timer(void *args)
{
    while (chip_8.running)
    {
        pthread_mutex_lock(&(chip_8.sound_timer.timer.lock));
        chip_8.sound_timer.timer.t = (chip_8.sound_timer.timer.t != 0) * (chip_8.sound_timer.timer.t - 1);
        if (chip_8.sound_timer.on && chip_8.sound_timer.timer.t == 0)
        {
            beep_stop();
            chip_8.sound_timer.on = false;
        }
        else if (!chip_8.sound_timer.on && chip_8.sound_timer.timer.t != 0)
        {
            beep();
            chip_8.sound_timer.on = true;
        }
        pthread_mutex_unlock(&(chip_8.sound_timer.timer.lock));
        SDL_Delay(16);
    }
    if (chip_8.sound_timer.on)
    {
        beep_stop();
    }
}

void cpu_stop(void)
{
    chip_8.running = false;
    pthread_join(chip_8.delay_timer.thread, NULL);
    pthread_join(chip_8.sound_timer.timer.thread, NULL);
    pthread_mutex_destroy(&(chip_8.delay_timer.lock));
    pthread_mutex_destroy(&(chip_8.sound_timer.timer.lock));

    if (chip_8.screen != NULL)
    {
        free(chip_8.screen);
    }
}

#define SEC_TO_NS(sec) ((sec)*1000000000)
static uint64_t get_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = SEC_TO_NS((uint64_t)ts.tv_sec) + (uint64_t)ts.tv_nsec;
    return ns;
}