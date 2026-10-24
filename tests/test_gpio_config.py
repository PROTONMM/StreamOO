import importlib.util
from pathlib import Path

path = Path(__file__).resolve().parents[1] / 'tools' / 'check_config.py'
spec=importlib.util.spec_from_file_location('check_config',path)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def test_default_gpio_has_no_collision():
    data=mod.parse(Path(__file__).resolve().parents[1] / 'sdkconfig.defaults')
    assert mod.check(data)==[]

def test_duplicate_rejected():
    data=mod.parse(Path(__file__).resolve().parents[1] / 'sdkconfig.defaults')
    data['LCD_DC']=data['LCD_CS']
    assert any('conflict' in e for e in mod.check(data))
