"""Startup screen policy boundaries; no SDK or display emulation."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class BootTransitionTests(unittest.TestCase):
    def test_logo_until_wifi_audio_or_ap_without_hiding_failure(self):
        source = r'''
#include "streamoo_ui_boot.h"
#include <assert.h>
int main(void) {
    assert(streamoo_ui_boot_wait(false,false,true,60000));
    assert(!streamoo_ui_boot_wait(true,false,true,0));
    assert(!streamoo_ui_boot_wait(true,true,true,0));
    assert(streamoo_ui_boot_wait(false,true,false,499));
    assert(!streamoo_ui_boot_wait(false,true,false,500));
    assert(streamoo_ui_boot_wait(false,true,true,29999));
    assert(!streamoo_ui_boot_wait(false,true,true,30000));
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)
            (path/'test.c').write_text(source)
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                            '-I'+str(ROOT/'components/ui/include'),str(path/'test.c'),
                            '-o',str(path/'test')],check=True)
            subprocess.run([str(path/'test')],check=True)
