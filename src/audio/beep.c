#include "audio/beep.h"
#include "SDL3/SDL.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SAMPLE_FREQUENCY 44100
#define SINE_FREQUENCY 1000
#define SAMPLES_PER_SINE ((SAMPLE_FREQUENCY) / (SINE_FREQUENCY))

struct beeper
{
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
    double frequency;
    double volume;
    int pos;
    void (*writeData)(uint8_t *ptr, double data);
    int (*calculateOffset)(int sample, int channel);
} beeper;

void beep_with_init(void);

static int calculateOffset_f32(int sample, int channel);
static int calculateOffset_s16(int sample, int channel);

static void writeData_s16(uint8_t *ptr, double data);
static void writeData_f32(uint8_t *ptr, double data);

void beep_f(void);
void beep_cleanup(void);

void beep_callback(void *data, uint8_t *buffer, int length);

static double get_data(void);

uint32_t sample_position = 0;

void (*beep)(void) = beep_with_init;

void beep_init(void)
{
    uint32_t b = SDL_WasInit(SDL_INIT_AUDIO);
    if (!b)
    {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }

    SDL_AudioSpec audio_spec_template;

    SDL_zero(audio_spec_template);
    audio_spec_template.freq = SAMPLE_FREQUENCY;
    audio_spec_template.format = SDL_AUDIO_S16;
    audio_spec_template.channels = 1;
    audio_spec_template.samples = 512;
    audio_spec_template.callback = beep_callback;
    audio_spec_template.userdata = &sample_position;

    beeper.audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec_template, &(beeper.audio_spec), 0);

    if (beeper.audio_device == 0)
    {
        printf("Failed to open audio: %s\n", SDL_GetError());
        return;
    }

    const char *formatName;
    switch (beeper.audio_spec.format)
    {
    case SDL_AUDIO_S16:
        beeper.writeData = writeData_s16;
        beeper.calculateOffset = calculateOffset_s16;
        formatName = "AUDIO_S16";
        break;
    case SDL_AUDIO_F32:
        beeper.writeData = writeData_f32;
        beeper.calculateOffset = calculateOffset_f32;
        formatName = "AUDIO_F32";
        break;
    default:
        SDL_Log("Unsupported audio format: %i", beeper.audio_spec.format);
        // TODO: throw exception
    }

    printf("Audio device name: %s\n", SDL_GetAudioDeviceName(beeper.audio_device, 0));

    printf("format: %s\n", formatName);
    printf("audio_spec_template.freq %d = %d\n", audio_spec_template.freq, beeper.audio_spec.freq);
    printf("audio_spec_template.format %d = %d\n", audio_spec_template.format, beeper.audio_spec.format);
    printf("audio_spec_template.channels %d = %d\n", audio_spec_template.channels, beeper.audio_spec.channels);
    printf("audio_spec_template.samples %d = %d\n", audio_spec_template.samples, beeper.audio_spec.samples);
    printf("audio_spec_template.callback %p = %p\n", audio_spec_template.callback, beeper.audio_spec.callback);
    printf("audio_spec_template.userdata %p = %p\n", &audio_spec_template.userdata, beeper.audio_spec.userdata);
    printf("padding: %d\n", beeper.audio_spec.padding);
    printf("size: %d\n", beeper.audio_spec.size);

    beep = beep_f;

    atexit(beep_cleanup);
}

void beep_with_init(void)
{
    beep_init();
    beep_f();
}

static int calculateOffset_s16(int sample, int channel)
{
    return (sample * sizeof(int16_t) * beeper.audio_spec.channels) +
           (channel * sizeof(int16_t));
}

static int calculateOffset_f32(int sample, int channel)
{
    return (sample * sizeof(float) * beeper.audio_spec.channels) +
           (channel * sizeof(float));
}

static void writeData_s16(uint8_t *ptr, double data)
{
    int16_t *ptr_typed = (int16_t *)ptr;
    double range = (double)INT16_MAX - (double)INT16_MIN;
    double data_scaled = data * range / 2.0;
    *ptr_typed = data_scaled;
}

static void writeData_f32(uint8_t *ptr, double data)
{
    float *ptr_typed = (float *)ptr;
    *ptr_typed = data;
}

void beep_f(void)
{
    SDL_PlayAudioDevice(beeper.audio_device);
}

void beep_stop()
{
    SDL_PauseAudioDevice(beeper.audio_device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void beep_cleanup(void)
{
    SDL_CloseAudioDevice(beeper.audio_device);
}

void beep_callback(void *data, uint8_t *buffer, int length)
{
    // Unused parameters
    (void)data;
    (void)length;

    // Write data to the entire buffer by iterating through all samples and
    // channels.
    for (int sample = 0; sample < beeper.audio_spec.samples; ++sample)
    {
        double data = get_data();
        beeper.pos++;

        // Write the same data to all channels
        for (int channel = 0; channel < beeper.audio_spec.channels; ++channel)
        {
            int offset = beeper.calculateOffset(sample, channel);
            uint8_t *ptrData = buffer + offset;
            beeper.writeData(ptrData, data);
        }
    }
}

static double get_data(void)
{
    double sampleRate = (double)(beeper.audio_spec.freq);

    // Units: samples
    double period = sampleRate / beeper.frequency;

    // Reset m_pos when it reaches the start of a period so it doesn't run off
    // to infinity (though this won't happen unless you are playing sound for a
    // very long time)
    if (beeper.pos % (int)period == 0)
    {
        beeper.pos = 0;
    }

    double pos = beeper.pos;
    double angular_freq = (1.0 / period) * 2.0 * M_PI;
    double amplitude = beeper.volume;

    return sin(pos * angular_freq) * amplitude;
}

void beep_set_frequency(double frequency)
{
    beeper.frequency = frequency;
}

void beep_set_volume(double volume)
{
    beeper.volume = volume;
}