#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DACAMP_VOLUME_STEP_BITS 7
#define DACAMP_VOLUME_STEP (1 << DACAMP_VOLUME_STEP_BITS)
#define DACAMP_MIN_VOLUME_DB (-50)
#define DACAMP_VOLUME_PER_DB_UAC2 256 
#define DACAMP_MIN_VOLUME_UAC2 (DACAMP_MIN_VOLUME_DB * DACAMP_VOLUME_PER_DB_UAC2)
#define DACAMP_VOLUME_CTRL_100_DB (25600)

void dacamp_init(void);

void dacamp_set_volume(int percent);
void dacamp_get_volume(int16_t volume[3], int8_t mute[3]);

void dacamp_start(uint32_t sampleRate);

void dacamp_change_sample_rate(uint32_t sampleRate);

void dacamp_stop(void);

void dacamp_flush(void);

void dacamp_set_mute(bool mute);
void dacamp_adjust_volume(int delta_percent);
int dacamp_get_volume_percent(void);
bool dacamp_is_muted(void);
uint32_t dacamp_get_current_sample_rate(void);
bool dacamp_is_enabled(void);

void dacamp_debug_stuff_task(void);

//samples is an array of LR 16 bit or 24 (stored as 32) bit sample pairs
//sampleSize is 4 for PCM16 or 8 for PCM24
//L = sample&0xFFFF, R = sample >> 16 
//L = sample&0xFFFFFFFF, R = sample >> 32 
//returns how many samples were written to the internal buffer
int dacamp_pcm_put(const uint32_t* samples, int sampleCount, int sampleSize, const int16_t *volume, const int8_t *mute);