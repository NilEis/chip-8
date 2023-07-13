#include "audio/beep.h"
#include "SDL2/SDL.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define SAMPLE_FREQUENCY 44100
#define SINE_FREQUENCY 1000
#define SAMPLES_PER_SINE ((SAMPLE_FREQUENCY) / (SINE_FREQUENCY))

SDL_AudioSpec audio_spec_template;
SDL_AudioSpec audio_spec;

SDL_AudioDeviceID audio_device;

void beep_with_init(void);
void beep_f(void);
void beep_cleanup(void);
void SDLAudioCallback(void *data, uint8_t *buffer, int length);

uint32_t sample_position = 0;

void (*beep)(void) = beep_with_init;

void beep_with_init(void)
{
    uint32_t b = SDL_WasInit(SDL_INIT_AUDIO);
    if (!b)
    {
        SDL_Init(SDL_INIT_AUDIO);
    }

    beep = beep_f;

    SDL_zero(audio_spec_template);
    audio_spec_template.freq = SAMPLE_FREQUENCY;
    audio_spec_template.format = AUDIO_F32LSB;
    audio_spec_template.channels = 1;
    audio_spec_template.samples = 2048;
    audio_spec_template.callback = SDLAudioCallback;
    audio_spec_template.userdata = &sample_position;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec_template, &audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    printf("audio_spec_template.freq %d = %d\n", SAMPLE_FREQUENCY, audio_spec.freq);
    printf("audio_spec_template.format %d = %d\n", AUDIO_F32LSB, audio_spec.format);
    printf("audio_spec_template.channels %d = %d\n", 1, audio_spec.channels);
    printf("audio_spec_template.samples %d = %d\n", 2048, audio_spec.samples);
    printf("audio_spec_template.callback %p = %p\n", SDLAudioCallback, audio_spec.callback);
    printf("audio_spec_template.userdata %p = %p\n", &sample_position, audio_spec.userdata);

    if (audio_device == 0)
    {
        printf("Failed to open audio: %s\n", SDL_GetError());
    }

    atexit(beep_cleanup);

    beep_f();
}

void beep_f(void)
{
    SDL_PauseAudioDevice(audio_device, 0);
    while(1);
    SDL_PauseAudioDevice(audio_device, 1);
}

void SDLAudioCallback(void *data, uint8_t *buffer, int length)
{
    putchar('a');
    uint32_t *pos = data;
    for (int i = 0; i < length; ++i)
    {
        buffer[i] = (sin((double)(*pos) / (double)SAMPLES_PER_SINE * M_PI * 2.0) + 1.0) * 127.5;
        ++(*pos);
    }
}

void beep_cleanup(void)
{
    SDL_CloseAudioDevice(audio_device);
}