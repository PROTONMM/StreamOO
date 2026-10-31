from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_byte_source.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    assert(argc==2); int mode=atoi(argv[1]);
    uint8_t storage[17],data[512],out[512];
    for (unsigned i=0;i<512;++i) data[i]=(uint8_t)i;
    streamoo_byte_ring_t r;
    assert(!streamoo_byte_ring_init(&r,storage,1));
    assert(streamoo_byte_ring_init(&r,storage,sizeof storage));
    size_t n=99; bool last=true;
    assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_WAIT && !n && !last);
    if (mode==0) {
        assert(streamoo_byte_ring_write(&r,data,1)==1);
        assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_WAIT);
        streamoo_byte_ring_finish(&r,false);
        assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_DATA && n==1 && last);
        assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_END && !n && !last);
        assert(streamoo_byte_ring_write(&r,data,1)==0);
    } else if (mode==1) {
        size_t written=0,read=0; unsigned stalls=0;
        while (read<sizeof data) {
            size_t wanted=sizeof data-written;
            size_t accepted=streamoo_byte_ring_write(&r,data+written,wanted);
            if (accepted<wanted) ++stalls;
            written+=accepted;
            assert(r.count<=r.capacity);
            if (written==sizeof data) streamoo_byte_ring_finish(&r,false);
            streamoo_source_result_t result=streamoo_byte_ring_read(&r,out+read,7,&n,&last);
            assert(result==STREAMOO_SOURCE_DATA); read+=n;
            assert(last==(read==sizeof data));
        }
        assert(stalls && !memcmp(data,out,sizeof data) && !r.count);
    } else if (mode==2) {
        assert(streamoo_byte_ring_write(&r,data,17)==17);
        streamoo_byte_ring_finish(&r,true);
        streamoo_byte_ring_finish(&r,false);
        assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_ERROR && !n && !last);
        assert(streamoo_byte_ring_write(&r,data,1)==0);
        streamoo_byte_ring_clear(&r);
        assert(streamoo_byte_ring_write(&r,data,17)==17);
        assert(!r.failed && !r.ended);
    } else {
        streamoo_byte_ring_finish(&r,false);
        assert(streamoo_byte_ring_read(&r,out,20,&n,&last)==STREAMOO_SOURCE_END);
        assert(streamoo_byte_ring_read(&r,out,0,&n,&last)==STREAMOO_SOURCE_ERROR);
        assert(streamoo_byte_ring_read(NULL,out,20,&n,&last)==STREAMOO_SOURCE_ERROR);
    }
    return 0;
}
'''
class ByteSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(); cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name); (base/'test.c').write_text(HARNESS); cls.binary=base/'test'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Icomponents/compressed_source/include',
            str(base/'test.c'),'components/compressed_source/streamoo_byte_source.c','-o',str(cls.binary)],cwd=ROOT,check=True)
    def run_case(self,mode): subprocess.run([str(self.binary),str(mode)],check=True,timeout=5)
    def test_final_byte_and_terminal_state(self): self.run_case(0)
    def test_partial_write_wrap_backpressure_and_order(self): self.run_case(1)
    def test_sticky_failure_and_clear(self): self.run_case(2)
    def test_empty_eof_and_bad_arguments(self): self.run_case(3)
