#include "dacamp.h"
#include <stdbool.h>
#include <stdint.h>

static int current_volume = 75;
static int16_t volume_values[3] = {25600 * 75 / 100, 25600 * 75 / 100, 25600 * 75 / 100};
static int8_t mute_values[3] = {0, 0, 0};

void dacamp_set_volume(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    current_volume = percent;
    int16_t uac_value = (int16_t)((percent * DACAMP_VOLUME_CTRL_100_DB) / 100);
    volume_values[0] = uac_value;
    volume_values[1] = uac_value;
    volume_values[2] = uac_value;
    if (current_volume == 0)
    {
        mute_values[0] = 1;
        mute_values[1] = 1;
        mute_values[2] = 1;
    }
    else
    {
        mute_values[0] = 0;
        mute_values[1] = 0;
        mute_values[2] = 0;
    }
}

void dacamp_set_mute(bool mute)
{
    if (mute)
    {
        mute_values[0] = 1;
        mute_values[1] = 1;
        mute_values[2] = 1;
    }
    else if (current_volume > 0)
    {
        mute_values[0] = 0;
        mute_values[1] = 0;
        mute_values[2] = 0;
    }
}

void dacamp_adjust_volume(int delta_percent)
{
    dacamp_set_volume(current_volume + delta_percent);
}

int dacamp_get_volume_percent(void)
{
    return current_volume;
}

bool dacamp_is_muted(void)
{
    return mute_values[0] != 0;
}

void dacamp_get_volume(int16_t volume[3], int8_t mute[3])
{
    if (volume)
    {
        volume[0] = volume_values[0];
        volume[1] = volume_values[1];
        volume[2] = volume_values[2];
    }
    if (mute)
    {
        mute[0] = mute_values[0];
        mute[1] = mute_values[1];
        mute[2] = mute_values[2];
    }
}
