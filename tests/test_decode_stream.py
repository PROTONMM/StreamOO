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
int main(int argc,char **argv) {
    assert(argc==2); unsigned mode=(unsigned)atoi(argv[1]);
    fake_t f={.mode=mode}; streamoo_decode_stream_t s;
    uint8_t data[600]={0}; int32_t out[480];
    size_t bytes=mode==9 ? sizeof data : 3;
    unsigned cycle=(unsigned)bytes*1024;
    assert(!streamoo_decode_stream_init(&s,data,0,&f,decode,reset,3072,3072));
    assert(streamoo_decode_stream_init(&s,data,bytes,&f,decode,reset,cycle,cycle));
    assert(!streamoo_decode_stream_read(&s,out,241)); assert(f.calls==0);
    if (mode && mode!=9) {
        bool ok=true;
        for (unsigned i=0;i<20 && ok;++i) ok=streamoo_decode_stream_read(&s,out,240);
        assert(!ok && s.failed);
        unsigned calls=f.calls;
        assert(!streamoo_decode_stream_read(&s,out,240)); assert(calls==f.calls);
    } else {
        for (unsigned block=0;block<(mode==9 ? 5200u : 100u);++block) {
            assert(streamoo_decode_stream_read(&s,out,240));
            for (unsigned i=0;i<240;++i) {
                unsigned word=((block*240+i)%cycle)&65535;
                int32_t expected=(word>=32768 ? (int32_t)word-65536 : (int32_t)word)*65536;
                assert(out[i*2]==expected && out[i*2+1]==expected);
            }
            unsigned calls=f.calls; /* No background eager decode. */
            assert(s.calls==calls && s.pcm_bytes<=8192 && s.pending<=257);
        }
        assert(f.resets==(mode==9 ? 2u : 7u) && s.cycles==f.resets);
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
            '-Icomponents/decoder/include','-Icomponents/pcm_adapter/include',str(base/'test.c'),
            'components/decoder/streamoo_decode_stream.c','components/pcm_adapter/streamoo_pcm_adapter.c',
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
