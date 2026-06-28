#include "terminal.h"
#include "dacamp.h"
#include "eq.h"
#include "usb_descriptors.h"
#include "tusb.h"

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/watchdog.h>
#include <pico/stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char line_buffer[128];
static size_t line_length = 0;

#define TERMINAL_MAX_PROFILES 8
#define TERMINAL_PROFILE_NAME_LEN 24
#define TERMINAL_PROFILE_FLASH_MAGIC 0x46534850u
#define TERMINAL_PROFILE_FLASH_VERSION 1u
#define TERMINAL_PROFILE_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct
{
    char name[TERMINAL_PROFILE_NAME_LEN];
    bool used;
    eq_band_t bands[EQ_BAND_COUNT];
} terminal_eq_profile_t;

typedef struct
{
    float freq;
    float gain_db;
    float q;
    uint8_t enabled;
    uint8_t reserved[3];
} terminal_eq_band_store_t;

typedef struct
{
    uint8_t used;
    uint8_t reserved[3];
    char name[TERMINAL_PROFILE_NAME_LEN];
    terminal_eq_band_store_t bands[EQ_BAND_COUNT];
} terminal_eq_profile_store_t;

typedef union
{
    struct
    {
        uint32_t magic;
        uint32_t version;
        char default_profile_name[TERMINAL_PROFILE_NAME_LEN];
        uint8_t reserved[4];
        terminal_eq_profile_store_t profiles[TERMINAL_MAX_PROFILES];
    } data;
    uint8_t raw[FLASH_SECTOR_SIZE];
} terminal_profile_flash_page_t;

static terminal_eq_profile_t profiles[TERMINAL_MAX_PROFILES];
static char default_profile_name[TERMINAL_PROFILE_NAME_LEN] = "";
static terminal_profile_flash_page_t flash_page;

static int terminal_find_profile(const char *name)
{
    for (int i = 0; i < TERMINAL_MAX_PROFILES; ++i)
    {
        if (profiles[i].used && strcmp(profiles[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int terminal_find_free_profile(void)
{
    for (int i = 0; i < TERMINAL_MAX_PROFILES; ++i)
    {
        if (!profiles[i].used)
            return i;
    }
    return -1;
}

static void terminal_print(const char *s)
{
    if (tud_cdc_connected())
        tud_cdc_write_str(s);
}

static void print(const char *s)
{
    terminal_print(s);
}

static void print_prompt(void)
{
    print("fishce-dac> ");
}

static void terminal_load_flash_page(void);
static void terminal_import_flash_profiles(void);
static void terminal_export_flash_profiles(void);
static void terminal_flash_write_page(void);

void terminal_init(void)
{
    line_length = 0;
    line_buffer[0] = '\0';
    terminal_load_flash_page();
    terminal_import_flash_profiles();
}

static void terminal_write(const char *fmt, ...)
{
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    if (len > 0)
    {
        if (tud_cdc_connected())
            tud_cdc_write(tmp, (uint32_t)len);
    }
}

static void terminal_print_help(void)
{
    terminal_write("Available commands:\r\n");
    terminal_write("help [command]\r\n");
    terminal_write("eq info\r\n");
    terminal_write("eq set <band> <freq> <gain> <Q>\r\n");
    terminal_write("eq enable <band>\r\n");
    terminal_write("eq disable <band>\r\n");
    terminal_write("eq reset\r\n");
    terminal_write("eq save-profile <name>\r\n");
    terminal_write("eq load-profile <name>\r\n");
    terminal_write("eq list-profiles\r\n");
    terminal_write("eq delete-profile <name>\r\n");
    terminal_write("eq default <name>\r\n");
    terminal_write("volume info\r\n");
    terminal_write("volume set <0-100>\r\n");
    terminal_write("volume up [step]\r\n");
    terminal_write("volume down [step]\r\n");
    terminal_write("volume mute\r\n");
    terminal_write("volume unmute\r\n");
    terminal_write("dac info\r\n");
    terminal_write("dac status\r\n");
    terminal_write("dac version\r\n");
    terminal_write("dac uptime\r\n");
    terminal_write("system info\r\n");
    terminal_write("system save\r\n");
    terminal_write("system reset\r\n");
    terminal_write("system reboot\r\n");
    terminal_write("stats\r\n");
    terminal_write("stats cpu\r\n");
    terminal_write("stats ram\r\n");
    terminal_write("stats flash\r\n");
    terminal_write("stats audio\r\n");
}

static void terminal_print_help_command(const char *cmd)
{
    if (strcmp(cmd, "eq") == 0)
    {
        terminal_write("eq info\r\n");
        terminal_write("eq set <band> <freq> <gain> <Q>\r\n");
        terminal_write("eq enable <band>\r\n");
        terminal_write("eq disable <band>\r\n");
        terminal_write("eq reset\r\n");
        terminal_write("eq save-profile <name>\r\n");
        terminal_write("eq load-profile <name>\r\n");
        terminal_write("eq list-profiles\r\n");
        terminal_write("eq delete-profile <name>\r\n");
        terminal_write("eq default <name>\r\n");
        return;
    }
    if (strcmp(cmd, "volume") == 0)
    {
        terminal_write("volume info\r\n");
        terminal_write("volume set <0-100>\r\n");
        terminal_write("volume up [step]\r\n");
        terminal_write("volume down [step]\r\n");
        terminal_write("volume mute\r\n");
        terminal_write("volume unmute\r\n");
        return;
    }
    if (strcmp(cmd, "dac") == 0)
    {
        terminal_write("dac info\r\n");
        terminal_write("dac status\r\n");
        terminal_write("dac version\r\n");
        terminal_write("dac uptime\r\n");
        return;
    }
    if (strcmp(cmd, "system") == 0)
    {
        terminal_write("system info\r\n");
        terminal_write("system save\r\n");
        terminal_write("system reset\r\n");
        terminal_write("system reboot\r\n");
        return;
    }
    if (strcmp(cmd, "stats") == 0)
    {
        terminal_write("stats\r\n");
        terminal_write("stats cpu\r\n");
        terminal_write("stats ram\r\n");
        terminal_write("stats flash\r\n");
        terminal_write("stats audio\r\n");
        return;
    }
    terminal_write("No detailed help available for this command\r\n");
}

static void terminal_flash_write_page(void)
{
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(TERMINAL_PROFILE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(TERMINAL_PROFILE_FLASH_OFFSET, flash_page.raw, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);
}

static void terminal_load_flash_page(void)
{
    memcpy(&flash_page, (const void *)(XIP_BASE + TERMINAL_PROFILE_FLASH_OFFSET), FLASH_SECTOR_SIZE);
    if (flash_page.data.magic != TERMINAL_PROFILE_FLASH_MAGIC || flash_page.data.version != TERMINAL_PROFILE_FLASH_VERSION)
    {
        memset(&flash_page, 0, sizeof(flash_page));
        flash_page.data.magic = TERMINAL_PROFILE_FLASH_MAGIC;
        flash_page.data.version = TERMINAL_PROFILE_FLASH_VERSION;
        flash_page.data.default_profile_name[0] = '\0';
        terminal_flash_write_page();
    }
}

static void terminal_import_flash_profiles(void)
{
    for (int i = 0; i < TERMINAL_MAX_PROFILES; ++i)
    {
        profiles[i].used = flash_page.data.profiles[i].used != 0;
        if (profiles[i].used)
        {
            strncpy(profiles[i].name, flash_page.data.profiles[i].name, TERMINAL_PROFILE_NAME_LEN - 1);
            profiles[i].name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
            for (int b = 0; b < EQ_BAND_COUNT; ++b)
            {
                profiles[i].bands[b].freq = flash_page.data.profiles[i].bands[b].freq;
                profiles[i].bands[b].gain_db = flash_page.data.profiles[i].bands[b].gain_db;
                profiles[i].bands[b].q = flash_page.data.profiles[i].bands[b].q;
                profiles[i].bands[b].enabled = flash_page.data.profiles[i].bands[b].enabled != 0;
            }
        }
        else
        {
            profiles[i].name[0] = '\0';
        }
    }
    strncpy(default_profile_name, flash_page.data.default_profile_name, TERMINAL_PROFILE_NAME_LEN - 1);
    default_profile_name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
}

static void terminal_export_flash_profiles(void)
{
    flash_page.data.magic = TERMINAL_PROFILE_FLASH_MAGIC;
    flash_page.data.version = TERMINAL_PROFILE_FLASH_VERSION;
    strncpy(flash_page.data.default_profile_name, default_profile_name, TERMINAL_PROFILE_NAME_LEN - 1);
    flash_page.data.default_profile_name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
    for (int i = 0; i < TERMINAL_MAX_PROFILES; ++i)
    {
        flash_page.data.profiles[i].used = profiles[i].used ? 1 : 0;
        strncpy(flash_page.data.profiles[i].name, profiles[i].name, TERMINAL_PROFILE_NAME_LEN - 1);
        flash_page.data.profiles[i].name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
        for (int b = 0; b < EQ_BAND_COUNT; ++b)
        {
            flash_page.data.profiles[i].bands[b].freq = profiles[i].bands[b].freq;
            flash_page.data.profiles[i].bands[b].gain_db = profiles[i].bands[b].gain_db;
            flash_page.data.profiles[i].bands[b].q = profiles[i].bands[b].q;
            flash_page.data.profiles[i].bands[b].enabled = profiles[i].bands[b].enabled ? 1 : 0;
        }
    }
    terminal_flash_write_page();
}

static void terminal_save_profile(const char *name)
{
    if (!name || name[0] == '\0')
    {
        terminal_write("Profile name required\r\n");
        return;
    }
    int idx = terminal_find_profile(name);
    if (idx < 0)
    {
        idx = terminal_find_free_profile();
        if (idx < 0)
        {
            terminal_write("Profile storage full\r\n");
            return;
        }
    }
    strncpy(profiles[idx].name, name, TERMINAL_PROFILE_NAME_LEN - 1);
    profiles[idx].name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
    profiles[idx].used = true;
    for (int i = 0; i < EQ_BAND_COUNT; ++i)
    {
        eq_get_band(i, &profiles[idx].bands[i]);
    }
    terminal_export_flash_profiles();
    terminal_write("Profile saved to flash\r\n");
}

static void terminal_load_profile(const char *name)
{
    int idx = terminal_find_profile(name);
    if (idx < 0)
    {
        terminal_write("Profile not found\r\n");
        return;
    }
    for (int i = 0; i < EQ_BAND_COUNT; ++i)
    {
        eq_band_t *band = &profiles[idx].bands[i];
        if (band->enabled)
        {
            eq_set_band(i, band->freq, band->gain_db, band->q);
        }
        else
        {
            eq_enable_band(i, false);
        }
    }
    terminal_write("Profile loaded\r\n");
}

static void terminal_delete_profile(const char *name)
{
    int idx = terminal_find_profile(name);
    if (idx < 0)
    {
        terminal_write("Profile not found\r\n");
        return;
    }
    profiles[idx].used = false;
    terminal_write("Profile deleted\r\n");
}

static void terminal_list_profiles(void)
{
    bool any = false;
    for (int i = 0; i < TERMINAL_MAX_PROFILES; ++i)
    {
        if (profiles[i].used)
        {
            any = true;
            terminal_write("- %s\r\n", profiles[i].name);
        }
    }
    if (!any)
        terminal_write("No profiles saved\r\n");
}

static void terminal_set_default_profile(const char *name)
{
    int idx = terminal_find_profile(name);
    if (idx < 0)
    {
        terminal_write("Profile not found\r\n");
        return;
    }
    strncpy(default_profile_name, name, TERMINAL_PROFILE_NAME_LEN - 1);
    default_profile_name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
    flash_page.data.magic = TERMINAL_PROFILE_FLASH_MAGIC;
    flash_page.data.version = TERMINAL_PROFILE_FLASH_VERSION;
    strncpy(flash_page.data.default_profile_name, default_profile_name, TERMINAL_PROFILE_NAME_LEN - 1);
    flash_page.data.default_profile_name[TERMINAL_PROFILE_NAME_LEN - 1] = '\0';
    terminal_write("Default profile set\r\n");
}

static void terminal_stats(const char *sub)
{
    if (!sub)
    {
        terminal_write("Stats not available\r\n");
        return;
    }
    if (strcmp(sub, "cpu") == 0)
    {
        terminal_write("CPU stats not implemented\r\n");
        return;
    }
    if (strcmp(sub, "ram") == 0)
    {
        terminal_write("RAM stats not implemented\r\n");
        return;
    }
    if (strcmp(sub, "flash") == 0)
    {
        terminal_write("Flash stats not implemented\r\n");
        return;
    }
    if (strcmp(sub, "audio") == 0)
    {
        terminal_write("Audio stats not implemented\r\n");
        return;
    }
    terminal_write("Unknown stats command\r\n");
}

static void terminal_system(const char *sub)
{
    if (!sub)
    {
        terminal_write("Missing system command\r\n");
        return;
    }
    if (strcmp(sub, "info") == 0)
    {
        terminal_write("System info not implemented\r\n");
        return;
    }
    if (strcmp(sub, "save") == 0)
    {
        terminal_write("System save not implemented\r\n");
        return;
    }
    if (strcmp(sub, "reset") == 0 || strcmp(sub, "reboot") == 0)
    {
        terminal_write("Rebooting system...\r\n");
        watchdog_reboot(0, 0, 0);
        return;
    }
    terminal_write("Unknown system command\r\n");
}

static void terminal_dac(const char *sub)
{
    if (!sub)
    {
        terminal_write("Missing dac command\r\n");
        return;
    }
    if (strcmp(sub, "info") == 0 || strcmp(sub, "status") == 0)
    {
        terminal_write("DAC state:\r\n");
        terminal_write(" Enabled: %s\r\n", dacamp_is_enabled() ? "Yes" : "No");
        terminal_write(" Sample Rate: %u Hz\r\n", dacamp_get_current_sample_rate());
        return;
    }
    if (strcmp(sub, "version") == 0)
    {
        terminal_write("DAC firmware version not implemented\r\n");
        return;
    }
    if (strcmp(sub, "uptime") == 0)
    {
        uint64_t uptime_ms = to_us_since_boot(get_absolute_time()) / 1000;
        terminal_write("Uptime: %llu ms\r\n", (unsigned long long)uptime_ms);
        return;
    }
    terminal_write("Unknown dac command\r\n");
}

static void terminal_handle_command(char *cmd)
{
    char *token = strtok(cmd, " \t\r\n");
    if (!token) return;

    if (strcmp(token, "help") == 0)
    {
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub)
            terminal_print_help();
        else
            terminal_print_help_command(sub);
        return;
    }

    if (strcmp(token, "eq") == 0)
    {
        char *action = strtok(NULL, " \t\r\n");
        if (!action) { terminal_write("Missing eq command\r\n"); return; }
        if (strcmp(action, "info") == 0)
        {
            char buf[512];
            eq_get_status(buf, sizeof(buf));
            terminal_write(buf);
            return;
        }
        if (strcmp(action, "reset") == 0)
        {
            eq_reset();
            terminal_write("EQ reset\r\n");
            return;
        }
        if (strcmp(action, "set") == 0)
        {
            char *b = strtok(NULL, " \t\r\n");
            char *f = strtok(NULL, " \t\r\n");
            char *g = strtok(NULL, " \t\r\n");
            char *q = strtok(NULL, " \t\r\n");
            if (!b || !f || !g || !q) { terminal_write("Usage: eq set <band> <freq> <gain> <Q>\r\n"); return; }
            int bi = atoi(b) - 1;
            float freq = atof(f);
            float gain = atof(g);
            float qv = atof(q);
            if (!eq_set_band(bi, freq, gain, qv))
                terminal_write("Invalid EQ parameters\r\n");
            else
                terminal_write("EQ band set\r\n");
            return;
        }
        if (strcmp(action, "enable") == 0 || strcmp(action, "disable") == 0)
        {
            char *b = strtok(NULL, " \t\r\n");
            if (!b) { terminal_write("Usage: eq enable <band>\r\n"); return; }
            int bi = atoi(b) - 1;
            bool enable = strcmp(action, "enable") == 0;
            if (!eq_enable_band(bi, enable))
                terminal_write("Invalid band\r\n");
            else
                terminal_write(enable ? "EQ band enabled\r\n" : "EQ band disabled\r\n");
            return;
        }
        if (strcmp(action, "save-profile") == 0)
        {
            char *name = strtok(NULL, " \t\r\n");
            terminal_save_profile(name);
            return;
        }
        if (strcmp(action, "load-profile") == 0)
        {
            char *name = strtok(NULL, " \t\r\n");
            terminal_load_profile(name);
            return;
        }
        if (strcmp(action, "list-profiles") == 0)
        {
            terminal_list_profiles();
            return;
        }
        if (strcmp(action, "delete-profile") == 0)
        {
            char *name = strtok(NULL, " \t\r\n");
            terminal_delete_profile(name);
            return;
        }
        if (strcmp(action, "default") == 0)
        {
            char *name = strtok(NULL, " \t\r\n");
            terminal_set_default_profile(name);
            return;
        }
        terminal_write("Unknown eq command\r\n");
        return;
    }

    if (strcmp(token, "volume") == 0)
    {
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub) { terminal_write("Missing volume command\r\n"); return; }
        if (strcmp(sub, "info") == 0)
        {
            int vol = dacamp_get_volume_percent();
            terminal_write("Volume: %d%%\r\n", vol);
            terminal_write("Muted: %s\r\n", dacamp_is_muted() ? "Yes" : "No");
            return;
        }
        if (strcmp(sub, "set") == 0)
        {
            char *v = strtok(NULL, " \t\r\n");
            if (!v) { terminal_write("Usage: volume set <0-100>\r\n"); return; }
            int vol = atoi(v);
            if (vol < 0 || vol > 100) { terminal_write("Volume out of range\r\n"); return; }
            dacamp_set_volume(vol);
            terminal_write("Volume set\r\n");
            return;
        }
        if (strcmp(sub, "up") == 0 || strcmp(sub, "down") == 0)
        {
            char *stepText = strtok(NULL, " \t\r\n");
            int step = stepText ? atoi(stepText) : 5;
            if (step < 0) step = 5;
            if (strcmp(sub, "down") == 0)
                step = -step;
            dacamp_adjust_volume(step);
            terminal_write("Volume adjusted\r\n");
            return;
        }
        if (strcmp(sub, "mute") == 0)
        {
            dacamp_set_mute(true);
            terminal_write("Volume muted\r\n");
            return;
        }
        if (strcmp(sub, "unmute") == 0)
        {
            dacamp_set_mute(false);
            terminal_write("Volume unmuted\r\n");
            return;
        }
        terminal_write("Unknown volume command\r\n");
        return;
    }

    if (strcmp(token, "dac") == 0)
    {
        char *sub = strtok(NULL, " \t\r\n");
        terminal_dac(sub);
        return;
    }

    if (strcmp(token, "system") == 0)
    {
        char *sub = strtok(NULL, " \t\r\n");
        terminal_system(sub);
        return;
    }

    if (strcmp(token, "stats") == 0)
    {
        char *sub = strtok(NULL, " \t\r\n");
        terminal_stats(sub);
        return;
    }

    terminal_write("Unknown command\r\n");
}

void terminal_process_line(char *line)
{
    char *p = line;
    while (*p) { *p = (char)tolower((unsigned char)*p); p++; }
    terminal_handle_command(line);
}

void terminal_task(void)
{
    if (!tud_cdc_connected()) return;

    if (tud_cdc_available())
    {
        char c;
        while (tud_cdc_read(&c, 1) == 1)
        {
            if (c == '\r' || c == '\n')
            {
                if (line_length > 0)
                {
                    line_buffer[line_length] = '\0';
                    terminal_process_line(line_buffer);
                    line_length = 0;
                }
                print_prompt();
            }
            else if (c == '\b' || c == 127)
            {
                if (line_length) line_length--;
            }
            else if (line_length + 1 < sizeof(line_buffer))
            {
                line_buffer[line_length++] = c;
            }
        }
    }
}

void terminal_print_prompt(void)
{
    print_prompt();
}
