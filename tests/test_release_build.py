"""Compile production UI and exercise actual CMake test-source selection."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

from test_controls import ROOT, SOURCES


class ReleaseBuildTests(unittest.TestCase):
    def test_production_ui_requires_real_owners_and_draws_without_ip(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            (base / 'sdkconfig.h').write_text('#pragma once\n')
            (base / 'test.c').write_text(r'''
#include <assert.h>
#include <string.h>
#include "encoder_demo.h"
static unsigned commands;
static bool post(void *ctx,const streamoo_control_command_t *cmd){
 (void)ctx;assert(cmd->kind==STREAMOO_CONTROL_PLAY);++commands;return true;
}
int main(void){
 streamoo_encoder_demo_t d;uint8_t frame[8194];memset(frame,0x55,sizeof frame);
 assert(streamoo_encoder_demo_init(&d));
 assert(!d.station_id&&!d.running&&!d.ui.count&&d.imported_catalog);
 assert(!d.ui.running);
 assert(!streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_ROTATE,1},0,0));
 assert(!d.running&&!d.control_pending&&!d.selection_pending);
 assert(streamoo_encoder_demo_draw(&d,frame+1,8192,0));
 assert(frame[0]==0x55&&frame[8193]==0x55);
 const streamoo_ui_station_t stations[]={{11,(const uint8_t *)"Real",4}};
 assert(streamoo_encoder_demo_set_catalog(&d,stations,1,2));
 assert(streamoo_encoder_demo_bind_audio(&d,post,NULL));
 d.station_id=11;streamoo_ui_set_playing(&d.ui,11);
 assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,1));
 assert(commands==1&&!d.running); /* Only the real owner's ack changes state. */
 assert(streamoo_encoder_demo_audio_state(&d,38,true));
 assert(streamoo_encoder_demo_set_ip(&d,"192.168.1.10"));
 assert(streamoo_encoder_demo_draw(&d,frame+1,8192,2));
 assert(streamoo_encoder_demo_set_ip(&d,NULL));
 assert(streamoo_encoder_demo_draw(&d,frame+1,8192,3));
 return 0;
}
''')
            includes = ['-Icomponents/' + name + '/include' for name in
                        ('graphics', 'text', 'ui', 'ui_widgets', 'input')]
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-DESP_PLATFORM', '-I' + tmp, '-Imain', *includes,
                            *[s for s in SOURCES if s != 'tests/controls_test.c'],
                            str(base / 'test.c'), '-o', str(base / 'test')],
                           cwd=ROOT, check=True)
            subprocess.run([str(base / 'test')], check=True)

    def sources(self, *flags):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            output = base / 'sources.txt'
            script = base / 'select.cmake'
            script.write_text('''function(idf_component_register)
 cmake_parse_arguments(REG "" "" "SRCS;INCLUDE_DIRS;REQUIRES" ${ARGN})
 file(WRITE "''' + str(output) + '''" "${REG_SRCS}")
endfunction()
include("''' + str(ROOT / 'main/CMakeLists.txt') + '''")
''')
            cmake = os.environ.get('STREAMOO_TEST_CMAKE') or shutil.which('cmake', path='/usr/bin:/bin') or shutil.which('cmake')
            subprocess.run([cmake, *['-DCONFIG_STREAMOO_' + f + '=ON' for f in flags],
                            '-P', str(script)], check=True, capture_output=True)
            return set(output.read_text().split(';'))

    def test_radio_excludes_hardware_test_sources(self):
        self.assertEqual(self.sources('NETWORK_ENABLED', 'UI_ENABLED', 'DISPLAY_ENABLED'),
                         {'main.c', 'network_portal.c', 'encoder_demo.c', 'display_smoke.c'})

    def test_opt_in_display_and_tone_sources(self):
        sources = self.sources('DISPLAY_ENABLED', 'DISPLAY_TEST', 'I2S_SMOKE_TEST')
        self.assertTrue({'canvas_smoke.c', 'text_smoke.c', 'home_probe.c',
                         'ui_list_probe.c', 'audio_probe.c', 'audio_tone.c'} <= sources)
        self.assertNotIn('encoder_demo.c', sources)
        self.assertNotIn('network_portal.c', sources)

    def test_opt_in_controlled_audio_sources(self):
        sources = self.sources('UI_ENABLED', 'AUDIO_CONTROL_TEST', 'CODEC_FIXTURE_TEST')
        self.assertTrue({'audio_control_model.c', 'audio_control_probe.c',
                         'codec_fixture.c', 'audio_tone.c'} <= sources)
