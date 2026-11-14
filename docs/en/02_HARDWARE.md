# 02 — Hardware and Wiring

[English index](00_INDEX.md) · [Menuconfig instructions](BUILD_GUIDE.md#initial-configuration-and-menuconfig)

The target is ESP32-S3-WROOM-1-N16R8: 16 MB flash and 8 MB Octal PSRAM.
Wiring is based on [the reference schematic](../../references/YR.kicad_sch),
with the user's confirmed encoder correction. Do not infer pins from older
Arduino definitions.

| Device | Signal | Default GPIO | Notes |
| --- | --- | --- | --- |
| SSD1322 | MOSI | 11 | Four-wire SPI |
| SSD1322 | SCLK | 12 | SPI2 by default |
| SSD1322 | CS | 9 | Display chip select |
| SSD1322 | DC | 10 | Data/command |
| SSD1322 | RESET | −1 | No GPIO-controlled reset |
| PCM5102A | DATA OUT | 48 | ESP32 output to DAC |
| PCM5102A | BCLK | 47 | I²S bit clock |
| PCM5102A | LRCLK/WS | 21 | I²S channel clock |
| Encoder | A | 5 | Actual wiring confirmed by the user |
| Encoder | B | **6** | The reference schematic instead shows B=16 |
| Encoder | Button | 7 | Single encoder only |

The historical `config/yr_031.defaults` still represents the schematic's B=16.
The normal `sdkconfig.defaults` uses 5/6/7. Select according to the physical board,
not the filename of a profile. The second encoder's schematic GPIO6 conflicts
with the active encoder's B pin and must not be assumed available.

## Display and DAC

The user's display is a 3.12-inch msalamon OLED_312_YELLOW module using SSD1322,
256×64 pixels and four-wire SPI, with R5/R8 configured accordingly. The display
connector's CHIP_PU is tied to EN rather than a separate software RESET GPIO.
RESET=−1 relies on the board's reset/power sequence. Verify a cold start as well
as a software reset when adapting hardware.

The initial display profile uses SPI2 at 10 MHz, column offset 28 in four-pixel
units, and remap byte 20 (0x14). These values are panel-specific. Rendering is
4 bits per pixel with 16 grayscale levels.

The DAC output is Philips I²S, 48 kHz stereo, 32-bit slots, without MCLK.
The physical PCM5102A module must be configured for this format and unmuted.
Correct firmware pin assignments do not establish correct module jumpers,
power, analog wiring or output behavior.

## GPIO policy

The build and startup validators reject duplicate assignments, invalid GPIOs,
GPIO22–25, N16R8 flash/PSRAM pins 26–37, and strapping pins 0/3/45/46.
GPIO19/20 are protected while USB is reserved or enabled; GPIO43/44 are protected
for the default UART console. Custom console pins are checked for conflicts.
Only supported absent signals, such as display RESET, may use −1.

```sh
python3 tools/check_config.py sdkconfig --platform
```

The validator also requires target esp32s3, 16 MB flash, 80 MHz Octal PSRAM with
malloc access and memory testing, and a 240 MHz CPU. Check actual connector
availability separately. Detailed menu fields are in the [build guide](BUILD_GUIDE.md).

## Board acceptance

Check power/reset, OLED orientation and grayscale, encoder direction and detents,
left/right DAC channels, clean PLAY/STOP and volume changes, and USB/UART access.
Run optional test sources only through the documented test profile. A successful
host validator does not test electrical connections. The original board reports
remain in the [Polish hardware archive](../HARDWARE_BRINGUP.md).
