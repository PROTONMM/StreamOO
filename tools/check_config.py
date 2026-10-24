#!/usr/bin/env python3
"""Early sanity check for StreamOO sdkconfig. Not a substitute for chip-level validator."""
import sys
from pathlib import Path

PINS = ['LCD_MOSI','LCD_SCLK','LCD_CS','LCD_DC',
        'I2S_DOUT','I2S_BCLK','I2S_LRCLK',
        'ENCODER_GPIO_A','ENCODER_GPIO_B','ENCODER_GPIO_BUTTON']
def parse(path):
    data = {}
    for ln in Path(path).read_text().splitlines():
        if ln.startswith('CONFIG_STREAMOO_') and '=' in ln:
            k,v=ln.split('=',1); data[k.removeprefix('CONFIG_STREAMOO_')]=v
    return data

def check(d):
    seen={};errors=[]
    for name in PINS:
        if name not in d: errors.append(f'Missing {name}');continue
        try:p=int(d[name])
        except ValueError: errors.append(f'Non-numeric {name}');continue
        if p < 0 or p > 48: errors.append(f'Invalid GPIO{p}: {name}')
        if p in range(33,38):errors.append(f'GPIO{p} may be reserved by N16R8 PSRAM: {name}')
        if p in seen:errors.append(f'GPIO{p} conflict: {seen[p]} vs {name}')
        seen[p]=name
    return errors
if __name__ == '__main__':
    p=sys.argv[1] if len(sys.argv)>1 else 'sdkconfig'
    errors=check(parse(p))
    if errors: print('\n'.join('ERROR: '+e for e in errors));sys.exit(1)
    print('Basic GPIO map check passed')
