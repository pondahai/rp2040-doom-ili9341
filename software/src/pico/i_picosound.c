//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2008 David Flater
// Copyright(C) 2021-2022 Graham Sanderson
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	System interface for sound.
//

#include "config.h"

#include <string.h>
#include <assert.h>
#include <doom/sounds.h>
#include <z_zone.h>

#include "deh_str.h"
#include "i_sound.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomtype.h"
#include "i_picosound.h"
#if USE_AUDIO_I2S
#include "pico/audio_i2s.h"
#elif USE_AUDIO_PWM
#include "pico/audio_pwm.h"
#elif USE_CUSTOM_AUDIO_PWM
// #include "pico/audio_pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#define AUDIO_BUFFER_SIZE 1024
#define AUDIO_MAX_SOURCES 6
#define REPETITION_RATE 4
static uint32_t single_sample = 0;
static uint32_t *single_sample_ptr = &single_sample;
static int pwm_dma_chan, trigger_dma_chan, sample_dma_chan;

static uint8_t audio_buffers[2][AUDIO_BUFFER_SIZE];
static volatile int cur_audio_buffer;
static volatile int last_audio_buffer;
struct MIXER_SOURCE {
  const unsigned char *samples;
  int len;
  int loop_start;
  int pos;
  bool active;
  bool loop;
  uint16_t volume; // 8.8 fixed point
};

static struct MIXER_SOURCE mixer_sources[AUDIO_MAX_SOURCES];
static int16_t mixer_buffer[AUDIO_BUFFER_SIZE];
#endif

#include "pico/binary_info.h"
#include "hardware/gpio.h"

//dahai
// #include "audio.h"

#define ADPCM_BLOCK_SIZE 128
#define ADPCM_SAMPLES_PER_BLOCK_SIZE 249
#define LOW_PASS_FILTER
#define MIX_MAX_VOLUME 128
typedef struct channel_s channel_t;

static volatile enum {
    FS_NONE,
    FS_FADE_OUT,
    FS_FADE_IN,
    FS_SILENT,
} fade_state;
#define FADE_STEP 8 // must be power of 2
uint16_t fade_level;

struct channel_s
{
    const uint8_t *data;
    const uint8_t *data_end;
    uint32_t offset;
    uint32_t step;
    uint8_t left, right; // 0-255
    uint8_t decompressed_size;
#if SOUND_LOW_PASS
    uint8_t alpha256;
#endif
    int8_t decompressed[ADPCM_SAMPLES_PER_BLOCK_SIZE];
};
#if 1 // AUDIO_I2S or AUDIO_PWM
static struct audio_buffer_pool *producer_pool;

static struct audio_format audio_format = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        #if 1
        .sample_freq = PICO_SOUND_SAMPLE_FREQ,
        #endif

#if USE_AUDIO_I2S
        .channel_count = 2,
#elif USE_AUDIO_PWM
        .channel_count = 1,
#endif
};

static struct audio_buffer_format producer_format = {
        .format = &audio_format,
#if USE_AUDIO_I2S
        .sample_stride = 4
#elif USE_AUDIO_PWM
        .sample_stride = 4
#endif
};
#endif
// ====== FROM ADPCM-LIB =====
#define CLIP(data, min, max) \
if ((data) > (max)) data = max; \
else if ((data) < (min)) data = min;

/* step table */
static const uint16_t step_table[89] = {
        7, 8, 9, 10, 11, 12, 13, 14,
        16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66,
        73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658,
        724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
        3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
        7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
        32767
};

/* step index tables */
static const int index_table[] = {
        /* adpcm data size is 4 */
        -1, -1, -1, -1, 2, 4, 6, 8
};
// =============================

static void (*music_generator)(audio_buffer_t *buffer);

static boolean sound_initialized = false;
static channel_t channels[NUM_SOUND_CHANNELS];

static boolean use_sfx_prefix;

static inline bool is_channel_playing(int channel) {
    return channels[channel].decompressed_size != 0;
}

static inline void stop_channel(int channel) {
    channels[channel].decompressed_size = 0;
}

static bool check_and_init_channel(int channel) {
    return sound_initialized && ((uint)channel) < NUM_SOUND_CHANNELS;
}

int adpcm_decode_block_s8(int8_t *outbuf, const uint8_t *inbuf, int inbufsize)
{
#if 1
    int samples = 1, chunks;

    if (inbufsize < 4)
        return 0;

    int32_t pcmdata = (int16_t) (inbuf [0] | (inbuf [1] << 8));
    *outbuf++ = pcmdata>>8u;
    int index = inbuf[2];

    if (index < 0 || index > 88 || inbuf [3])     // sanitize the input a little...
        return 0;

    inbufsize -= 4;
    inbuf += 4;

    chunks = inbufsize / 4;
    samples += chunks * 8;

    while (chunks--) {
        for (int i = 0; i < 4; ++i) {
            int step = step_table[index], delta = step >> 3;

            if (*inbuf & 1) delta += (step >> 2);
            if (*inbuf & 2) delta += (step >> 1);
            if (*inbuf & 4) delta += step;
            if (*inbuf & 8) delta = -delta;

            pcmdata += delta;
            index += index_table [*inbuf & 0x7];
            CLIP(index, 0, 88);
            CLIP(pcmdata, -32768, 32767);
            outbuf [i * 2] = pcmdata>>8u;

            step = step_table[index], delta = step >> 3;

            if (*inbuf & 0x10) delta += (step >> 2);
            if (*inbuf & 0x20) delta += (step >> 1);
            if (*inbuf & 0x40) delta += step;
            if (*inbuf & 0x80) delta = -delta;

            pcmdata += delta;
            index += index_table[(*inbuf >> 4) & 0x7];
            CLIP(index, 0, 88);
            CLIP(pcmdata, -32768, 32767);
            outbuf [i * 2 + 1] = pcmdata>>8u;
            inbuf++;
        }

        outbuf += 8;
    }

    return samples;
#else
    extern int adpcm_decode_block (int16_t *outbuf, const uint8_t *inbuf, size_t inbufsize, int channels);
    static int16_t tmp[ADPCM_SAMPLES_PER_BLOCK_SIZE];
    int samples = adpcm_decode_block(tmp, inbuf, inbufsize, 1);
    for(int s=0;s<samples;s++) {
        outbuf[s] = tmp[s] / 256;
    }
    return samples;
#endif
}

static void decompress_buffer(channel_t *channel) {
    if (channel->data == channel->data_end) {
        channel->decompressed_size = 0;
    } else {
        int block_size = MIN(ADPCM_BLOCK_SIZE, channel->data_end - channel->data);
        channel->decompressed_size = adpcm_decode_block_s8(channel->decompressed, channel->data, block_size);
        assert(channel->decompressed_size && channel->decompressed_size <= sizeof(channel->decompressed));
        channel->data += block_size;
    }
}

static boolean init_channel_for_sfx(channel_t *ch, const sfxinfo_t *sfxinfo, int pitch)
{
    int lumpnum = sfx_mut(sfxinfo)->lumpnum;
    int lumplen = W_LumpLength(lumpnum);

    const uint8_t *data = W_CacheLumpNum(lumpnum, PU_STATIC); // we don't track because we assume in ROWAD anyway

    if (lumplen < 8 || data[0] != 0x03 || data[1] != 0x80) // note 0x80 i.e. only support compressed right now
    {
        return false;
    }

    // 16 bit sample rate field, 32 bit length field

//    int length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
//    length -= 40; // 8 for header, 32 because we didn't updated it in lump converter (which cuts of unused 16 bit leading/leadout)
//    if (length <= 0) {
//        return false;
//    }
    int length = lumplen - 8;
//    printf("channel %d lump %d size %d at %p len2 %d\n", (int)(ch-channels), lumpnum, lumplen, data, length);

    ch->data = data + 8;
    ch->data_end = ch->data + length;

    uint32_t sample_freq = (data[3] << 8) | data[2];
    if (pitch == NORM_PITCH)
        ch->step = sample_freq * 65536 / PICO_SOUND_SAMPLE_FREQ;
    else
        ch->step = (uint32_t)((sample_freq * pitch) * 65536ull / (PICO_SOUND_SAMPLE_FREQ * pitch));

    decompress_buffer(ch); // we need non-zero decompressed size if playing
    ch->offset = 0;

#if SOUND_LOW_PASS
//    const float dt = 1.0f / PICO_SOUND_SAMPLE_FREQ;
//    const float rc = 1.0f / (3.14f * sample_freq);
//    const float alpha = dt / (rc + dt);
//    ch->alpha256 = (int)(256*alpha);
    ch->alpha256 = 256u * 201u * sample_freq / (201u * sample_freq + 64u * (uint)PICO_SOUND_SAMPLE_FREQ);
#endif
    return true;
}

static void GetSfxLumpName(const sfxinfo_t *sfx, char *buf, size_t buf_len)
{
    // Linked sfx lumps? Get the lump number for the sound linked to.
    if (sfx->link != NULL)
    {
        sfx = sfx->link;
    }

    // Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
    // do this.

    if (use_sfx_prefix)
    {
        M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
    }
    else
    {
        M_StringCopy(buf, DEH_String(sfx->name), buf_len);
    }
}

static void I_Pico_PrecacheSounds(should_be_const sfxinfo_t *sounds, int num_sounds)
{
    // no-op
}

static int I_Pico_GetSfxLumpNum(should_be_const sfxinfo_t *sfx)
{
    char namebuf[9];
    GetSfxLumpName(sfx, namebuf, sizeof(namebuf));
    return W_GetNumForName(namebuf);
}

static void I_Pico_UpdateSoundParams(int handle, int vol, int sep)
{
    int left, right;

    if (!sound_initialized || handle < 0 || handle >= NUM_SOUND_CHANNELS)
    {
        return;
    }

    // todo graham seems unnecessary
    left = ((254 - sep) * vol) / 127;
    right = ((sep) * vol) / 127;

    if (left < 0) left = 0;
    else if ( left > 255) left = 255;
    if (right < 0) right = 0;
    else if (right > 255) right = 255;

    channels[handle].left = left;
    channels[handle].right = right;
}

static int I_Pico_StartSound(should_be_const sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
    if (!check_and_init_channel(channel)) return -1;

    stop_channel(channel);
    channel_t *ch = &channels[channel];
    if (!init_channel_for_sfx(ch, sfxinfo, pitch)) {
        assert(!is_channel_playing(channel)); // don't expect to have to mark it sotpped
    }
    I_Pico_UpdateSoundParams(channel, vol, sep);
    return channel;
}

static void I_Pico_StopSound(int channel)
{
    if (check_and_init_channel(channel)) {

    }
}

static boolean I_Pico_SoundIsPlaying(int channel)
{
    if (!check_and_init_channel(channel)) return false;
    return is_channel_playing(channel);
}

#if USE_CUSTOM_AUDIO_PWM
uint8_t *audio_get_buffer(void)
{
  if (last_audio_buffer == cur_audio_buffer) {
    return NULL;
  }

  uint8_t *buf = audio_buffers[last_audio_buffer];
  last_audio_buffer = cur_audio_buffer;
  return buf;
}

static int audio_claim_unused_source(void)
{
  for (int i = 0; i < AUDIO_MAX_SOURCES; i++) {
    if (! mixer_sources[i].active) {
      mixer_sources[i].active = true;
      return i;
    }
  }
  return -1;
}

int __not_in_flash_func(audio_play_once)(const uint8_t *samples, int len)
{
  int source_id = audio_claim_unused_source();
  if (source_id < 0) {
    return -1;
  }
  struct MIXER_SOURCE *source = &mixer_sources[source_id];
  source->samples = samples;
  source->len = len;
  source->pos = 0;
  source->loop = false;
  source->volume = 255;
  return source_id;
}
#endif // USE_CUSTOM_AUDIO_PWM

static void I_Pico_UpdateSound(void)
{
    if (!sound_initialized) return;

    // todo note this is called from D_Main around the game loop, which is fast enough now but may not be.
    //  we can either poll more frequently, or use IRQ but then we have to be careful with threading (both OPL and channels)
    // todo hopefully at least we can run the AI fast enough.
#if 1 // AUDIO_I2S or AUDIO_PWM
#if USE_AUDIO_I2S
                    int32_t pwmtable[]={
                        0x0101,
                        0x0101,
                        0x0303,
                        0x0303,
                        0x0707,
                        0x0707,
                        0x0707,
                        0x0f0f,
                        0x0f0f,
                        0x1f1f,
                        0x1f1f,
                        0x3f3f,
                        0x3f3f,
                        0x7f7f,
                        0x7f7f,
                        0x7fff,
                        0xffff,
                        0x01010101,
                        0x01010101,
                        0x03030303,
                        0x03030303,
                        0x07070707,
                        0x07070707,
                        0x07070707,
                        0x0f0f0f0f,
                        0x0f0f0f0f,
                        0x1f1f1f1f,
                        0x1f1f1f1f,
                        0x3f3f3f3f,
                        0x3f3f3f3f,
                        0x7f7f7f7f,
                        0x7f7f7f7f,
                        0x7fff7fff,
                        0x7fff7fff,
                        0x00010001,
                        0x00010001,
                        0x00030003,
                        0x00070007,
                        0x000f000f,
                        0x001f001f,
                        0x003f003f,
                        0x007f007f,
                        0x00ff00ff,
                        0x01ff01ff,
                        0x03ff03ff,
                        0x07ff07ff,
                        0x0fff0fff,
                        0x1fff1fff,
                        0x3fff3fff,
                        0x7fff7fff,
                        0x7fff7fff,                        
                        // 0xffff,
                        // 0x7fff,
                        // 0x3fff,
                        // 0x1fff,
                        // 0x0fff,
                        // 0x07ff,
                        // 0x03ff,
                        // 0x01ff,
                        // 0x00ff,
                        // 0x007f,
                        // 0x003f,
                        // 0x001f,
                        // 0x000f,
                        // 0x0007,
                        // 0x0003,
                        // 0x0001,
                        // 0x0001ffff,
                        // 0x0003ffff,
                        // 0x0007ffff,
                        // 0x000fffff,
                        // 0x001fffff,
                        // 0x003fffff,
                        // 0x007fffff,
                        // 0x00ffffff,
                        // 0x01ffffff,
                        // 0x03ffffff,
                        // 0x07ffffff,
                        // 0x0fffffff,
                        // 0x1fffffff,
                        // 0x3fffffff,
                        // 0x7fffffff,
                        // 0x7fffffff,
                    };
#endif

    audio_buffer_t *buffer = take_audio_buffer(producer_pool, false);
    if (buffer) {
        if (music_generator) {
            // todo think about volume; this already has a (<< 3) in it
            music_generator(buffer);
        } else {
            memset(buffer->buffer->bytes, 0, buffer->buffer->size);
        }
        for(int ch=0; ch < NUM_SOUND_CHANNELS; ch++) {
            if (is_channel_playing(ch)) {
                channel_t *channel = &channels[ch];
                assert(channel->decompressed_size);
                int voll = channel->left/2;
                int volr = channel->right/2;
                uint offset_end = channel->decompressed_size * 65536;
                assert(channel->offset < offset_end);
                int16_t *samples = (int16_t *)buffer->buffer->bytes;
#if SOUND_LOW_PASS
                int alpha256 = channel->alpha256;
                int beta256 = 256 - alpha256;
                int sample = channel->decompressed[channel->offset >> 16];
#endif
                for(int s=0;s<buffer->max_sample_count;s++) {
#if !SOUND_LOW_PASS
                    int sample = channel->decompressed[channel->offset >> 16];
#else
                    // todo graham, note that since we are all at the same frequency (and it isn't the end
                    //  of the world anyway, we could do this across all channels at once)
                    sample = (beta256 * sample + alpha256 * channel->decompressed[channel->offset >> 16]) / 256;
#endif
                    *samples++ += sample * voll;
//#if USE_AUDIO_I2S
                    *samples++ += sample * volr;
//#endif
                    channel->offset += channel->step;
                    if (channel->offset >= offset_end) {
                        channel->offset -= offset_end;
                        decompress_buffer(channel);
                        offset_end = channel->decompressed_size * 65536;
                        if (channel->offset >= offset_end) {
                            stop_channel(ch);
                            break;
                        }
                    }
                }
            }
        }
        // enum audio_correction_mode m = audio_pwm_get_correction_mode();
        // bool done = false;
        // done = audio_pwm_set_correction_mode(m);

        buffer->sample_count = buffer->max_sample_count;
        if (fade_state == FS_SILENT) {
            memset(buffer->buffer->bytes, 0, buffer->buffer->size);
        } else if (fade_state != FS_NONE) {
            int16_t *samples = (int16_t *)buffer->buffer->bytes;
            int fade_step = fade_state == FS_FADE_IN ? FADE_STEP : -FADE_STEP;
            int i;
            for(i=0;i<buffer->sample_count * 2 && fade_level;i+=2) {
                samples[i] = (samples[i] * (int)fade_level) >> 16;
                samples[i+1] = (samples[i+1] * (int)fade_level) >> 16;
                fade_level += fade_step;
            }
            if (!fade_level) {
                if (fade_state == FS_FADE_OUT) {
                    for(;i<buffer->sample_count * 2;i++) {
                        samples[i] = 0;
                    }
                    fade_state = FS_SILENT;
                } else {
                    fade_state = FS_NONE;
                }
            }
        }
        //dahai
        int16_t *samples = (int16_t *)buffer->buffer->bytes;
#if USE_AUDIO_I2S
        for(int i=0;i<buffer->sample_count * 2;i+=2){
            samples[i] = ~pwmtable[(samples[i]/1024)&31];
            samples[i+1] = ~pwmtable[(samples[i]/1024)&31]>>16;
        }
#endif
        give_audio_buffer(producer_pool, buffer);

    }

#endif // AUDIO_I2S or AUDIO_PWM

#if USE_CUSTOM_AUDIO_PWM
        uint8_t *buffer = audio_get_buffer();
        if (music_generator) {
            // todo think about volume; this already has a (<< 3) in it
            music_generator(buffer);
        } else {
            memset(buffer, 0, AUDIO_BUFFER_SIZE);
        }
        for(int ch=0; ch < NUM_SOUND_CHANNELS; ch++) {
            if (is_channel_playing(ch)) {
                channel_t *channel = &channels[ch];
                assert(channel->decompressed_size);
                int voll = channel->left/2;
                int volr = channel->right/2;
                uint offset_end = channel->decompressed_size * 65536;
                assert(channel->offset < offset_end);
                uint8_t *samples = (uint8_t *)buffer;
#if SOUND_LOW_PASS
                int alpha256 = channel->alpha256;
                int beta256 = 256 - alpha256;
                int sample = channel->decompressed[channel->offset >> 16];
#endif
                for(int s=0;s<AUDIO_BUFFER_SIZE;s++) {
#if !SOUND_LOW_PASS
                    int sample = channel->decompressed[channel->offset >> 16];
#else
                    // todo graham, note that since we are all at the same frequency (and it isn't the end
                    //  of the world anyway, we could do this across all channels at once)
                    sample = (beta256 * sample + alpha256 * channel->decompressed[channel->offset >> 16]) / 256;
#endif
                    *samples++ += sample * voll;

                    channel->offset += channel->step;
                    if (channel->offset >= offset_end) {
                        channel->offset -= offset_end;
                        decompress_buffer(channel);
                        offset_end = channel->decompressed_size * 65536;
                        if (channel->offset >= offset_end) {
                            stop_channel(ch);
                            break;
                        }
                    }
                }
            }
        }
        //buffer->sample_count = buffer->max_sample_count;
        if (fade_state == FS_SILENT) {
            memset(buffer, 0, AUDIO_BUFFER_SIZE);
        } else if (fade_state != FS_NONE) {
            uint8_t *samples = (uint8_t *)buffer;
            int fade_step = fade_state == FS_FADE_IN ? FADE_STEP : -FADE_STEP;
            int i;
            for(i=0;i<AUDIO_BUFFER_SIZE * 2 && fade_level;i+=2) {
                samples[i] = (samples[i] * (int)fade_level) >> 16;
                samples[i+1] = (samples[i+1] * (int)fade_level) >> 16;
                fade_level += fade_step;
            }
            if (!fade_level) {
                if (fade_state == FS_FADE_OUT) {
                    for(;i<AUDIO_BUFFER_SIZE * 2;i++) {
                        samples[i] = 0;
                    }
                    fade_state = FS_SILENT;
                } else {
                    fade_state = FS_NONE;
                }
            }
        }
        // give_audio_buffer(producer_pool, buffer);

#endif
}

static void I_Pico_ShutdownSound(void)
{
    if (!sound_initialized)
    {
        return;
    }
    sound_initialized = false;
}

#if USE_CUSTOM_AUDIO_PWM
static void __isr __time_critical_func(dma_handler)()
{
  cur_audio_buffer = 1 - cur_audio_buffer;
  dma_hw->ch[sample_dma_chan].al1_read_addr       = (intptr_t) &audio_buffers[cur_audio_buffer][0];
  dma_hw->ch[trigger_dma_chan].al3_read_addr_trig = (intptr_t) &single_sample_ptr;

  dma_hw->ints1 = 1u << trigger_dma_chan;
}
#endif

static boolean I_Pico_InitSound(boolean _use_sfx_prefix)
{
    int i;
    use_sfx_prefix = _use_sfx_prefix;
#if 1 // AUDIO_I2S or AUDIO_PWM
    // todo this will likely need adjustment - maybe with IRQs/double buffer & pull from audio we can make it quite small
    producer_pool = audio_new_producer_pool(&producer_format, 2, 1024); // todo correct size
#endif // AUDIO_I2S or AUDIO_PWM
#if USE_AUDIO_I2S
    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            //.clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 6,
            .pio_sm = 0,
    };

    const struct audio_format *output_format;
    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

#if INCREASE_I2S_DRIVE_STRENGTH
    //bi_decl(bi_program_feature("12mA I2S"));
    gpio_set_drive_strength(PICO_AUDIO_I2S_DATA_PIN, GPIO_DRIVE_STRENGTH_12MA);
    //gpio_set_drive_strength(PICO_AUDIO_I2S_CLOCK_PIN_BASE, GPIO_DRIVE_STRENGTH_12MA);
    //gpio_set_drive_strength(PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, GPIO_DRIVE_STRENGTH_12MA);
#endif
    // we want to pass thr
    bool ok = audio_i2s_connect_extra(producer_pool, false, 0, 0, NULL);
    assert(ok);
    audio_i2s_set_enabled(true);

#elif USE_AUDIO_PWM
    struct audio_pwm_channel_config config = {
                .core = {
                        .base_pin = 0,
                        .pio_sm = 0,
                        .dma_channel =0
                },
                .pattern =3,
    };
    // set_sys_clock_48mhz();
    bool __unused ok;
    const struct audio_format *output_format;
    output_format = audio_pwm_setup(&audio_format, -1, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }
    ok = audio_pwm_default_connect(producer_pool, false);
    assert(ok);
    audio_pwm_set_enabled(true);

#elif USE_CUSTOM_AUDIO_PWM
  gpio_set_function(PICO_AUDIO_PWM_PIO, GPIO_FUNC_PWM);

  int audio_pin_slice = pwm_gpio_to_slice_num(PICO_AUDIO_PWM_PIO);
  int audio_pin_chan = pwm_gpio_to_channel(PICO_AUDIO_PWM_PIO);

  uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  float clock_div = ((float)f_clk_sys * 1000.0f) / 254.0f / (float) PICO_SOUND_SAMPLE_FREQ / (float) REPETITION_RATE;

  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, clock_div);
  pwm_config_set_wrap(&config, 254);
  pwm_init(audio_pin_slice, &config, true);

  pwm_dma_chan     = dma_claim_unused_channel(true);
  trigger_dma_chan = dma_claim_unused_channel(true);
  sample_dma_chan  = dma_claim_unused_channel(true);

  // setup PWM DMA channel
  dma_channel_config pwm_dma_chan_config = dma_channel_get_default_config(pwm_dma_chan);
  channel_config_set_transfer_data_size(&pwm_dma_chan_config, DMA_SIZE_32);              // transfer 32 bits at a time
  channel_config_set_read_increment(&pwm_dma_chan_config, false);                        // always read from the same address
  channel_config_set_write_increment(&pwm_dma_chan_config, false);                       // always write to the same address
  channel_config_set_chain_to(&pwm_dma_chan_config, sample_dma_chan);                    // trigger sample DMA channel when done
  channel_config_set_dreq(&pwm_dma_chan_config, DREQ_PWM_WRAP0 + audio_pin_slice);       // transfer on PWM cycle end
  dma_channel_configure(pwm_dma_chan,
                        &pwm_dma_chan_config,
                        &pwm_hw->slice[audio_pin_slice].cc,   // write to PWM slice CC register
                        &single_sample,                       // read from single_sample
                        REPETITION_RATE,                      // transfer once per desired sample repetition
                        false                                 // don't start yet
                        );

  // setup trigger DMA channel
  dma_channel_config trigger_dma_chan_config = dma_channel_get_default_config(trigger_dma_chan);
  channel_config_set_transfer_data_size(&trigger_dma_chan_config, DMA_SIZE_32);          // transfer 32-bits at a time
  channel_config_set_read_increment(&trigger_dma_chan_config, false);                    // always read from the same address
  channel_config_set_write_increment(&trigger_dma_chan_config, false);                   // always write to the same address
  channel_config_set_dreq(&trigger_dma_chan_config, DREQ_PWM_WRAP0 + audio_pin_slice);   // transfer on PWM cycle end
  dma_channel_configure(trigger_dma_chan,
                        &trigger_dma_chan_config,
                        &dma_hw->ch[pwm_dma_chan].al3_read_addr_trig,     // write to PWM DMA channel read address trigger
                        &single_sample_ptr,                               // read from location containing the address of single_sample
                        REPETITION_RATE * AUDIO_BUFFER_SIZE,              // trigger once per audio sample per repetition rate
                        false                                             // don't start yet
                        );
  dma_channel_set_irq1_enabled(trigger_dma_chan, true);    // fire interrupt when trigger DMA channel is done
  irq_set_exclusive_handler(DMA_IRQ_1, dma_handler);
  irq_set_enabled(DMA_IRQ_1, true);

  // setup sample DMA channel
  dma_channel_config sample_dma_chan_config = dma_channel_get_default_config(sample_dma_chan);
  channel_config_set_transfer_data_size(&sample_dma_chan_config, DMA_SIZE_8);  // transfer 8-bits at a time
  channel_config_set_read_increment(&sample_dma_chan_config, true);            // increment read address to go through audio buffer
  channel_config_set_write_increment(&sample_dma_chan_config, false);          // always write to the same address
  dma_channel_configure(sample_dma_chan,
                        &sample_dma_chan_config,
                        (char*)&single_sample + 2*audio_pin_chan,  // write to single_sample
                        &audio_buffers[0][0],                      // read from audio buffer
                        1,                                         // only do one transfer (once per PWM DMA completion due to chaining)
                        false                                      // don't start yet
                        );


  // clear audio buffers
  memset(audio_buffers[0], 128, AUDIO_BUFFER_SIZE);
  memset(audio_buffers[1], 128, AUDIO_BUFFER_SIZE);
  
  // kick things off with the trigger DMA channel
  dma_channel_start(trigger_dma_chan);

#endif


    sound_initialized = true;
    return true;
}

static snddevice_t sound_pico_devices[] =
{
    SNDDEVICE_SB,
};

sound_module_t sound_pico_module =
{
    sound_pico_devices,
    arrlen(sound_pico_devices),
    I_Pico_InitSound,
    I_Pico_ShutdownSound,
    I_Pico_GetSfxLumpNum,
    I_Pico_UpdateSound,
    I_Pico_UpdateSoundParams,
    I_Pico_StartSound,
    I_Pico_StopSound,
    I_Pico_SoundIsPlaying,
    I_Pico_PrecacheSounds,
};

bool I_PicoSoundIsInitialized(void) {
    return sound_initialized;
}

void I_PicoSoundSetMusicGenerator(void (*generator)(audio_buffer_t *buffer)) {
    music_generator = generator;
}

#if PICO_ON_DEVICE
void I_PicoSoundFade(bool in) {
    fade_state = in ? FS_FADE_IN : FS_FADE_OUT;
    fade_level = in ? FADE_STEP : 0x10000 - FADE_STEP;
}

bool I_PicoSoundFading(void) {
    return fade_state == FS_FADE_IN || fade_state == FS_FADE_OUT;
}
#endif
