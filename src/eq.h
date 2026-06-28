#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define EQ_BAND_COUNT 8

typedef struct {
    float freq;
    float gain_db;
    float q;
    bool enabled;
    float a0;
    float a1;
    float a2;
    float b1;
    float b2;
    float z1_l;
    float z2_l;
    float z1_r;
    float z2_r;
} eq_band_t;

void eq_init(uint32_t sample_rate);
bool eq_set_band(int index, float freq, float gain_db, float q);
bool eq_enable_band(int index, bool enable);
void eq_reset(void);
void eq_set_sample_rate(uint32_t sample_rate);
void eq_get_band(int index, eq_band_t *band_out);
void eq_process_sample(int32_t *left, int32_t *right);
void eq_get_status(char *buffer, size_t len);
