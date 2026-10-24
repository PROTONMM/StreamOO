#!/usr/bin/env python3
"""Validate StreamOO N16R8 GPIO assignments before touching peripherals."""
import sys
from pathlib import Path

PINS = ['LCD_MOSI', 'LCD_SCLK', 'LCD_CS', 'LCD_DC', 'LCD_RST',
        'I2S_DOUT', 'I2S_BCLK', 'I2S_LRCLK',
        'ENCODER_GPIO_A', 'ENCODER_GPIO_B', 'ENCODER_GPIO_BUTTON']


def parse(path):
    data = {}
    for line in Path(path).read_text().splitlines():
        if line.startswith('CONFIG_') and '=' in line:
            key, value = line.split('=', 1)
            data[key.removeprefix('CONFIG_').removeprefix('STREAMOO_')] = value
    return data


def check(data):
    seen, errors = {}, []
    usb = any(data.get(key) == 'y' for key in (
        'RESERVE_USB', 'ESP_CONSOLE_USB_SERIAL_JTAG', 'ESP_CONSOLE_USB_CDC',
        'ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG'))
    uart = data.get('ESP_CONSOLE_UART_DEFAULT') == 'y'
    names = list(PINS)
    if data.get('ESP_CONSOLE_UART_CUSTOM') == 'y':
        names += ['ESP_CONSOLE_UART_TX_GPIO', 'ESP_CONSOLE_UART_RX_GPIO']
    for name in names:
        if name not in data:
            errors.append(f'Missing {name}')
            continue
        try:
            pin = int(data[name])
        except (ValueError, TypeError):
            errors.append(f'Non-numeric {name}')
            continue
        if pin == -1 and name in ('LCD_RST', 'ESP_CONSOLE_UART_RX_GPIO'):
            continue
        if pin < 0 or pin > 48 or 22 <= pin <= 25:
            errors.append(f'Invalid GPIO{pin}: {name}')
        elif 26 <= pin <= 37:
            errors.append(f'GPIO{pin} reserved for N16R8 flash/PSRAM: {name}')
        elif pin in (0, 3, 45, 46):
            errors.append(f'GPIO{pin} boot-strapping forbidden by board policy: {name}')
        elif usb and pin in (19, 20):
            errors.append(f'GPIO{pin} owned by USB: {name}')
        elif uart and pin in (43, 44):
            errors.append(f'GPIO{pin} owned by UART console: {name}')
        elif pin in seen:
            errors.append(f'GPIO{pin} conflict: {seen[pin]} vs {name}')
        seen[pin] = name
    return errors


def check_platform(data):
    required = {
        'IDF_TARGET': '"esp32s3"',
        'ESPTOOLPY_FLASHSIZE_16MB': 'y',
        'SPIRAM': 'y',
        'SPIRAM_MODE_OCT': 'y',
        'SPIRAM_SPEED_80M': 'y',
        'SPIRAM_USE_MALLOC': 'y',
        'SPIRAM_MEMTEST': 'y',
        'ESP_DEFAULT_CPU_FREQ_MHZ_240': 'y',
    }
    return [f'N16R8 requires CONFIG_{key}={value}'
            for key, value in required.items() if data.get(key) != value]


if __name__ == '__main__':
    try:
        data = parse(sys.argv[1] if len(sys.argv) > 1 else 'sdkconfig')
        errors = check(data) + (check_platform(data) if '--platform' in sys.argv else [])
    except OSError as error:
        sys.exit(str(error))
    if errors:
        sys.exit('\n'.join('ERROR: ' + error for error in errors))
    print('N16R8 GPIO map check passed')
