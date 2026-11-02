#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "io.h"
#include "sys/alt_irq.h"

#include "i_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#define OUTPUT_RATE 48000u
#define CHANNELS    8

/* Required by the existing sound configuration bindings. */
int use_libsamplerate = 0;
float libsamplerate_scale = 1.0f;

/* Inspect these in the debugger; do not print inside the ISR. */
volatile uint32_t doom_audio_interrupts;
volatile uint32_t doom_audio_frames;
volatile uint32_t doom_audio_empty_observations;

typedef struct {
    const uint8_t *data;
    uint32_t length;
    uint32_t step;
} Sample;

typedef struct {
    const Sample *sample;
    uint32_t position;
    uint32_t fraction;
    int left_gain;
    int right_gain;
} Voice;

static Voice voices[CHANNELS];
static boolean initialized;
static boolean prefix_enabled;

static int clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

static boolean valid_channel(int channel)
{
    return channel >= 0 && channel < CHANNELS;
}

static sfxinfo_t *original_sfx(sfxinfo_t *sfx)
{
    while (sfx->link != NULL)
        sfx = sfx->link;

    return sfx;
}

static int find_lump(sfxinfo_t *sfx)
{
    char name[9];

    sfx = original_sfx(sfx);

    if (prefix_enabled)
        snprintf(name, sizeof(name), "ds%.6s", sfx->name);
    else
        snprintf(name, sizeof(name), "%.8s", sfx->name);

    return W_CheckNumForName(name);
}

static Sample *load_sample(sfxinfo_t *sfx)
{
    sfx = original_sfx(sfx);

    if (sfx->driver_data != NULL)
        return (Sample *)sfx->driver_data;

    int lump = find_lump(sfx);

    if (lump < 0)
        return NULL;

    int lump_length = W_LumpLength(lump);

    if (lump_length < 8)
        return NULL;

    const uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    uint32_t rate = (uint32_t)data[2]
                  | ((uint32_t)data[3] << 8);

    uint32_t length = (uint32_t)data[4]
                    | ((uint32_t)data[5] << 8)
                    | ((uint32_t)data[6] << 16)
                    | ((uint32_t)data[7] << 24);

    if (data[0] != 3 || data[1] != 0 ||
        rate == 0 || length <= 48 ||
        length > (uint32_t)lump_length - 8u) {
        W_ReleaseLumpNum(lump);
        return NULL;
    }

    Sample *sample = malloc(sizeof(*sample));

    if (sample == NULL) {
        W_ReleaseLumpNum(lump);
        return NULL;
    }

    /* Match the existing DMX loader: omit 16 padding bytes
       at each end of the sample payload. */
    sample->data = data + 8u + 16u;
    sample->length = length - 32u;
    sample->step =
        (uint32_t)(((uint64_t)rate << 16) / OUTPUT_RATE);

    /*
     * Keep valid sound lumps PU_STATIC for this first port.
     * The ISR must never access purgeable WAD data.
     */
    sfx->driver_data = sample;
    return sample;
}

static void audio_isr(void *context)
{
    (void)context;
    ++doom_audio_interrupts;

    uint32_t space = IORD_32DIRECT(AUDIO_BASE, 4);
    unsigned int left_space = (space >> 24) & 255u;
    unsigned int right_space = (space >> 16) & 255u;
    unsigned int count =
        left_space < right_space ? left_space : right_space;

    if (left_space == 128u || right_space == 128u)
        ++doom_audio_empty_observations;

    /* Process a bounded batch, not an endless refill loop. */
    for (unsigned int frame = 0; frame < count; ++frame) {
        int left = 0;
        int right = 0;

        for (unsigned int channel = 0; channel < CHANNELS;
             ++channel) {
            Voice *voice = &voices[channel];
            const Sample *sample = voice->sample;

            if (sample == NULL)
                continue;

            int value = (int)sample->data[voice->position] - 128;

            left += value * voice->left_gain;
            right += value * voice->right_gain;

            uint32_t next = voice->fraction + sample->step;
            voice->position += next >> 16;
            voice->fraction = next & 65535u;

            if (voice->position >= sample->length)
                voice->sample = NULL;
        }

        /* Mixing headroom, followed by signed 16-bit saturation. */
        left = clamp(left / 2, -32768, 32767);
        right = clamp(right / 2, -32768, 32767);

        IOWR_32DIRECT(AUDIO_BASE, 8, (uint32_t)(int32_t)left);
        IOWR_32DIRECT(AUDIO_BASE, 12, (uint32_t)(int32_t)right);

        ++doom_audio_frames;
    }

    __asm__ volatile ("fence iorw,iorw" ::: "memory");
}

static boolean audio_init(boolean use_prefix)
{
    extern int snd_channels;

    prefix_enabled = use_prefix;
    snd_channels = CHANNELS;
    snd_samplerate = OUTPUT_RATE;

    if (!(IORD_32DIRECT(AV_CONFIG_BASE, 4) & 0x100u)) {
        puts("DOOM audio: codec configuration incomplete");
        return false;
    }

    memset(voices, 0, sizeof(voices));

    IOWR_32DIRECT(AUDIO_BASE, 0, 8u);
    IOWR_32DIRECT(AUDIO_BASE, 0, 0u);

    /* Prime both output FIFOs with silence. */
    for (unsigned int i = 0; i < 128u; ++i) {
        IOWR_32DIRECT(AUDIO_BASE, 8, 0u);
        IOWR_32DIRECT(AUDIO_BASE, 12, 0u);
    }

    if (alt_ic_isr_register(
            AUDIO_IRQ_INTERRUPT_CONTROLLER_ID,
            AUDIO_IRQ, audio_isr, NULL, NULL) != 0) {
        IOWR_32DIRECT(AUDIO_BASE, 0, 8u);
        puts("DOOM audio: interrupt registration failed");
        return false;
    }

    initialized = true;

    /* Enable playback FIFO write interrupts. */
    IOWR_32DIRECT(AUDIO_BASE, 0, 2u);

    puts("DOOM audio: 8 channels, stereo, 48000 Hz");
    return true;
}

static void audio_shutdown(void)
{
    alt_irq_context state = alt_irq_disable_all();

    IOWR_32DIRECT(AUDIO_BASE, 0, 8u);
    initialized = false;
    memset(voices, 0, sizeof(voices));

    alt_irq_enable_all(state);
}

static void update_params(int channel, int volume, int separation)
{
    if (!initialized || !valid_channel(channel))
        return;

    volume = clamp(volume, 0, 127);
    separation = clamp(separation, 0, 254);

    int left = volume * (254 - separation) / 127;
    int right = volume * separation / 127;

    alt_irq_context state = alt_irq_disable_all();
    voices[channel].left_gain = left;
    voices[channel].right_gain = right;
    alt_irq_enable_all(state);
}

static int start_sound(sfxinfo_t *sfx, int channel,
                       int volume, int separation)
{
    if (!initialized || !valid_channel(channel))
        return -1;

    /* WAD access and allocation happen outside the ISR. */
    Sample *sample = load_sample(sfx);

    if (sample == NULL)
        return -1;

    volume = clamp(volume, 0, 127);
    separation = clamp(separation, 0, 254);

    Voice next = {
        sample, 0, 0,
        volume * (254 - separation) / 127,
        volume * separation / 127
    };

    alt_irq_context state = alt_irq_disable_all();
    voices[channel] = next;
    alt_irq_enable_all(state);

    return channel;
}

static void stop_sound(int channel)
{
    if (!valid_channel(channel))
        return;

    alt_irq_context state = alt_irq_disable_all();
    voices[channel].sample = NULL;
    alt_irq_enable_all(state);
}

static boolean sound_playing(int channel)
{
    if (!initialized || !valid_channel(channel))
        return false;

    alt_irq_context state = alt_irq_disable_all();
    boolean playing = voices[channel].sample != NULL;
    alt_irq_enable_all(state);

    return playing;
}

static void precache(sfxinfo_t *sounds, int count)
{
    unsigned int loaded = 0;

    /* Entry zero is the "no sound" sentinel. */
    for (int i = 1; i < count; ++i) {
        if (load_sample(&sounds[i]) != NULL)
            ++loaded;
    }

    printf("DOOM audio: cached %u sound entries\n", loaded);
}

static void audio_update(void)
{
    /* Playback is serviced by the audio interrupt. */
}

static snddevice_t devices[] = { SNDDEVICE_SB };

sound_module_t DG_sound_module = {
    devices,
    1,
    audio_init,
    audio_shutdown,
    find_lump,
    audio_update,
    update_params,
    start_sound,
    stop_sound,
    sound_playing,
    precache
};