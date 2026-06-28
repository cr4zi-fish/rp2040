#include "eq.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static eq_band_t bands[EQ_BAND_COUNT];
static uint32_t sample_rate = 48000;

static bool is_valid_band(int index)
{
    return index >= 0 && index < EQ_BAND_COUNT;
}

static void update_band_coeffs(eq_band_t *band)
{
    if (!band->enabled)
        return;

    float omega = 2.0f * M_PI * band->freq / sample_rate;
    float sinw = sinf(omega);
    float cosw = cosf(omega);
    float alpha = sinw / (2.0f * band->q);
    float A = powf(10.0f, band->gain_db / 40.0f);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha / A;

    band->a0 = b0 / a0;
    band->a1 = b1 / a0;
    band->a2 = b2 / a0;
    band->b1 = a1 / a0;
    band->b2 = a2 / a0;
}

void eq_init(uint32_t sr)
{
    sample_rate = sr;
    eq_reset();
}

bool eq_set_band(int index, float freq, float gain_db, float q)
{
    if (!is_valid_band(index)) return false;
    if (freq < 20.0f || freq > 20000.0f) return false;
    if (gain_db < -12.0f || gain_db > 12.0f) return false;
    if (q < 0.3f || q > 10.0f) return false;

    eq_band_t *band = &bands[index];
    band->freq = freq;
    band->gain_db = gain_db;
    band->q = q;
    band->enabled = true;
    memset(&band->z1_l, 0, sizeof(band->z1_l));
    memset(&band->z2_l, 0, sizeof(band->z2_l));
    memset(&band->z1_r, 0, sizeof(band->z1_r));
    memset(&band->z2_r, 0, sizeof(band->z2_r));

    update_band_coeffs(band);
    return true;
}

bool eq_enable_band(int index, bool enable)
{
    if (!is_valid_band(index)) return false;
    bands[index].enabled = enable;
    update_band_coeffs(&bands[index]);
    return true;
}

void eq_reset(void)
{
    memset(bands, 0, sizeof(bands));
    for (int i = 0; i < EQ_BAND_COUNT; ++i)
    {
        bands[i].freq = 1000.0f;
        bands[i].gain_db = 0.0f;
        bands[i].q = 1.0f;
        bands[i].enabled = false;
        update_band_coeffs(&bands[i]);
    }
}

void eq_set_sample_rate(uint32_t sr)
{
    sample_rate = sr;
    for (int i = 0; i < EQ_BAND_COUNT; ++i)
        update_band_coeffs(&bands[i]);
}

void eq_get_band(int index, eq_band_t *band_out)
{
    if (!band_out || !is_valid_band(index)) return;
    *band_out = bands[index];
}

void eq_process_sample(int32_t *left, int32_t *right)
{
    float xl = (float)*left;
    float xr = (float)*right;

    for (int i = 0; i < EQ_BAND_COUNT; ++i)
    {
        eq_band_t *band = &bands[i];
        if (!band->enabled) continue;

        float outl = band->a0 * xl + band->a1 * band->z1_l + band->a2 * band->z2_l - band->b1 * band->z1_l - band->b2 * band->z2_l;
        band->z2_l = band->z1_l;
        band->z1_l = xl;
        xl = outl;

        float outr = band->a0 * xr + band->a1 * band->z1_r + band->a2 * band->z2_r - band->b1 * band->z1_r - band->b2 * band->z2_r;
        band->z2_r = band->z1_r;
        band->z1_r = xr;
        xr = outr;
    }

    *left = (int32_t)xl;
    *right = (int32_t)xr;
}

void eq_get_status(char *buffer, size_t len)
{
    size_t off = 0;
    off += snprintf(buffer + off, len - off, "EQ status:\r\n");
    for (int i = 0; i < EQ_BAND_COUNT && off < len; ++i)
    {
        eq_band_t *band = &bands[i];
        off += snprintf(buffer + off, len - off, "Band %d: %s %.1fHz %+0.1fdB Q%.2f\r\n", i + 1,
                        band->enabled ? "On" : "Off", band->freq, band->gain_db, band->q);
    }
}
