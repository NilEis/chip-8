#include "cpu.h"
#include "SDL3/SDL.h"
#include "defines.h"
#include "audio/beep.h"
#include "video/screen.h"
#include <pthread.h>
#include <stdio.h>
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
    screen_t screen;
    uint16_t pc;
    uint16_t i;
    cpu_timer_t delay_timer;
    struct
    {
        bool on;
        cpu_timer_t timer;
    } sound_timer;
    uint8_t regs[16];
    bool should_stop;
    bool screen_running;
    bool delay_running;
    bool sound_running;
    struct
    {
        uint16_t sp;
        uint16_t mem[64];
    } stack;
    uint8_t mem[CHIP_MEM_SIZE];
} chip_8 = {0};

static void *window_thread(void *args);
static void *cpu_delay_timer(void *args);
static void *cpu_sound_timer(void *args);
static uint64_t get_ns(void);

void cpu_init(bool SUPER_CHIP, uint64_t Hz)
{
    chip_8.ns_per_tick = 1000000000 / Hz;

    printf("ns per tick: %" PRIu64 "\n", chip_8.ns_per_tick);

    beep_init();
    beep_set_volume(0.75);
    beep_set_frequency(G_SHARP_4);

    screen_init(SUPER_CHIP, 600, 400);

    pthread_mutex_init(&(chip_8.delay_timer.lock), NULL);
    pthread_mutex_init(&(chip_8.sound_timer.timer.lock), NULL);

    if (SUPER_CHIP)
    {
        chip_8.screen.data = calloc(4 * 2, sizeof(chip_8.screen.data[0]));
        chip_8.screen.width = CHIP_SUPER_DISPLAY_WIDTH;
        chip_8.screen.height = CHIP_SUPER_DISPLAY_HEIGHT;
    }
    else
    {
        chip_8.screen.data = calloc(2 * 1, sizeof(chip_8.screen.data[0]));
        chip_8.screen.width = CHIP_DISPLAY_WIDTH;
        chip_8.screen.height = CHIP_DISPLAY_HEIGHT;
    }

    memset(chip_8.mem, 0, sizeof(chip_8.mem));
    memset(&(chip_8.stack), 0, sizeof(chip_8.stack));
    memset(&(chip_8.regs), 0, sizeof(chip_8.regs));

    chip_8.should_stop = false;
    chip_8.screen_running = true;
    chip_8.delay_running = true;
    chip_8.sound_running = true;

    pthread_create(&(chip_8.screen.thread), NULL, window_thread, NULL);
    pthread_create(&(chip_8.delay_timer.thread), NULL, cpu_delay_timer, NULL);
    pthread_create(&(chip_8.sound_timer.timer.thread), NULL, cpu_sound_timer, NULL);
}

uint64_t cpu_tick(void)
{
    static uint64_t error = 0;

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

    volatile uint64_t st = get_ns() - start_time;

    while (st < chip_8.ns_per_tick)
    {
        st = (get_ns() - start_time) + error;
    }
    error = st - chip_8.ns_per_tick;
    return (st - error) * (!chip_8.should_stop);
}

static void *window_thread(void *args)
{
    SDL_Event event;
    while (chip_8.screen_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                chip_8.should_stop = true;
            }
        }
        if (chip_8.screen.updated)
        {
            pthread_mutex_lock(&(chip_8.screen.lock));

            screen_update(&(chip_8.screen));

            chip_8.screen.updated = false;
            pthread_mutex_unlock(&(chip_8.screen.lock));
        }
    }
    screen_stop();
}

static void *cpu_delay_timer(void *args)
{
    while (chip_8.delay_running)
    {
        pthread_mutex_lock(&(chip_8.delay_timer.lock));
        chip_8.delay_timer.t = (chip_8.delay_timer.t != 0) * (chip_8.delay_timer.t - 1);
        pthread_mutex_unlock(&(chip_8.delay_timer.lock));
        SDL_Delay(16);
    }
    return NULL;
}

static void *cpu_sound_timer(void *args)
{
    while (chip_8.sound_running)
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
    return NULL;
}

void cpu_stop(void)
{
    chip_8.screen_running = false;
    pthread_join(chip_8.screen.thread, NULL);

    chip_8.delay_running = false;
    pthread_join(chip_8.delay_timer.thread, NULL);

    chip_8.sound_running = false;
    pthread_join(chip_8.sound_timer.timer.thread, NULL);

    pthread_mutex_destroy(&(chip_8.screen.lock));
    pthread_mutex_destroy(&(chip_8.delay_timer.lock));
    pthread_mutex_destroy(&(chip_8.sound_timer.timer.lock));
    free(chip_8.screen.data);
    SDL_Quit();
}

#define SEC_TO_NS(sec) ((sec)*1000000000)
static uint64_t get_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = SEC_TO_NS((uint64_t)ts.tv_sec) + (uint64_t)ts.tv_nsec;
    return ns;
}