from pathlib import Path
import os,subprocess,tempfile,unittest
from test_network_player import HEADERS
ROOT=Path(__file__).resolve().parents[1]
class ClockSettingsTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  (base/'freertos').mkdir()
  (base/'esp_err.h').write_text(HEADERS['esp_err.h']+'\n#define ESP_ERR_NVS_NOT_FOUND 10\n#define ESP_ERR_TIMEOUT 11\n')
  (base/'freertos/FreeRTOS.h').write_text(HEADERS['freertos/FreeRTOS.h'])
  (base/'freertos/semphr.h').write_text('#pragma once\ntypedef void *SemaphoreHandle_t;\nvoid *xSemaphoreCreateMutex(void);\nint xSemaphoreTake(void *,unsigned);\nvoid xSemaphoreGive(void *);\n')
  (base/'nvs_flash.h').write_text('#pragma once\n#include <stddef.h>\n#include "esp_err.h"\ntypedef int nvs_handle_t;\n#define NVS_READONLY 0\n#define NVS_READWRITE 1\nint nvs_flash_init(void);\nint nvs_open(const char *,int,int *);\nint nvs_get_str(int,const char *,char *,size_t *);\nint nvs_set_str(int,const char *,const char *);\nint nvs_commit(int);\nvoid nvs_close(int);\n')
  cls.binary=str(base/'settings')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_CLOCK_SANITIZE','').split(),'-I'+str(base),'-Icomponents/clock/include','components/clock/streamoo_clock.c','components/clock/streamoo_clock_settings.c','tests/clock_settings_test.c','-o',cls.binary],cwd=ROOT,check=True)
 def case(self,n):subprocess.run([self.binary,str(n)],check=True)
 def test_default_save_noop_and_commit_rollback(self):self.case(0)
 def test_restored_zone_after_restart(self):self.case(1)
 def test_nvs_init_failure_no_erase(self):self.case(2)
 def test_unknown_saved_zone_defaults_safely(self):self.case(3)
 def test_read_failure_keeps_default_and_disables_write(self):self.case(4)
 def test_mutex_allocation_failure(self):self.case(5)

 def test_saved_zone_survives_fresh_process(self):
  with tempfile.TemporaryDirectory() as tmp:
   env={**os.environ,'STREAMOO_CLOCK_TEST_NVS':str(Path(tmp)/'nvs.bin')}
   subprocess.run([self.binary,'6'],check=True,env=env)
   subprocess.run([self.binary,'7'],check=True,env=env)
