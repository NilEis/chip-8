#include "cpu.h"
#include "SDL3/SDL.h"
#include "defines.h"
#include "audio/beep.h"
#include "video/screen.h"
#include <pthread.h>
#include <stdio.h>
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
    int8_t regs[16];
    bool should_stop;
    bool delay_running;
    bool sound_running;
    struct
    {
        uint16_t sp;
        uint16_t mem[64];
    } stack;
    int8_t mem[CHIP_MEM_SIZE];
} chip_8 = {0};

static void *cpu_delay_timer(void *args);
static void *cpu_sound_timer(void *args);
static void cpu_disasm(uint16_t pc);
static uint64_t get_ns(void);

void cpu_init(bool SUPER_CHIP, uint64_t Hz)
{
    chip_8.ns_per_tick = 1000000000 / Hz;
    chip_8.pc = 0x200;

    printf("ns per tick: %" PRIu64 "\n", chip_8.ns_per_tick);

    beep_init();
    beep_set_volume(0.75);
    beep_set_frequency(G_SHARP_4);

    screen_init(SUPER_CHIP, 600, 400);

    chip_8.screen.updated = true;

    pthread_mutex_init(&(chip_8.delay_timer.lock), NULL);
    pthread_mutex_init(&(chip_8.sound_timer.timer.lock), NULL);

    if (SUPER_CHIP)
    {
        chip_8.screen.width = CHIP_SUPER_DISPLAY_WIDTH;
        chip_8.screen.height = CHIP_SUPER_DISPLAY_HEIGHT;
    }
    else
    {
        chip_8.screen.width = CHIP_DISPLAY_WIDTH;
        chip_8.screen.height = CHIP_DISPLAY_HEIGHT;
    }

    memset(&(chip_8.screen.data), 0, sizeof(chip_8.screen.data));
    memset(chip_8.mem, 0, sizeof(chip_8.mem));
    memset(&(chip_8.stack), 0, sizeof(chip_8.stack));
    memset(&(chip_8.regs), 0, sizeof(chip_8.regs));

    chip_8.should_stop = false;
    chip_8.delay_running = true;
    chip_8.sound_running = true;

    pthread_create(&(chip_8.delay_timer.thread), NULL, cpu_delay_timer, NULL);
    pthread_create(&(chip_8.sound_timer.timer.thread), NULL, cpu_sound_timer, NULL);
}

uint16_t cpu_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        printf("error: Couldn't open %s\n", path);
        return 0;
    }
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    if (sizeof(chip_8.mem) < fsize + 0x200)
    {
        printf("error: program does not fit in memory\n");
        return 0;
    }
    fread(&(chip_8.mem[0x200]), fsize, 1, f);
    fclose(f);
    for (int i = 0; i < fsize; i += 2)
    {
        printf("%02X %02X\n", chip_8.mem[0x200 + i], chip_8.mem[0x201 + i]);
    }
    return fsize;
}

uint64_t cpu_tick(void)
{
    static uint64_t error = 0;

    uint64_t start_time = get_ns();

    if (chip_8.pc >= sizeof(chip_8.mem))
    {
        chip_8.should_stop = true;
        return 0;
    }

    uint16_t fetched = chip_8.mem[chip_8.pc] | chip_8.mem[chip_8.pc + 1];
    fetched <<= 8;
    fetched |= chip_8.mem[chip_8.pc + 1];
    chip_8.pc += 2;
    switch (fetched & 0xF000)
    {
    case 0x0:
        switch (fetched & 0xFF)
        {
        case 0xE0:
            memset(&(chip_8.screen.data), 0, sizeof(chip_8.screen.data));
            break;
        case 0xEE:
            chip_8.pc = chip_8.stack.mem[chip_8.stack.sp - 1];
            chip_8.stack.sp--;
            break;
        default:
            break;
        }
        break;
    case 0x1:
        chip_8.pc = fetched & 0x0FFF;
        break;
    case 0x2:
        chip_8.stack.mem[chip_8.stack.sp] = chip_8.pc;
        chip_8.stack.sp++;
        chip_8.pc = fetched & 0x0FFF;
        break;
    case 0x3:
        if (chip_8.regs[(fetched & 0x0F00) >> 8] == fetched & 0x00FF)
        {
            chip_8.pc += 2;
        }
        break;
    case 0x4:
        if (chip_8.regs[(fetched & 0x0F00) >> 8] != fetched & 0x00FF)
        {
            chip_8.pc += 2;
        }
        break;
    case 0x5:
        if (chip_8.regs[(fetched & 0x0F00) >> 8] == chip_8.regs[(fetched & 0x00F0) >> 4])
        {
            chip_8.pc += 2;
        }
        break;
    case 0x6:
        chip_8.regs[(fetched & 0x0F00) >> 8] = fetched & 0x00FF;
        break;
    case 0x7:
        break;
    case 0x8:
        uint8_t r1 = (fetched & 0x0F00) >> 8;
        uint8_t r2 = (fetched & 0x00F0) >> 4;
        switch (fetched & 0x000F)
        {
        case 0x0:
            chip_8.regs[r1] = chip_8.regs[r2];
            break;
        case 0x1:
            chip_8.regs[r1] |= chip_8.regs[r2];
            break;
        case 0x2:
            chip_8.regs[r1] &= chip_8.regs[r2];
            break;
        case 0x3:
            chip_8.regs[r1] ^= chip_8.regs[r2];
            break;
        case 0x4:
        {
            int16_t tmp = chip_8.regs[r1] + chip_8.regs[r2];
            chip_8.regs[r1] = tmp;
            chip_8.regs[0xF] = tmp > INT8_MAX;
        }
        break;
        case 0x5:
        {
            int16_t tmp = chip_8.regs[r1] - chip_8.regs[r2];
            chip_8.regs[r1] = tmp;
            chip_8.regs[0xF] = tmp < INT8_MIN;
        }
        break;
        case 0x6:
            chip_8.regs[0xF] = chip_8.regs[r1] & 0b00000001;
            chip_8.regs[r1] >>= 1;
            break;
        case 0x7:
        {
            int16_t tmp = chip_8.regs[r2] - chip_8.regs[r1];
            chip_8.regs[r1] = tmp;
            chip_8.regs[0xF] = tmp < INT8_MIN;
        }
        break;
        case 0xE:
            chip_8.regs[0xF] = chip_8.regs[r1] & 0b10000000;
            chip_8.regs[r1] <<= 1;
            break;
        default:
            break;
        }
        break;
    case 0x9:
        if (chip_8.regs[(fetched & 0x0F00) >> 8] != chip_8.regs[(fetched & 0x00F0) >> 4])
        {
            chip_8.pc += 2;
        }
        break;
    case 0xb:
        chip_8.pc = (fetched & 0x0FFF) + chip_8.regs[0];
        break;
    default:
        printf("PC: %" PRIX8 "Was? %" PRIX16 "\n", chip_8.pc, fetched);
        break;
    }

    SDL_Event event;
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

    volatile uint64_t st = get_ns() - start_time;

    while (st < chip_8.ns_per_tick)
    {
        st = (get_ns() - start_time) + error;
    }
    error = st - chip_8.ns_per_tick;
    return st - error;
}

void cpu_disassemble(int start, int stop)
{
    for (int i = start; i < stop; i++)
    {
        cpu_disasm(i);
        i += 2;
    }
}

// max len: INST [0000], 0000        // Comment
static void cpu_disasm(uint16_t pc)
{
    uint8_t code[2] = {(chip_8.mem[pc]), chip_8.mem[pc + 1]};
    printf("%02" PRIX8 "%02" PRIX8 ": ", code[0], code[1]);
    switch (code[0] >> 4)
    {
    case 0x00:
    {
        switch (code[1])
        {
        case 0xE0:
            printf("CLS                          // Clear screen\n");
            break;
        case 0xEE:
            printf("RET                          // Return from subroutine\n");
            break;
        default:
            printf("Invalid instruction: %02" PRIX8 "%02" PRIX8 "\n", code[0], code[1]);
        }
    }
    break;
    case 0x01:
        printf("JMP  0x%" PRIX16 "%02" PRIX16 "                   // Jump to address\n", code[0] & 0x0F, code[1]);
        break;
    case 0x02:
        printf("CALL 0x%" PRIX16 "%02" PRIX16 "                   // Call subroutine\n", code[0] & 0x0F, code[1]);
        break;
    case 0x03:
        printf("IFEQ V%" PRIX8 " 0x%02" PRIX16 "                 // Skip next instruction if V%" PRIX8 " EQ 0x%02" PRIX8 "\n", code[0] & 0x0F, code[1], code[0] & 0x0F, code[1]);
        break;
    case 0x04:
        printf("IFNQ V%" PRIX8 " 0x%02" PRIX16 "                 // Skip next instruction if V%" PRIX8 " NOT EQ 0x%02" PRIX8 "\n", code[0] & 0x0F, code[1], code[0] & 0x0F, code[1]);
        break;
    case 0x05:
        printf("IFEQ V%" PRIX8 " V%" PRIX8 "                   // Skip next instruction if V%" PRIX8 " EQ V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[1] >> 4);
        break;
    case 0x06:
    {
        printf("MOV  V%" PRIX8 ", 0x%" PRIX16 "                 // Set V%" PRIX8 " to 0x%" PRIX8 "\n", code[0] & 0x0F, code[1], code[0] & 0x0F, code[1]);
    }
    break;
    case 0x07:
        printf("ADD. V%" PRIX8 ", 0x%" PRIX16 "                  // Set V%" PRIX8 " to V%" PRIX8 " + 0x%" PRIX8 "\n", code[0] & 0x0F, code[1], code[0] & 0x0F, code[0] & 0x0F, code[1]);
        break;
    case 0x08:
        switch (code[1] & 0x000F)
        {
        case 0x0:
            printf("MOV  V%" PRIX8 ", V%" PRIX8 "                  // Set V%" PRIX8 " to V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x1:
            printf("OR   V%" PRIX8 ", V%" PRIX8 "                  // Set V%" PRIX8 " to V%" PRIX8 " OR V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x2:
            printf("AND  V%" PRIX8 ", V%" PRIX8 "                   // Set V%" PRIX8 " to V%" PRIX8 " AND V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x3:
            printf("XOR  V%" PRIX8 ", V%" PRIX8 "                   // Set V%" PRIX8 " to V%" PRIX8 " XOR V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x4:
            printf("ADD. V%" PRIX8 ", V%" PRIX8 "                   // Set V%" PRIX8 " to V%" PRIX8 " + V%" PRIX8 " and set VF to 1 if overflow\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x5:
            printf("SUB. V%" PRIX8 ", V%" PRIX8 "                   // Set V%" PRIX8 " to V%" PRIX8 " - V%" PRIX8 " and set VF to 1 if underflow(?)\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[0] & 0x0F, code[1] >> 4);
            break;
        case 0x6:
            printf("SHR. V%" PRIX8 "                       // Shift V%" PRIX8 " one to the right and move the dropped bit to VF\n", code[0] & 0x0F, code[0] & 0x0F);
            break;
        case 0x7:
            printf("SBB. V%" PRIX8 ", V%" PRIX8 "                   // Set V%" PRIX8 " to V%" PRIX8 " - V%" PRIX8 " and set VF to 1 if underflow(?)\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F);
            break;
        case 0xE:
            printf("SHL. V%" PRIX8 "                       // Shift V%" PRIX8 " one to the left and move the dropped bit to VF\n", code[0] & 0x0F, code[0] & 0x0F);
            break;
        default:
            break;
        }
        break;
    case 0x09:
        printf("IFEQ V%" PRIX8 " V%" PRIX8 "                   // Skip next instruction if V%" PRIX8 " EQ V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[0] & 0x0F, code[1] >> 4);
        break;
    case 0x0a:
    {
        printf("SETI 0x%" PRIX8 "%02" PRIX8 "                   // Set I to 0x%" PRIX8 "%02" PRIX8 "\n", code[0] & 0x0F, code[1], code[0] & 0x0F, code[1]);
    }
    break;
    case 0x0b:
        printf("JMP 0x%" PRIX8 "%02" PRIX8 " + V0               // Jump to address\n", code[0] & 0x0F, code[1]);
        break;
    case 0x0c:
        printf("c not handled yet\n");
        break;
    case 0x0d:
        printf("DRAW V%" PRIX8 "V%" PRIX8 " N%" PRIX8 "            // Draw the sprite in memory with %" PRIX8 " rows at I on coordinates V%" PRIX8 ", V%" PRIX8 "\n", code[0] & 0x0F, code[1] >> 4, code[1] & 0x0F, code[0] & 0x0F, code[1] >> 4, code[1] & 0x0F);
        break;
    case 0x0e:
        printf("e not handled yet\n");
        break;
    case 0x0f:
        printf("f not handled yet\n");
        break;
    }
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

bool cpu_stopped(void)
{
    return chip_8.should_stop;
}

void cpu_stop(void)
{
    chip_8.delay_running = false;
    pthread_join(chip_8.delay_timer.thread, NULL);

    chip_8.sound_running = false;
    pthread_join(chip_8.sound_timer.timer.thread, NULL);

    screen_stop();
    pthread_mutex_destroy(&(chip_8.screen.lock));
    pthread_mutex_destroy(&(chip_8.delay_timer.lock));
    pthread_mutex_destroy(&(chip_8.sound_timer.timer.lock));
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