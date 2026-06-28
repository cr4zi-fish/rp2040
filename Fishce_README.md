# Fishce DAC

Low-Latency USB DAC/AMP Firmware for IEMs based on RP2040-Zero and PCM5102A.

## Overview

Fishce DAC is an open-source USB DAC firmware project built around RP2040-Zero and PCM5102A.

Goals:

- TinyUSB (mandatory)
- USB Audio Class 1 (UAC1)
- USB CDC terminal
- 8-band Parametric EQ
- Flash profile storage
- Low-latency audio for gaming
- Modular architecture
- Future expandability

## Software Stack

Mandatory:

- C/C++
- Pico SDK
- TinyUSB
- CMake
- ARM GCC
- UF2 output

TinyUSB shall be the primary USB implementation.

## USB

Composite device:

- UAC1 Audio
- CDC Terminal

Supported platforms:

- Windows
- Linux
- macOS
- Android
- iPadOS
- iOS (best effort)

Device Name:

```text
Fishce DAC
```

Prompt:

```bash
fishce-dac>
```

## Audio

- PCM5102A via I2S
- 16-bit minimum
- Optional 24-bit
- 44.1 kHz
- 48 kHz
- Stereo output
- DMA optimized
- Double buffering
- Target latency <20ms
- Ideal latency <10ms

## DSP

### 8-Band PEQ

Per band:

- Frequency
- Gain
- Q
- Enable/Disable

Ranges:

- 20Hz–20kHz
- -12dB to +12dB
- Q 0.3–10

## Profiles

Store in internal flash.

Functions:

- save
- load
- delete
- list
- rename
- duplicate
- default profile

## Volume

- set
- mute
- unmute
- persistent storage

## Terminal Commands

```bash
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

dac info
dac status
dac version
dac uptime

system info
system save
system reset
system reboot

stats
stats cpu
stats ram
stats flash
stats audio
```

## Example

```text
=== fishce-dac system info ===

[Volume]
Current : 75%
Muted   : No

[EQ]
Band 1 : 80Hz -3dB Q1.0

[Profiles]
bass-boost
flat
treble

[DAC]
48000Hz
16bit
Streaming Yes

[Hardware]
Firmware v1.0.0
CPU 42%
RAM 182KB
Flash 48KB / 2MB
```

## Hardware

### RP2040-Zero

- USB Type-C
- BOOT
- RESET
- 2MB Flash

### PCM5102A

Pins:

- SCK
- BCK
- DIN
- LRCK
- VIN
- GND
- FLT
- DEMP
- XSMT
- FMT
- ROUT
- LROUT

Integrated 3.5mm jack.

## Suggested Pin Mapping

```text
GP18 -> BCK
GP19 -> LRCK
GP20 -> DIN

3V3 -> VIN
GND -> GND
```

## BOOT Button

Short press:
Next profile

Long press:
Previous profile

Double press:
Default profile

Hold at startup:
Safe mode

## Repository Layout

```text
Fishce-DAC/

README.md
LICENSE
.gitignore
CMakeLists.txt

src/
include/
audio/
usb/
eq/
terminal/
storage/
profiles/
system/
drivers/
docs/
examples/
firmware/
```

## Documentation

Required docs:

```text
docs/

CLI.md
USB.md
PEQ.md
STORAGE.md
ARCHITECTURE.md
HARDWARE.md
KNOWN_ISSUES.md
ROADMAP.md
```

## Build

```bash
mkdir build
cd build

cmake ..

make -j8
```

Expected:

```text
fishce-dac.uf2
```

## Flashing

Hold BOOT.

Connect USB.

Copy:

```text
fishce-dac.uf2
```

Device reboots automatically.

## Troubleshooting

### Not detected

Check:

- TinyUSB
- descriptors
- cable
- power

### No audio

Check:

- BCK
- LRCK
- DIN
- PCM5102A power

### Distortion

Reduce:

- gain
- PEQ boost
- digital volume

### Audio dropouts

Inspect:

- DMA
- CPU load
- buffering
- USB synchronization

## Known Limitations

- RP2040 has no DSP accelerator
- 44.1kHz may require tuning
- Very low latency can reduce stability
- Flash endurance is finite
- 24-bit DSP may increase CPU usage

## Future Features

- OLED
- SH1106
- Encoder
- Physical buttons
- WebUI
- REST API
- ESP32 companion module
- Bluetooth controller
- OTA updates

## Acceptance Criteria

Firmware should:

- compile successfully
- generate UF2
- enumerate as UAC1
- expose CDC terminal
- stream audio
- save settings
- restore settings
- survive reboot
- support future expansion

Fix all discoverable software issues before release.

Hardware testing results will be reported later for further debugging.

## License

MIT License recommended.

## Roadmap

### v1.0

- TinyUSB
- UAC1
- 8 PEQ
- Flash profiles
- CLI
- Volume
- BOOT switching

### v1.1

- OLED

### v1.2

- Encoder

### v1.3

- Web control

### v2.0

- UAC2
- Advanced DSP
- FIR
- Convolution
