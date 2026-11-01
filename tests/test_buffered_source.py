from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
FREERTOS='''#pragma once
#include <stdint.h>
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define pdTRUE 1
'''
SEMAPHORE=r'''
#pragma once
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"
extern int force_busy,fail_create;
typedef pthread_mutex_t *SemaphoreHandle_t;
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    if (fail_create) return NULL;
    SemaphoreHandle_t m=malloc(sizeof(*m));
    if (m && pthread_mutex_init(m,NULL)) { free(m); return NULL; }
    return m;
}
static inline int xSemaphoreTake(SemaphoreHandle_t m,TickType_t ticks) {
    if (force_busy) return 0;
    if (!ticks) return pthread_mutex_trylock(m)==0;
    struct timespec t; clock_gettime(CLOCK_REALTIME,&t);
    t.tv_nsec+=(long)ticks*1000000L;
    t.tv_sec+=t.tv_nsec/1000000000L; t.tv_nsec%=1000000000L;
    return pthread_mutex_timedlock(m,&t)==0;
}
static inline int xSemaphoreGive(SemaphoreHandle_t m) { return pthread_mutex_unlock(m)==0; }
static inline void vSemaphoreDelete(SemaphoreHandle_t m) { pthread_mutex_destroy(m); free(m); }
'''
HARNESS=r'''
#include "streamoo_buffered_source.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
int force_busy,fail_create;
static streamoo_buffered_source_t *s;
static pthread_barrier_t prepared,rewound;
#define TOTAL 200003u
static void *producer(void *context) {
    (void)context; size_t offset=0; uint8_t chunk[997];
    while (offset<TOTAL) {
        size_t n=TOTAL-offset; if (n>sizeof chunk) n=sizeof chunk;
        for (size_t i=0;i<n;++i) chunk[i]=(uint8_t)(offset+i);
        offset+=streamoo_buffered_source_write(s,1,chunk,n); sched_yield();
    }
    while (!streamoo_buffered_source_finish(s,1,false)) sched_yield();
    return NULL;
}
static void *old_producer(void *context) {
    (void)context; uint8_t old[]={99,99},fresh[]={1,2,3};
    streamoo_buffered_source_stats_t stats;
    assert(streamoo_buffered_source_stats(s,&stats)); uint32_t token=stats.generation;
    pthread_barrier_wait(&prepared); pthread_barrier_wait(&rewound);
    assert(!streamoo_buffered_source_write(s,token,old,sizeof old));
    assert(!streamoo_buffered_source_finish(s,token,true));
    assert(streamoo_buffered_source_stats(s,&stats));
    assert(streamoo_buffered_source_write(s,stats.generation,fresh,sizeof fresh)==sizeof fresh);
    assert(streamoo_buffered_source_finish(s,stats.generation,false)); return NULL;
}
int main(int argc,char **argv) {
    assert(argc==2); int mode=atoi(argv[1]); uint8_t storage[31],data[64],out[257];
    memset(data,7,sizeof data); size_t bytes=99; bool last=true;
    if (mode==5) {
        assert(!streamoo_buffered_source_create(NULL,31));
        fail_create=1; assert(!streamoo_buffered_source_create(storage,31));
        streamoo_buffered_source_destroy(NULL); return 0;
    }
    s=streamoo_buffered_source_create(storage,sizeof storage); assert(s);
    streamoo_buffered_source_stats_t stats;
    assert(streamoo_buffered_source_stats(s,&stats) && stats.generation==1);
    if (mode==0) {
        assert(streamoo_buffered_source_write(s,1,data,64)==31);
        assert(streamoo_buffered_source_stats(s,&stats) && stats.backpressure==1);
        assert(streamoo_buffered_source_finish(s,1,false));
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_DATA);
        assert(bytes==31 && last && !memcmp(data,out,31));
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_END);
        assert(streamoo_buffered_source_stop(s));
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_ERROR);
        assert(!streamoo_buffered_source_rewind(s));
        assert(!streamoo_buffered_source_write(s,1,data,2));
    } else if (mode==1) {
        assert(streamoo_buffered_source_write(s,1,data,8)==8);
        assert(streamoo_buffered_source_rewind(s));
        assert(!streamoo_buffered_source_write(s,1,data,8));
        assert(!streamoo_buffered_source_finish(s,1,true));
        assert(streamoo_buffered_source_stats(s,&stats) && stats.generation==2 && !stats.queued && !stats.ended && stats.stale==2);
        assert(streamoo_buffered_source_write(s,2,data,8)==8);
    } else if (mode==2) {
        uint32_t empty=99,busy=99;
        assert(streamoo_buffered_source_waits(s,&empty,&busy)&&!empty&&!busy);
        assert(streamoo_buffered_source_write(s,1,data,8)==8);
        force_busy=1; memset(out,33,sizeof out);
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_WAIT);
        assert(streamoo_buffered_source_waits(s,&empty,&busy)&&!empty&&busy==1);
        assert(!bytes && !last && out[0]==33);
        assert(!streamoo_buffered_source_write(s,1,data,8));
        assert(!streamoo_buffered_source_rewind(s)); assert(!streamoo_buffered_source_stop(s));
        assert(!streamoo_buffered_source_stats(s,&stats)); force_busy=0;
        assert(streamoo_buffered_source_stats(s,&stats) && stats.generation==1 && stats.queued==8);
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_DATA&&bytes==7);
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_WAIT);
        assert(streamoo_buffered_source_waits(s,&empty,&busy)&&empty==1&&busy==1);
        assert(!streamoo_buffered_source_waits(NULL,&empty,&busy));
        assert(!streamoo_buffered_source_waits(s,NULL,&busy));
        assert(!streamoo_buffered_source_waits(s,&empty,NULL));
    } else if (mode==3) {
        pthread_t thread; assert(!pthread_create(&thread,NULL,producer,NULL));
        size_t offset=0;
        while (offset<TOTAL) {
            streamoo_source_result_t result=streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last);
            if (result==STREAMOO_SOURCE_WAIT) { sched_yield(); continue; }
            assert(result==STREAMOO_SOURCE_DATA && bytes && offset+bytes<=TOTAL);
            for (size_t i=0;i<bytes;++i) assert(out[i]==(uint8_t)(offset+i));
            offset+=bytes; assert(last==(offset==TOTAL));
        }
        assert(!pthread_join(thread,NULL));
        assert(streamoo_buffered_source_stop(s));
    } else if (mode==4) {
        pthread_t thread; pthread_barrier_init(&prepared,NULL,2); pthread_barrier_init(&rewound,NULL,2);
        assert(!pthread_create(&thread,NULL,old_producer,NULL));
        pthread_barrier_wait(&prepared); assert(streamoo_buffered_source_rewind(s));
        pthread_barrier_wait(&rewound); assert(!pthread_join(thread,NULL));
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_DATA);
        assert(bytes==3 && last && out[0]==1 && out[1]==2 && out[2]==3);
        assert(streamoo_buffered_source_stats(s,&stats) && stats.stale==2);
        pthread_barrier_destroy(&prepared); pthread_barrier_destroy(&rewound);
    } else {
        assert(streamoo_buffered_source_write(s,1,data,8)==8);
        assert(streamoo_buffered_source_finish(s,1,true));
        assert(streamoo_buffered_source_read(s,out,sizeof out,&bytes,&last)==STREAMOO_SOURCE_ERROR);
    }
    streamoo_buffered_source_destroy(s); return 0;
}
'''
class BufferedSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name); (base/'freertos').mkdir()
        (base/'freertos/FreeRTOS.h').write_text(FREERTOS)
        (base/'freertos/semphr.h').write_text(SEMAPHORE)
        (base/'test.c').write_text(HARNESS); cls.binary=base/'test'
        subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror',
            '-pthread','-I'+str(base),'-Icomponents/compressed_source/include',str(base/'test.c'),
            'components/compressed_source/streamoo_byte_source.c','components/compressed_source/streamoo_buffered_source.c',
            '-o',str(cls.binary)],cwd=ROOT,check=True)
    def run_case(self,mode): subprocess.run([str(self.binary),str(mode)],check=True,timeout=15)
    def test_backpressure_eof_and_stop(self): self.run_case(0)
    def test_old_generation_cannot_write_or_finish(self): self.run_case(1)
    def test_lock_contention_does_not_mutate_source(self): self.run_case(2)
    def test_concurrent_producer_and_consumer_preserve_bytes(self): self.run_case(3)
    def test_rewind_between_capture_and_publish(self): self.run_case(4)
    def test_creation_failure(self): self.run_case(5)
    def test_terminal_failure(self): self.run_case(6)
