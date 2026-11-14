# StreamOO — Menuconfig, Building and Flashing

[English index](00_INDEX.md) · [Polski](../BUILD_GUIDE.md)

Updated: 2026-11-14. Target hardware: ESP32-S3-WROOM-1-N16R8, SSD1322,
PCM5102A; native **ESP-IDF 6.1**. For operating the radio, see the
[user guide](USER_GUIDE.md). Unless stated otherwise, run commands from the
project root in a terminal with the ESP-IDF environment activated.

## Development environment

Install **ESP-IDF 6.1** and its ESP32-S3 tools. The official
[Espressif v6.1 setup guide](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/get-started/index.html)
covers ESP-IDF Installation Manager (EIM) on Linux, Windows and macOS.
The project checks for version 6.1 and target `esp32s3` in CMake;
other SDK versions do not pass this check.

On Linux/macOS, activate the installed environment in every new terminal.
For an SDK installed at the following location:

```sh
source "$HOME/.espressif/v6.1/esp-idf/export.sh"
idf.py --version
```

Adjust the path to your installation, or use the EIM activation script.
On Windows, open the installer's ESP-IDF terminal. Use `python` instead of
`python3` if that is the interpreter name in your environment. Multiline commands
using `\` below are for Linux/macOS shells; enter those commands on one line
on Windows.

You need Git, the IDF Python environment, CMake, Ninja and the SDK's Xtensa
toolchain. Initial configuration may download managed components and therefore
needs Internet access. Component `idf_component.yml` manifests and
`dependencies.lock` define dependency versions. Do not update them as part of
a routine build.

Use a checkout containing the implementation. As recorded for this documentation
on 2026-11-14, it is on `feature/streamoo-implementation`, proposed for `main`
in [PR #1](https://github.com/PROTONMM/StreamOO/pull/1). Once merged, `main` can
be used. Check your branch with `git branch --show-current`.

To obtain an implementation checkout:

```sh
git clone --branch feature/streamoo-implementation https://github.com/PROTONMM/StreamOO.git
cd StreamOO
```

## Where settings are changed

| Setting | Location | Rebuild required? |
| --- | --- | --- |
| GPIO, encoder Mode A/B, buffer, FFT, FLAC decoder, diagnostics | `idf.py menuconfig` → StreamOO Configuration | Yes |
| Shared setup AP password | Private file created by `tools/configure_network.py` | Yes |
| Home Wi-Fi profiles, playlist, time zone, autoplay, `.local` name, list timeout | Radio web interface | No |
| Volume and visualization | Encoder; volume also through the web interface | No |
| Web theme | Page header; browser storage | No |

`sdkconfig.defaults` provides initial values. The generated local `sdkconfig`
**takes precedence**. Updating defaults or checking out a newer project version
does not overwrite all previous choices. `menuconfig` saves to `sdkconfig`,
not to the device. Its changes take effect after rebuilding and flashing.

## Private AP password

Before the first build intended to use Wi-Fi:

```sh
python3 tools/configure_network.py
python3 tools/configure_network.py --check
```

The script asks for the password twice without echoing it. It requires 8–63
printable ASCII characters. Use the same agreed AP password for builds for
different devices; the SSID differs because it includes a MAC-derived suffix.

The file is stored outside the repository at `~/.config/streamoo/network.json`,
or under `$XDG_CONFIG_HOME/streamoo/` when that variable is set. Do not put the
password in command arguments, `sdkconfig` or defaults. The script rejects
locations inside the repository, including the ignored `secrets/` directory.

After first creating the file, run `idf.py reconfigure build`. You do not need
to enter the password again for subsequent builds. If the default file is absent,
firmware can build but **networking remains disabled**, even with
`STREAMOO_NETWORK_ENABLED=y`. There is no fallback open AP. An invalid explicitly
selected file causes configuration to fail.

Optionally select another path outside the checkout:

```sh
python3 tools/configure_network.py --file /path/outside/repo/network.json
idf.py -DSTREAMOO_NETWORK_SECRETS_FILE=/path/outside/repo/network.json reconfigure build
```

Replace the example path with your actual path. The choice is retained in that
build directory's CMake cache. To return to the default file, run
`idf.py -DSTREAMOO_NETWORK_SECRETS_FILE= reconfigure build`.
The password is embedded in a private header under `build/` and in the firmware
binary. Do not publish artifacts containing your private key as generic examples.

## Initial configuration and menuconfig

For a new checkout without a configuration to preserve:

```sh
idf.py set-target esp32s3
idf.py menuconfig
```

For an existing project with a valid `sdkconfig`, simply run:

```sh
idf.py menuconfig
```

`set-target` clears the build directory and regenerates configuration, saving
the previous configuration as `sdkconfig.old`. Do not run it for every update.
See the [official idf.py reference](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/tools/idf-py.html)
for command behavior.

Select **StreamOO Configuration**. Navigate with arrow keys and Enter, toggle
options with Space, search for symbols with `/`, and open help with `?`.
Use **Save**, then **Exit**. Search examples: `STREAMOO_ENCODER_MODE` or
`STREAMOO_FLAC_DR`. Dependent options appear when their prerequisites are enabled.

### Normal radio profile

| Menu option | Symbol after `CONFIG_` | Normal setting |
| --- | --- | --- |
| Enable network radio and Wi-Fi configuration | `STREAMOO_NETWORK_ENABLED` | Enabled |
| Enable encoder input | `STREAMOO_ENCODER_ENABLED` | Enabled |
| Enable interactive radio UI | `STREAMOO_UI_ENABLED` | Enabled; requires encoder and display |
| Hardware: SSD1322 → Enable SSD1322 display | `STREAMOO_DISPLAY_ENABLED` | Enabled |
| Reserve GPIO19/20 for USB debugging | `STREAMOO_RESERVE_USB` | Enabled |
| Prepare hardware AES/SHA before I2S | `STREAMOO_CRYPTO_EARLY_INIT` | Enabled with hardware AES/SHA |
| Compile detailed UART diagnostics and runtime profiling | `STREAMOO_DIAGNOSTICS` | Disabled |
| Compile hardware test sources and synthetic screens | `STREAMOO_HARDWARE_TESTS` | Disabled |

Preparing crypto before I²S is part of the normal profile; do not disable it as
an unnecessary test. SDK settings outside the StreamOO menu, recorded in defaults:
16 MB flash, 80 MHz Octal PSRAM with memory test and malloc allocation, 240 MHz
CPU, custom `partitions.csv`, size optimization, hardware AES/SHA, TCP receive
window 23040 bytes and receive mailbox 18. Normal logging uses WARN with selected
application information. The validator checks the N16R8 profile and GPIO mapping.

### Hardware: SSD1322

| Field | Symbol after `CONFIG_STREAMOO_` | Initial value |
| --- | --- | --- |
| SPI host | `LCD_SPI_HOST` | 2 (SPI2); alternatively 3 |
| MOSI GPIO | `LCD_MOSI` | 11 |
| SCLK GPIO | `LCD_SCLK` | 12 |
| CS GPIO | `LCD_CS` | 9 |
| DC GPIO | `LCD_DC` | 10 |
| RESET GPIO | `LCD_RST` | −1: no GPIO-controlled reset |
| SPI clock (MHz) | `LCD_SPI_CLOCK_MHZ` | 10; range 1–10 |
| Experimental panel column offset | `LCD_COLUMN_OFFSET` | 28, in four-pixel units |
| Experimental A0 remap byte (decimal) | `LCD_REMAP` | 20, or 0x14 |

This profile matches the project's 256×64 panel using four-wire SPI. Change
offset/remap only when adapting another panel and inspecting the resulting image.
RESET=−1 requires a hardware-provided reset. See [hardware and wiring](02_HARDWARE.md).

### Hardware: PCM5102A (I2S)

| Field | Symbol after `CONFIG_STREAMOO_` | GPIO |
| --- | --- | --- |
| I2S DATA OUT GPIO | `I2S_DOUT` | 48 |
| I2S BCLK GPIO | `I2S_BCLK` | 47 |
| I2S LRCLK GPIO | `I2S_LRCLK` | 21 |

No separate MCLK is used. Leave tone and file-source tests in this menu disabled
for normal radio operation.

### Hardware: Encoder

| Field | Symbol after `CONFIG_STREAMOO_` | Value |
| --- | --- | --- |
| Encoder A GPIO | `ENCODER_GPIO_A` | 5 |
| Encoder B GPIO | `ENCODER_GPIO_B` | **6** |
| Encoder button GPIO | `ENCODER_GPIO_BUTTON` | 7 |
| Enable internal pull-ups | `ENCODER_INTERNAL_PULLUP` | Disabled for the user's hardware |
| Invert encoder direction | `ENCODER_REVERSED` | Enabled |
| Use half-quadrature detents | `ENCODER_HALF_QUADRATURE` | Enabled |
| Encoder interaction mode | `ENCODER_MODE_A` / `ENCODER_MODE_B` | A |

Mode A: a short press opens the list during playback or starts PLAY after STOP;
a long press stops. Mode B: short press PLAY/STOP, long press list. See the
[complete gesture table](USER_GUIDE.md#rotary-encoder).

The default **5/6/7** matches the wiring confirmed by the user. Historical
`config/yr_031.defaults` follows the schematic's **B=16**; do not apply it without
checking your board. If rotation is reversed, change `Invert encoder direction`.
Match pull-ups and quadrature settings to the actual encoder.

### Audio and DSP

| Field | Symbol after `CONFIG_STREAMOO_` | Normal value / range |
| --- | --- | --- |
| FLAC decoder | `FLAC_DR` / `FLAC_MICRO` / `FLAC_ESPRESSIF` | dr_flac 0.13.4 |
| Compressed stream ring size (KiB) | `STREAM_BUFFER_KIB` | 256; range 64–2048 |
| FFT size | `FFT_1024` / `FFT_2048` | 1024; optionally 2048 |
| Use ESP-DSP sc16 SIMD FFT on ESP32-S3 | `DSP_ENABLE_SIMD` | Enabled |

A larger buffer uses more PSRAM; changing capacity alone does not guarantee
dropout-free playback. FFT 2048 increases analysis cost. Changes require rebuilding
and listening tests. Choosing a visualization remains an encoder gesture,
independent of FFT size. Alternative FLAC backends have different limitations
and validation histories; see [format validation](FORMAT_VALIDATION.md).

### GPIO validation

After saving `menuconfig`:

```sh
python3 tools/check_config.py sdkconfig --platform
```

Expected output: `N16R8 GPIO map check passed`. CMake also runs this check
automatically. It rejects collisions, nonexistent GPIO 22–25, memory-reserved
26–37 and strapping pins 0/3/45/46. GPIO 19/20 are protected when USB is reserved
or used; 43/44 are protected when using the default UART console. Do not bypass
validator errors. Also check whether the selected signal is physically accessible
on your board's connectors.

## Building

```sh
idf.py reconfigure build
```

For subsequent code changes, `idf.py build` is sufficient. Use `reconfigure`
again after first creating the private file or changing CMake variables.

Build outputs are in the ignored `build/` directory:

| File | Purpose |
| --- | --- |
| `streamoo.bin` | Firmware application |
| `streamoo.elf` | Symbols, including those needed for crash analysis |
| `bootloader/bootloader.bin` | Bootloader |
| `partition_table/partition-table.bin` | Partition table |
| `flash_args` | Flash arguments for this build |
| `config/sdkconfig.h` | Actual generated build configuration |

Use `idf.py size` or `idf.py size-components` to inspect size. No separate
LittleFS image is required: the radio stores the station catalog after web
import, and web assets are embedded in the application.

## Flashing and updating

Connect the board with a data cable and choose its actual port, for example
`/dev/ttyUSB0` or `/dev/ttyACM0` on Linux, `/dev/cu.usbmodem…` on macOS,
or `COM4` on Windows. Close other programs using that port.

```sh
idf.py -p /dev/ttyUSB0 flash monitor
```

This builds any required files and flashes the bootloader, partition table and
application. Exit the monitor with **Ctrl+]**. To monitor without flashing:
`idf.py -p /dev/ttyUSB0 monitor`. On Windows, replace the port with e.g. `COM4`.
If automatic bootloader entry fails, use your board's BOOT/RESET sequence and retry.

Current [partition layout](../../partitions.csv):

| Partition | Offset | Size |
| --- | --- | --- |
| NVS | `0x9000` | `0x6000` |
| PHY | `0xf000` | `0x1000` |
| factory (application) | `0x10000` | `0xEF0000` |
| storage (LittleFS) | `0xF00000` | `0x100000` |

There is one large application, **no OTA and no rollback slot**. Normal `flash`
with the same layout does not flash NVS/LittleFS images and preserves settings
and stations. Do not use `erase-flash` for routine updates. Export the catalog
before migrating from a historical layout: moving LittleFS may require reimport.
For the first flash and partition changes, use the complete `flash` operation
rather than only `app-flash`.

After restarting, check Wi-Fi, a known working station, PLAY/STOP, volume, display
and web interface. Reload the web page after updating. See [first startup](USER_GUIDE.md#first-startup).

## Diagnostics and tests

Host tests run on the computer without flashing the device:

```sh
python3 -m unittest discover -s tests -v
```

They require a host C compiler and Python. Some frontend tests use Node.js.
NumPy enables comparison against an independent FFT reference. Missing optional
tools may cause SKIP rather than a passed check. Browser tests also need local
sockets and Chrome at `/usr/bin/google-chrome`, the path currently used by the scripts:

```sh
python3 tools/check_portal_browser.py
python3 tools/check_portal_layout.py
```

These scripts use synthetic data and a local server. They do not test a physical
phone, radio or station availability.

For detailed logs from the radio, enable `Compile detailed UART diagnostics and
runtime profiling`. For long sessions, select the FreeRTOS U64 runtime counter
and esp_timer clock. Alternatively, create a separate diagnostic profile:

```sh
idf.py -B build/diagnostics \
  -D SDKCONFIG=sdkconfig.diagnostics.local.defaults \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.diagnostics.defaults' build
```

Despite its `.local.defaults` suffix, this is a new private full `sdkconfig`
ignored by Git. The overlay applies when it is first created. Custom GPIO changes
from the main `sdkconfig` are not automatically copied; inspect this profile
before flashing. Edit it with the same command, replacing `build` with `menuconfig`.
To flash diagnostics, retain the same `-B` and `-D` options and replace the final
action with `-p PORT flash monitor`. `-B` alone does not select a separate
configuration. To restore normal firmware, build and flash from the main `build/`
configuration.

`Compile hardware test sources and synthetic screens` also enables diagnostics
and exposes test options. Enabling it alone does not start a tone.

| Test | StreamOO Configuration settings |
| --- | --- |
| Encoder-controlled AAC/MP3 file or sine | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=n`, `UI_ENABLED=y`, `AUDIO_CONTROL_TEST=y`; select the source in the I2S menu |
| Finite OLED and I2S channel test | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=n`, `UI_ENABLED=n`, `DISPLAY_ENABLED=y`, `DISPLAY_TEST=y`, `I2S_SMOKE_TEST=y` |
| OLED pulsation experiment during playback | `HARDWARE_TESTS=y`, `NETWORK_ENABLED=y`, `UI_ENABLED=y`, `LCD_TIMING_PROBE=y` |

The abbreviated symbols in this table have the `CONFIG_STREAMOO_` prefix.
Networking takes priority over test audio. Before returning to normal operation,
disable hardware tests and diagnostics, and enable networking, display, encoder
and UI. See the [test and delivery guide](12_TESTS_DELIVERY.md).

For FLAC comparisons, use the wrapper that verifies the selected backend before
flashing:

```sh
python3 tools/build_flac.py dr build
# Optional: flash this specific variant.
python3 tools/build_flac.py dr flash --port /dev/ttyUSB0
```

Supported names are `dr`, `micro` and `esp`. Do not use only `-B build/flac-dr`
to select a decoder. These are separate profiles; normal `idf.py build` uses the
main `sdkconfig`. Before sharing logs, remove private URLs, network names and
station names. Do not attach the private AP file or generated secret header.

## Build troubleshooting

| Symptom | Action |
| --- | --- |
| `idf.py` missing or wrong Python | Activate the intended IDF environment in the current terminal |
| `StreamOO requires ESP-IDF 6.1` | Check `idf.py --version` and the active SDK; retain the CMake check |
| N16R8 or GPIO validation error | Correct the reported options in `menuconfig`; check which `SDKCONFIG` the build uses |
| Defaults changes do not take effect | Edit the existing configuration through `menuconfig`; defaults do not forcibly overwrite it |
| Invalid private AP configuration | Run `configure_network.py --check`; check the location and any path cached in CMake |
| Build succeeds but Wi-Fi is absent | Check the private AP file and `NETWORK_ENABLED`; after creating the file, reconfigure, build and flash the new image |
| Dependency downloads fail | Check registry access; keep the manifests and lock rather than substituting arbitrary versions |
| Snap CMake fails due to its environment | On the development workstation, `export PATH="/usr/bin:$PATH"` after IDF activation selects working system CMake/Ninja when installed there |
| Serial port busy or permission denied | Close other monitors/programs; check the data cable, port name and user access to serial devices |

When distributing firmware, include [LICENSE](../../LICENSE),
[licensing scope](../../LICENSING.md) and applicable
[third-party notices](../../THIRD_PARTY_NOTICES.md). The application's MIT license
does not replace the terms of its libraries and assets.
