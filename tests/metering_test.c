#include "streamoo_metering.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t scheduler=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static bool permit,paused,finished;
static void (*entry)(void *); static void *argument;
static unsigned allocations;
static bool fail_allocation,fail_task;
static atomic_llong now;
int64_t esp_timer_get_time(void){return atomic_load(&now);}
void *heap_caps_calloc(size_t n,size_t bytes,unsigned caps){
    assert(caps==3);if(fail_allocation)return NULL;
    void *p=calloc(n,bytes);if(p)++allocations;return p;
}
void heap_caps_free(void *p){if(p){assert(allocations);--allocations;free(p);}}
unsigned uxTaskGetStackHighWaterMark(void *p){(void)p;return 3000;}
static void pause_worker(void){
    pthread_mutex_lock(&scheduler);paused=true;pthread_cond_broadcast(&changed);
    while(!permit)pthread_cond_wait(&changed,&scheduler);
    permit=false;paused=false;pthread_mutex_unlock(&scheduler);
}
void vTaskDelay(unsigned ticks){assert(ticks==5);pause_worker();}
void vTaskDelete(void *p){
    assert(!p);pthread_mutex_lock(&scheduler);finished=true;
    pthread_cond_broadcast(&changed);pthread_mutex_unlock(&scheduler);pthread_exit(NULL);
}
static void *run(void *unused){(void)unused;pause_worker();entry(argument);abort();}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *ctx,unsigned priority,void *handle){
    assert(!strcmp(name,"audio_levels")&&stack==4096&&priority==1&&!handle);
    if(fail_task)return 0;
    entry=fn;argument=ctx;permit=paused=finished=false;
    assert(!pthread_create(&worker,NULL,run,NULL));
    pthread_mutex_lock(&scheduler);
    while(!paused)pthread_cond_wait(&changed,&scheduler);
    pthread_mutex_unlock(&scheduler);return 1;
}
static void step(void){
    atomic_fetch_add(&now,5000);
    pthread_mutex_lock(&scheduler);assert(paused&&!finished);
    paused=false;permit=true;pthread_cond_broadcast(&changed);
    while(!paused&&!finished)pthread_cond_wait(&changed,&scheduler);
    pthread_mutex_unlock(&scheduler);
}
static void close_meter(streamoo_metering_t *m){
    assert(!streamoo_metering_dispose(m)); /* Must wait for consumer ACK. */
    assert(allocations==1);step();assert(!pthread_join(worker,NULL));
    assert(streamoo_metering_dispose(m));assert(!allocations);
}
static int32_t lr[480];
static void push(streamoo_metering_t *m,unsigned seq){
    assert(streamoo_pcm_push(streamoo_metering_tap(m),lr,seq));
}
int main(int argc,char **argv){
    assert(argc==2);int test=atoi(argv[1]);
    streamoo_metering_t *m=NULL;streamoo_metering_state_t state;
    for(unsigned i=0;i<240;++i){lr[2*i]=16000*65536;lr[2*i+1]=-8000*65536;}
    if(test==0){
        assert(streamoo_metering_open(4,NULL)==ESP_ERR_INVALID_ARG);
        assert(!streamoo_metering_tap(NULL));assert(streamoo_metering_dispose(NULL));
        assert(!streamoo_metering_state(NULL,&state));
        fail_allocation=true;assert(streamoo_metering_open(4,&m)==ESP_ERR_NO_MEM&&!m);
        fail_allocation=false;fail_task=true;
        assert(streamoo_metering_open(4,&m)==ESP_ERR_NO_MEM&&!m&&!allocations);return 0;
    }
    assert(streamoo_metering_open(17,&m)==ESP_OK);
    assert(!streamoo_metering_state(m,NULL));
    assert(streamoo_metering_state(m,&state));assert(!state.levels.valid&&state.generation==17);
    for(unsigned seq=0;seq<8;++seq){push(m,seq);if(seq%4==3)step();}
    assert(streamoo_metering_state(m,&state));
    assert(state.levels.valid&&state.levels.windows==1&&state.generation==17);
    assert(state.levels.rms[0]==0&&state.levels.rms[1]==0); /* 10Hz publication not due yet. */
    assert(state.completed_us==10000&&state.stack_hwm==3000);
    if(test==1){close_meter(m);}
    else if(test==2){
        atomic_fetch_add(&now,200001);
        assert(streamoo_metering_state(m,&state));
        assert(!state.levels.valid&&!state.levels.rms[0]&&!state.levels.peak[1]);
        close_meter(m);
    }else if(test==3){
        for(unsigned seq=8;seq<12;++seq)push(m,seq);
        assert(!streamoo_pcm_push(streamoo_metering_tap(m),lr,12)); /* drop newest */
        step();push(m,13);step();assert(streamoo_metering_state(m,&state));
        assert(state.levels.valid&&state.levels.discontinuities==1); /* retain last complete window, not partial data */
        for(unsigned seq=14;seq<=20;++seq){push(m,seq);step();}
        assert(streamoo_metering_state(m,&state));
        assert(state.levels.valid&&state.levels.windows==2&&state.levels.discontinuities==1);
        close_meter(m);
    }else if(test==4){
        close_meter(m);assert(streamoo_metering_open(18,&m)==ESP_OK);
        assert(streamoo_metering_state(m,&state));
        assert(state.generation==18&&!state.levels.valid&&!state.levels.windows);
        close_meter(m);
    }else if(test==6){
        atomic_store(&now,100000);step();
        assert(streamoo_metering_state(m,&state));
        assert(state.levels.valid&&state.levels.rms[0]>5000&&state.levels.rms[0]<7000);
        assert(state.levels.peak[0]==16000);
        close_meter(m);
    }else if(test==5){
        atomic_store(&now,1);
        assert(streamoo_metering_state(m,&state));assert(!state.levels.valid);
        close_meter(m);
    }else abort();
    return 0;
}
