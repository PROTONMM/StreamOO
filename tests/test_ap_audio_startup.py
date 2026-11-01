from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HEADER=r'''
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
typedef struct{char version[32],idf_ver[32];uint8_t app_elf_sha256[32];} esp_app_desc_t;
const esp_app_desc_t *esp_app_get_description(void);
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
static inline const char *esp_err_to_name(esp_err_t err){(void)err;return "fake";}
static inline void test_log(const char *tag,const char *format,...) __attribute__((format(printf,2,3)));
static inline void test_log(const char *tag,const char *format,...){(void)tag;(void)format;}
#define ESP_LOGI(...) test_log(__VA_ARGS__)
#define ESP_LOGW(...) test_log(__VA_ARGS__)
#define ESP_LOGE(...) test_log(__VA_ARGS__)
typedef struct { uint8_t volume,visualization; } streamoo_preferences_value_t;
typedef struct { int unused; } streamoo_network_hooks_t;
extern const streamoo_network_hooks_t streamoo_app_network_hooks;
esp_err_t streamoo_board_validate(void);
esp_err_t streamoo_platform_start(void);
esp_err_t streamoo_platform_crypto_prepare(void);
esp_err_t streamoo_preferences_start(void);
esp_err_t streamoo_clock_settings_start(void);
esp_err_t streamoo_settings_start(void);
bool streamoo_preferences_initial(streamoo_preferences_value_t *);
esp_err_t streamoo_input_probe_start(void);
esp_err_t streamoo_audio_control_start(uint8_t);
esp_err_t streamoo_player_start(uint8_t);
esp_err_t streamoo_audio_probe_start(void);
esp_err_t streamoo_display_smoke_start(void);
bool streamoo_network_boot_ap_enabled(void);
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *);
'''
CONFIG=r'''
#pragma once
#define CONFIG_STREAMOO_ENCODER_UI_TEST 1
#define CONFIG_STREAMOO_ENCODER_GESTURE_TEST 1
#define CONFIG_STREAMOO_LCD_SMOKE_TEST 1
#define CONFIG_STREAMOO_NETWORK_AP_TEST 1
#define CONFIG_STREAMOO_LCD_MOSI 11
#define CONFIG_STREAMOO_LCD_SCLK 12
#define CONFIG_STREAMOO_LCD_CS 9
#define CONFIG_STREAMOO_LCD_DC 10
#define CONFIG_STREAMOO_I2S_DOUT 48
#define CONFIG_STREAMOO_I2S_BCLK 47
#define CONFIG_STREAMOO_I2S_LRCLK 21
#define CONFIG_STREAMOO_ENCODER_GPIO_A 5
#define CONFIG_STREAMOO_ENCODER_GPIO_B 6
#define CONFIG_STREAMOO_ENCODER_GPIO_BUTTON 7
#define CONFIG_LWIP_TCP_WND_DEFAULT 23040
#define CONFIG_LWIP_TCP_RECVMBOX_SIZE 18
#define CONFIG_LWIP_TCP_MSS 1440
#ifndef TEST_FINITE
#define CONFIG_STREAMOO_AUDIO_CONTROL_TEST 1
#else
#define CONFIG_STREAMOO_I2S_SMOKE_TEST 1
#endif
'''
HARNESS=r'''
#include "common.h"
#include <stdlib.h>
const esp_app_desc_t *esp_app_get_description(void){static const esp_app_desc_t a={.version="fixture",.idf_ver="fake"};return &a;}
static int mode,controlled,finite,network,display,player,clock_started,prepared;
esp_err_t streamoo_platform_crypto_prepare(void){assert(!controlled&&!finite&&!network&&!display&&!player);++prepared;return mode==4?ESP_FAIL:ESP_OK;}
esp_err_t streamoo_settings_start(void){return ESP_OK;}
esp_err_t streamoo_clock_settings_start(void){++clock_started;return mode==3?ESP_FAIL:ESP_OK;}
esp_err_t streamoo_player_start(uint8_t v){assert(v==87&&prepared==1);++player;return ESP_OK;}
const streamoo_network_hooks_t streamoo_app_network_hooks={0};
esp_err_t streamoo_board_validate(void){return ESP_OK;}
esp_err_t streamoo_platform_start(void){return ESP_OK;}
esp_err_t streamoo_preferences_start(void){return ESP_OK;}
bool streamoo_preferences_initial(streamoo_preferences_value_t *v){v->volume=87;v->visualization=1;return true;}
esp_err_t streamoo_input_probe_start(void){return ESP_OK;}
esp_err_t streamoo_audio_control_start(uint8_t volume){assert(volume==87&&prepared==1);++controlled;return ESP_OK;}
esp_err_t streamoo_audio_probe_start(void){assert(prepared==1);++finite;return ESP_OK;}
esp_err_t streamoo_display_smoke_start(void){assert(clock_started==1&&prepared==1);++display;return ESP_OK;}
bool streamoo_network_boot_ap_enabled(void){return mode!=0;}
esp_err_t streamoo_network_start(const streamoo_network_hooks_t *h){assert(h&&prepared==1);++network;return mode==2 ? ESP_FAIL:ESP_OK;}
void app_main(void);
int main(int argc,char **argv){
 assert(argc==2);mode=atoi(argv[1]);app_main();
 assert(prepared==1);
 if(mode==4){assert(!controlled&&!finite&&!network&&!display&&!player&&!clock_started);return 0;}
 assert(display==1 && network==1);assert(player==(mode!=0));
 if(mode)assert(!controlled && !finite);
 else {
#ifdef TEST_FINITE
 assert(finite==1 && !controlled);
#else
 assert(controlled==1 && !finite);
#endif
 }
 return 0;
}
'''
class APAudioStartupTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name);(base/'common.h').write_text(HEADER);(base/'sdkconfig.h').write_text(CONFIG)
        for name in ('esp_app_desc.h','esp_log.h','streamoo_board.h','streamoo_platform.h','streamoo_input_probe.h',
                     'display_smoke.h','audio_probe.h','audio_control_probe.h','streamoo_preferences.h',
                     'streamoo_network.h','network_portal.h','streamoo_player.h','streamoo_clock_settings.h','streamoo_settings.h'):
            (base/name).write_text('#include "common.h"\n')
        shutil.copyfile(ROOT/'main/main.c',base/'app.c');(base/'test.c').write_text(HARNESS)
        cls.binaries=[]
        for mode in ('controlled','finite'):
            binary=base/mode;flags=['-DTEST_FINITE'] if mode=='finite' else []
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*flags,'-I'+str(base),
                str(base/'app.c'),str(base/'test.c'),'-o',str(binary)],check=True)
            cls.binaries.append(binary)
    def run_case(self,binary,mode):subprocess.run([str(self.binaries[binary]),str(mode)],check=True,timeout=5)
    def test_ap_skips_decoder_pipeline(self):self.run_case(0,1)
    def test_ap_skips_finite_tone(self):self.run_case(1,1)
    def test_ap_failure_does_not_start_tone(self):self.run_case(0,2)
    def test_ap_disabled_preserves_controlled_audio(self):self.run_case(0,0)
    def test_ap_disabled_preserves_finite_test(self):self.run_case(1,0)

    def test_clock_storage_failure_preserves_startup(self):self.run_case(0,3)
    def test_crypto_failure_stops_before_audio_or_network(self):self.run_case(0,4)
