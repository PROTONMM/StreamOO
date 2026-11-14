"""Exercise the actual runtime profiler across task deletion/TCB reuse."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HEADERS={
    'sdkconfig.h': '''#pragma once
#define CONFIG_STREAMOO_DIAGNOSTICS 1
#define CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS 1
#define CONFIG_SPIRAM_MODE_OCT 1
#define CONFIG_SPIRAM_SPEED_80M 1
#define CONFIG_ESPTOOLPY_FLASHSIZE_16MB 1
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ 240
''',
    'esp_err.h': '#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_ERR_INVALID_STATE 1\n#define ESP_ERR_NO_MEM 2\n',
    'esp_timer.h': '#pragma once\n#include <stdint.h>\nint64_t esp_timer_get_time(void);\n',
    'esp_log.h': '#pragma once\nvoid fake_log(const char *,const char *,...) __attribute__((format(printf,2,3)));\n#define ESP_LOGI fake_log\n#define ESP_LOGW fake_log\n#define ESP_LOGE fake_log\n',
    'esp_chip_info.h': '#pragma once\n#define CHIP_ESP32S3 9\ntypedef struct{int model;unsigned cores;}esp_chip_info_t;\nvoid esp_chip_info(esp_chip_info_t *);\n',
    'esp_flash.h': '#pragma once\n#include <stdint.h>\n#include "esp_err.h"\nesp_err_t esp_flash_get_size(void *,uint32_t *);\n',
    'esp_heap_caps.h': '#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_INTERNAL 1\n#define MALLOC_CAP_8BIT 2\n#define MALLOC_CAP_SPIRAM 4\nsize_t heap_caps_get_free_size(unsigned);\nsize_t heap_caps_get_minimum_free_size(unsigned);\nsize_t heap_caps_get_largest_free_block(unsigned);\n',
    'esp_idf_version.h': '#pragma once\nconst char *esp_get_idf_version(void);\n',
    'esp_psram.h': '#pragma once\n#include <stdbool.h>\n#include <stddef.h>\nbool esp_psram_is_initialized(void);\nsize_t esp_psram_get_size(void);\n',
    'freertos/FreeRTOS.h': '#pragma once\n#include <stdint.h>\ntypedef unsigned UBaseType_t;\ntypedef int BaseType_t;\n#define tskNO_AFFINITY -1\n#define pdPASS 1\n#define pdMS_TO_TICKS(x) (x)\n',
    'freertos/task.h': '''#pragma once
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
#ifndef configRUN_TIME_COUNTER_TYPE
#define configRUN_TIME_COUNTER_TYPE uint64_t
#endif
typedef struct{
 TaskHandle_t xHandle;const char *pcTaskName;UBaseType_t xTaskNumber;
 configRUN_TIME_COUNTER_TYPE ulRunTimeCounter;unsigned usStackHighWaterMark;
}TaskStatus_t;
UBaseType_t uxTaskGetNumberOfTasks(void);
UBaseType_t uxTaskGetSystemState(TaskStatus_t *,UBaseType_t,configRUN_TIME_COUNTER_TYPE *);
BaseType_t xTaskGetCoreID(TaskHandle_t);
unsigned uxTaskGetStackHighWaterMark(void *);
void vTaskDelay(unsigned);
int xTaskCreate(void (*)(void *),const char *,unsigned,void *,unsigned,void *);
''',
    'freertos/idf_additions.h': '#pragma once\n',
}
HARNESS=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "streamoo_platform.c"
static int64_t clock_us;
static uint64_t reader_runtime;
static unsigned reader_number=10;
static char task_line[256];
void fake_log(const char *tag,const char *format,...){
 (void)tag;
 if(strstr(format,"CPU task=")){
  va_list ap;va_start(ap,format);vsnprintf(task_line,sizeof task_line,format,ap);va_end(ap);
 }
}
int64_t esp_timer_get_time(void){return clock_us;}
UBaseType_t uxTaskGetNumberOfTasks(void){return 3;}
UBaseType_t uxTaskGetSystemState(TaskStatus_t *tasks,UBaseType_t capacity,configRUN_TIME_COUNTER_TYPE *total){
 assert(capacity>=3);*total=(uint64_t)clock_us*2;
 tasks[0]=(TaskStatus_t){(void *)1,"IDLE0",1,(uint64_t)clock_us/2,1000};
 tasks[1]=(TaskStatus_t){(void *)2,"IDLE1",2,(uint64_t)clock_us/2,1000};
 tasks[2]=(TaskStatus_t){(void *)3,"http_reader",reader_number,reader_runtime,4096};
 return 3;
}
BaseType_t xTaskGetCoreID(TaskHandle_t h){return h==(void *)2?1:0;}
unsigned uxTaskGetStackHighWaterMark(void *h){(void)h;return 1000;}
void vTaskDelay(unsigned ticks){(void)ticks;}
int xTaskCreate(void (*fn)(void *),const char *n,unsigned s,void *c,unsigned p,void *h){
 (void)fn;(void)n;(void)s;(void)c;(void)p;(void)h;return pdPASS;
}
void esp_chip_info(esp_chip_info_t *c){*c=(esp_chip_info_t){CHIP_ESP32S3,2};}
esp_err_t esp_flash_get_size(void *f,uint32_t *n){(void)f;*n=16*1024*1024;return ESP_OK;}
size_t heap_caps_get_free_size(unsigned c){(void)c;return 1024;}
size_t heap_caps_get_minimum_free_size(unsigned c){(void)c;return 512;}
size_t heap_caps_get_largest_free_block(unsigned c){(void)c;return 256;}
const char *esp_get_idf_version(void){return "6.1";}
bool esp_psram_is_initialized(void){return true;}
size_t esp_psram_get_size(void){return 8*1024*1024;}
static void snapshot(uint64_t runtime){clock_us+=10000000;reader_runtime=runtime;task_line[0]=0;log_cpu();}
int main(int argc,char **argv){
 assert(argc==2);int mode=atoi(argv[1]);
 snapshot(3000000);assert(!task_line[0]);
 snapshot(5000000);assert(strstr(task_line,"load=20.0%"));
 if(mode==0){snapshot(7500000);assert(strstr(task_line,"load=25.0%"));}
 else if(mode==1){
  ++reader_number;snapshot(100000);assert(!task_line[0]);
  snapshot(2100000);assert(strstr(task_line,"load=20.0%"));
 }else if(mode==2){
  /* New task can already exceed the previous lifetime's counter. Identity
   * still has to invalidate the first mixed-lifetime interval. */
  ++reader_number;snapshot(6000000);assert(!task_line[0]);
  snapshot(8000000);assert(strstr(task_line,"load=20.0%"));
 }else if(mode==3){
  snapshot(100000);assert(!task_line[0]); /* Defensive counter-reset guard. */
  snapshot(3100000);assert(strstr(task_line,"load=30.0%"));
 }
 return 0;
}
'''

class CpuProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name);(base/'freertos').mkdir()
        for name,body in HEADERS.items(): (base/name).write_text(body)
        (base/'test.c').write_text(HARNESS)
        cls.binary=str(base/'cpu_profile')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
            *os.environ.get('STREAMOO_CPU_PROFILE_SANITIZE','').split(),
            '-I'+str(base),'-Icomponents/platform/include','-Icomponents/platform',
            str(base/'test.c'),'-o',cls.binary],cwd=ROOT,check=True)
    def case(self,mode):subprocess.run([self.binary,str(mode)],check=True,timeout=10)
    def test_regular_interval(self):self.case(0)
    def test_recycled_task_handle_lower_counter(self):self.case(1)
    def test_recycled_task_handle_higher_counter(self):self.case(2)
    def test_counter_reset_never_underflows(self):self.case(3)

    def test_diagnostics_also_accept_sdk_32_bit_counter(self):
        base=Path(self.temp.name)
        binary=str(base/'cpu_profile_u32')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
            '-DconfigRUN_TIME_COUNTER_TYPE=uint32_t',
            '-I'+str(base),'-Icomponents/platform/include','-Icomponents/platform',
            str(base/'test.c'),'-o',binary],cwd=ROOT,check=True)
        for mode in range(4):
            subprocess.run([binary,str(mode)],check=True,timeout=10)
