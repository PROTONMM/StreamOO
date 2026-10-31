from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_decode_stream.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned frame,calls,resets,mode; bool emitted; } fake_t;
static esp_err_t reset(void *p) {
    fake_t *f=p; ++f->resets; f->frame=0; f->emitted=false;
    return f->mode==7 ? -1 : ESP_OK;
}
static esp_err_t decode(void *p,uint8_t *input,size_t bytes,bool eos,
    uint8_t *pcm,size_t capacity,streamoo_decode_result_t *r) {
    fake_t *f=p; ++f->calls; assert(input && bytes && capacity==8192);
    assert((uintptr_t)pcm%_Alignof(int32_t)==0);
    (void)eos;
    if (f->mode==1) return -1;
    if (f->mode==2) return ESP_OK; /* no progress */
    if (f->mode==3) { r->consumed=bytes+1; return ESP_OK; }
    if (f->mode==4) { r->produced=capacity+1; return ESP_OK; }
    if (f->mode==8) { r->consumed=bytes; return ESP_OK; } /* empty cycle */
    if (f->emitted) { r->consumed=1; f->emitted=false; return ESP_OK; }
    for (unsigned i=0;i<1024;++i) {
        unsigned value=f->frame++;
        pcm[i*4]=value&255; pcm[i*4+1]=(value>>8)&255;
        pcm[i*4+2]=value&255; pcm[i*4+3]=(value>>8)&255;
    }
    r->produced=4096; r->format=(streamoo_pcm_format_t){48000,2,16};
    if (f->mode==5) r->format.rate=44100;
    if (f->mode==6) r->produced=4095;
    f->emitted=true; return ESP_OK;
}

typedef struct { const uint8_t *data; size_t bytes,cursor; unsigned mode,reads; streamoo_byte_ring_t ring; uint8_t storage[17]; } input_t;
static streamoo_source_result_t read_source(void *p,uint8_t *out,size_t capacity,size_t *bytes,bool *last) {
    input_t *s=p; *bytes=0; *last=false; ++s->reads;
    if (s->mode==14) return streamoo_byte_ring_read(&s->ring,out,capacity,bytes,last);
    if (s->mode==10 && s->reads%2) return STREAMOO_SOURCE_WAIT;
    if (s->mode==11) return STREAMOO_SOURCE_ERROR;
    if (s->mode==12) { *bytes=1; return STREAMOO_SOURCE_WAIT; }
    if (s->cursor==s->bytes) return STREAMOO_SOURCE_END;
    size_t n=s->bytes-s->cursor; if (n>capacity) n=capacity;
    memcpy(out,s->data+s->cursor,n); s->cursor+=n; *bytes=n; *last=s->cursor==s->bytes;
    return STREAMOO_SOURCE_DATA;
}
static bool rewind_source(void *p) { input_t *s=p; s->cursor=0; if (s->mode==14) streamoo_byte_ring_clear(&s->ring); return s->mode!=13; }

int main(int argc,char **argv) {
    assert(argc==2); unsigned mode=(unsigned)atoi(argv[1]);
    fake_t f={.mode=mode}; streamoo_decode_stream_t s;
    uint8_t data[600]={0}; int32_t out[480];
    size_t bytes=(mode==9 || mode==14) ? sizeof data : 3;
    unsigned cycle=(unsigned)bytes*1024;
    input_t input={.data=data,.bytes=bytes,.mode=mode};
    assert(streamoo_byte_ring_init(&input.ring,input.storage,sizeof input.storage));
    assert(!streamoo_decode_stream_init(&s,(streamoo_byte_source_t){0},&f,decode,reset,3072,3072));
    assert(streamoo_decode_stream_init(&s,(streamoo_byte_source_t){&input,read_source,rewind_source},&f,decode,reset,cycle,cycle));
    assert(streamoo_decode_stream_read(&s,out,241)==STREAMOO_STREAM_FAILED); assert(f.calls==0);
    if (mode && mode!=9 && mode!=10 && mode!=14) {
        streamoo_stream_result_t result=STREAMOO_STREAM_READY;
        for (unsigned i=0;i<20 && result!=STREAMOO_STREAM_FAILED;++i) result=streamoo_decode_stream_read(&s,out,240);
        assert(result==STREAMOO_STREAM_FAILED && s.failed);
        unsigned calls=f.calls;
        assert(streamoo_decode_stream_read(&s,out,240)==STREAMOO_STREAM_FAILED); assert(calls==f.calls);
    } else {
        for (unsigned block=0;block<((mode==9 || mode==14) ? 5200u : 100u);++block) {
            memset(out,0x55,sizeof out);
            streamoo_stream_result_t result;
            while ((result=streamoo_decode_stream_read(&s,out,240))==STREAMOO_STREAM_WAIT) {
                for (unsigned j=0;j<480;++j) assert(out[j]==0x55555555);
                if (mode==14) {
                    size_t n=input.bytes-input.cursor; if (n>11) n=11;
                    input.cursor+=streamoo_byte_ring_write(&input.ring,input.data+input.cursor,n);
                    if (input.cursor==input.bytes) streamoo_byte_ring_finish(&input.ring,false);
                }
            }
            assert(result==STREAMOO_STREAM_READY);
            for (unsigned i=0;i<240;++i) {
                unsigned word=((block*240+i)%cycle)&65535;
                int32_t expected=(word>=32768 ? (int32_t)word-65536 : (int32_t)word)*65536;
                assert(out[i*2]==expected && out[i*2+1]==expected);
            }
            unsigned calls=f.calls; /* No background eager decode. */
            assert(s.calls==calls && s.pcm_bytes<=8192 && s.pending<=257);
        }
        assert(f.resets==((mode==9 || mode==14) ? 2u : 7u) && s.cycles==f.resets);
    }
    return 0;
}
'''
class DecodeStreamTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name)
        (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n')
        (base/'test.c').write_text(HARNESS); cls.binary=base/'test'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(base),
            '-Icomponents/decoder/include','-Icomponents/pcm_adapter/include','-Icomponents/compressed_source/include',str(base/'test.c'),
            'components/decoder/streamoo_decode_stream.c','components/pcm_adapter/streamoo_pcm_adapter.c',
            'components/compressed_source/streamoo_byte_source.c',
            '-o',str(cls.binary)],cwd=ROOT,check=True)
    def run_case(self,mode):
        subprocess.run([str(self.binary),str(mode)],check=True,timeout=5)
    def test_packet_tail_partial_input_and_loop_continuity(self): self.run_case(0)
    def test_decoder_error(self): self.run_case(1)
    def test_no_progress(self): self.run_case(2)
    def test_consumption_overflow(self): self.run_case(3)
    def test_output_overflow(self): self.run_case(4)
    def test_unsupported_format(self): self.run_case(5)
    def test_partial_pcm_frame(self): self.run_case(6)
    def test_reset_error(self): self.run_case(7)
    def test_empty_cycle(self): self.run_case(8)

    def test_multiple_input_chunks_and_eof(self): self.run_case(9)

    def test_temporary_wait_and_resume(self): self.run_case(10)
    def test_source_error(self): self.run_case(11)
    def test_invalid_wait_contract(self): self.run_case(12)
    def test_rewind_error(self): self.run_case(13)

    def test_ring_feeder_wait_partial_block_and_rewind(self): self.run_case(14)
