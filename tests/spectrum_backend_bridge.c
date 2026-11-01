#include "streamoo_spectrum.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
#include "dsps_fft2r.h"
#endif

typedef struct {
    _Alignas(16) uint64_t before[2];
    streamoo_spectrum_t spectrum;
    uint64_t after[2];
} guarded_spectrum_t;

void *spectrum_create(unsigned n, unsigned rate) {
    guarded_spectrum_t *g=aligned_alloc(16,sizeof *g);
    if(!g)return NULL;
    g->before[0]=g->before[1]=UINT64_C(0x123456789abcdef0);
    g->after[0]=g->after[1]=UINT64_C(0xfedcba9876543210);
    if(!streamoo_spectrum_init(&g->spectrum,n,rate)){free(g);return NULL;}
    return g;
}
void spectrum_destroy(void *context) {
    guarded_spectrum_t *g=context;
    assert(g->before[0]==UINT64_C(0x123456789abcdef0));
    assert(g->before[1]==UINT64_C(0x123456789abcdef0));
    assert(g->after[0]==UINT64_C(0xfedcba9876543210));
    assert(g->after[1]==UINT64_C(0xfedcba9876543210));
    free(g);
}
bool spectrum_run(void *context,const int32_t *lr,size_t frames) {
    guarded_spectrum_t *g=context;
    return streamoo_spectrum_analyze(&g->spectrum,lr,frames);
}
const float *spectrum_amplitudes(void *context) {
    return ((guarded_spectrum_t *)context)->spectrum.amplitude;
}
const uint16_t *spectrum_bands(void *context) {
    return ((guarded_spectrum_t *)context)->spectrum.bands;
}
size_t spectrum_workspace_bytes(void) {return sizeof(streamoo_spectrum_t);}

#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
bool spectrum_rejects_misalignment_and_table_conflict(void) {
    unsigned char *storage=aligned_alloc(16,sizeof(streamoo_spectrum_t)+16);
    assert(storage);memset(storage,0xa5,sizeof(streamoo_spectrum_t)+16);
    bool rejected=!streamoo_spectrum_init((streamoo_spectrum_t *)(void *)(storage+4),1024,48000);
    for(size_t i=0;i<sizeof(streamoo_spectrum_t)+16;++i)assert(storage[i]==0xa5);
    free(storage);
    /* ESP-DSP returns ESP_OK when already initialized by someone else.
     * The adapter must not silently use that other table. */
    static _Alignas(16) int16_t other_table[2048];
    dsps_fft2r_deinit_sc16();
    assert(dsps_fft2r_init_sc16(other_table,2048)==ESP_OK);
    guarded_spectrum_t *g=aligned_alloc(16,sizeof *g);assert(g);
    rejected=rejected&&!streamoo_spectrum_init(&g->spectrum,1024,48000);
    free(g);dsps_fft2r_deinit_sc16();
    return rejected;
}
#endif

#ifdef STREAMOO_SC16_HOST_TEST
int main(void) {
    /* ASan/UBSan exercise the real ESP-DSP ANSI fixed-point arithmetic and
     * adapter. AES3 instruction execution itself requires an ESP32-S3. */
    int32_t *lr=calloc(4096,sizeof *lr);
    assert(lr);
    assert(spectrum_rejects_misalignment_and_table_conflict());
    for(unsigned n=1024;n<=2048;n*=2) {
        void *g=spectrum_create(n,48000);assert(g);
        for(unsigned i=0;i<n;++i) {
            lr[2*i]=(i%3)?INT32_MAX:INT32_MIN;
            lr[2*i+1]=(i%2)?INT32_MAX:INT32_MIN;
        }
        assert(spectrum_run(g,lr,n));
        assert(!spectrum_run(g,lr,n-1));
        assert(!spectrum_run(g,NULL,n));
        spectrum_destroy(g);
    }
    free(lr);
    return 0;
}
#endif
