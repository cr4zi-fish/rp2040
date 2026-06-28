# Fishce DAC

Low-latency USB DAC firmware for RP2040-Zero with PCM5102A support.

## Overview

This project implements a USB audio device with an integrated USB CDC terminal for command control. It is designed for the RP2040-Zero and a PCM5102A DAC.

Features:
- USB Audio Class 1 output
- USB CDC terminal for runtime control
- EQ profile save/load via flash
- Volume/mute control
- Builds to UF2 for RP2040-Zero

## Hardware

### RP2040-Zero <-> PCM5102A wiring

The RP2040-Zero drives the PCM5102A using I2S signals.

Recommended wiring:

- `GP18` -> `BCK` (Bit Clock)
- `GP19` -> `LRCK` (Word Select)
- `GP20` -> `DIN` (Serial Data In)
- `3V3` -> `VIN`
- `GND` -> `GND`

Optional PCM5102A control pins (if used):
- `FMT`: select I2S mode
- `FLT`, `DEMP`, `XSMT`: leave as specified by your module

> Note: This firmware project is written for the RP2040-Zero platform and expects the PCM5102A to be connected to the RP2040 I2S pins listed above.

### Power

- Power the PCM5102A from the RP2040-Zero `3V3` rail.
- Common ground is required.

## Build instructions

From the repository root:

```bash
cd /workspaces/rp2040/src
mkdir -p build
cd build
cmake ..
make
```

After successful build, the UF2 file is generated at:

```text
src/build/rp2040_dac_amp.uf2
```

## Flash firmware to RP2040-Zero

1. Connect the RP2040-Zero to your PC with USB.
2. Put the device into bootloader mode by holding `BOOT` while plugging in, or by using the BOOTSEL button.
3. Copy `src/build/rp2040_dac_amp.uf2` to the RP2040-Zero mass storage drive.
4. The board will reboot and run the firmware.

## Windows terminal connection

### Using PuTTY

1. Open Device Manager.
2. Under `Ports (COM & LPT)`, find the RP2040 serial port like `COM3` or `COM4`.
3. In PuTTY:
   - Connection type: `Serial`
   - Serial line: `COMx`
   - Speed: `115200`
4. Click `Open`.
5. Press `Enter` once or twice.
6. Type `help` and press `Enter`.

If you do not see typed characters, enable local echo in PuTTY:
- `Terminal` → `Local echo` → `Force on`

### Common issues

- `Access is denied`: the COM port is busy or another app already opened it.
- Blank screen after open: press `Enter`, ensure the firmware is running, and use the correct COM port.
- If the board is in bootloader mode, the terminal will not show the Fishce DAC prompt.

## Terminal commands

Use the built-in CDC terminal for audio control:

```text
help
help <command>

eq info
eq set <band> <freq> <gain> <Q>
eq enable <band>
eq disable <band>
eq reset

eq save-profile <name>
eq load-profile <name>
eq list-profiles
eq delete-profile <name>
eq default <name>

volume info
volume set <0-100>
volume up <step>
volume down <step>
volume mute
volume unmute

system info
system save
system reset
system reboot
```

## Example terminal session

```text
fishce-dac> help
fishce-dac> volume set 75
fishce-dac> eq info
fishce-dac> eq save-profile bass-boost
```

## Notes

- The published release includes `src/build/rp2040_dac_amp.uf2`.
- Use a good USB cable and ensure the RP2040-Zero is not in BOOTSEL mode when opening the terminal.
- This project is made with good vibes and a strong focus on "vibe coding".

## Repository layout

- `src/` — firmware source files
- `src/build/` — generated build artifacts
- `src/main.c` — firmware entrypoint
- `src/dacamp.c` — audio output engine
- `src/terminal.c` — CDC terminal command parsing
- `src/eq.c` — EQ filter processing
