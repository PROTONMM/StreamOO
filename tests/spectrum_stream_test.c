#include "streamoo_spectrum_stream.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int32_t samples[8192];
int main(int argc,char **argv){
 assert(argc==2);int test=atoi(argv[1]);
 streamoo_spectrum_stream_t *s=calloc(1,sizeof *s);streamoo_spectrum_t *oracle=calloc(1,sizeof *oracle);
 assert(s&&oracle&&streamoo_spectrum_stream_init(s)&&streamoo_spectrum_init(oracle,1024,48000));
 for(unsigned i=0;i<4096;++i){samples[2*i]=(int32_t)((i*1234567u)%1073741824u);samples[2*i+1]=-samples[2*i];}
 if(test==0){
  assert(!streamoo_spectrum_stream_init(NULL));
  assert(!streamoo_spectrum_stream_feed(NULL,samples,240,0,true));
  assert(!streamoo_spectrum_stream_feed(s,NULL,240,0,true));
  assert(!streamoo_spectrum_stream_feed(s,samples,0,0,true));
  assert(!streamoo_spectrum_stream_feed(s,samples,241,0,true));assert(!s->filled&&!s->seen);
 }else if(test==1){
  unsigned total=0,windows=0;
  for(unsigned seq=0;seq<16;++seq){
   bool done=streamoo_spectrum_stream_feed(s,samples+2*total,240,seq,true);total+=240;
   if(done){assert(streamoo_spectrum_analyze(oracle,samples+2*(windows*512),1024));
    assert(!memcmp(oracle->bands,s->fft.bands,sizeof oracle->bands));++windows;}
  }
  assert(windows==6&&s->windows==6&&s->filled==768&&!s->gaps);
 }else if(test==2){
  for(unsigned seq=0;seq<4;++seq)assert(!streamoo_spectrum_stream_feed(s,samples+seq*480,240,seq,true));
  assert(s->filled==960);
  for(unsigned seq=5;seq<10;++seq){
   bool done=streamoo_spectrum_stream_feed(s,samples+seq*480,240,seq,true);
   assert(done==(seq==9));
  }
  assert(s->gaps==1&&s->windows==1);
  assert(streamoo_spectrum_analyze(oracle,samples+5*480,1024));
  assert(!memcmp(oracle->bands,s->fft.bands,sizeof oracle->bands));
 }else if(test==3){
  for(unsigned seq=0;seq<10;++seq)
   assert(!streamoo_spectrum_stream_feed(s,samples+seq*480,240,seq,false));
  assert(!s->windows&&s->skipped==3);
  assert(streamoo_spectrum_stream_feed(s,samples+10*480,240,10,true));
  assert(!streamoo_spectrum_stream_feed(s,samples+11*480,240,11,true));
  assert(s->windows==1&&s->skipped==3);
 }else if(test==4 || test==6){
  uint16_t previous=0;
  for(unsigned value=0;value<=65535;++value){
   uint16_t current=streamoo_spectrum_display_level((uint16_t)value);
   assert(current>=previous&&current<=32767);previous=current;
  }
  assert(!streamoo_spectrum_display_level(0));
  assert(!streamoo_spectrum_display_level(184));
  assert(streamoo_spectrum_display_level(32767)==32767);
  const unsigned values[]={328,1036,3277,10362};
  const unsigned pixels[]={0,3,9,18};
  for(unsigned i=0;i<4;++i)
   assert(streamoo_spectrum_display_level(values[i])*31u/32767u==pixels[i]);
 }else if(test==5){
  assert(!streamoo_spectrum_stream_feed(s,samples,240,UINT32_MAX,true));
  assert(!streamoo_spectrum_stream_feed(s,samples,240,0,true));assert(!s->gaps&&s->filled==480);
  float first=s->fft.window[256];streamoo_spectrum_stream_reset(s);
  assert(!s->filled&&!s->seen&&s->fft.window[256]==first);
  assert(!streamoo_spectrum_stream_feed(s,samples,240,99,true));assert(!s->gaps);
 }else abort();
 free(s);free(oracle);return 0;
}
