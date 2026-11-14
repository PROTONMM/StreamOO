#include "streamoo_spectrum.h"
#include <math.h>
#include <string.h>
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
#include "dsps_fft2r.h"
#include "esp_err.h"
#endif
#define PI 3.14159265358979323846

#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
/* ESP-DSP keeps one process-wide table pointer. A static aligned table gives
 * it firmware lifetime and avoids allocation when a player session starts. */
static _Alignas(16) int16_t sc16_twiddle[STREAMOO_SPECTRUM_MAX];
#endif

const char *streamoo_spectrum_backend(void) {
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
    return "esp-dsp-sc16-aes3-simd";
#else
    return "portable-float-radix2";
#endif
}

bool streamoo_spectrum_init(streamoo_spectrum_t *s, uint32_t n, uint32_t rate) {
    if(!s || (n!=1024 && n!=2048) || rate<8000 || rate>48000) return false;
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
    if((uintptr_t)s%16)return false;
    if(dsps_fft2r_init_sc16(sc16_twiddle,STREAMOO_SPECTRUM_MAX)!=ESP_OK ||
       dsps_fft_w_table_sc16!=sc16_twiddle ||
       dsps_fft_w_table_sc16_size!=STREAMOO_SPECTRUM_MAX)return false;
#endif
    memset(s,0,sizeof *s);
    s->size=n;s->sample_rate=rate;
    double window_sum=0;
    for(uint32_t i=0;i<n;++i) {
        s->window[i]=(float)(0.5*(1.0-cos(2.0*PI*i/(n-1))));
        window_sum+=s->window[i];
    }
    s->window_sum=(float)window_sum;
#if !(defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD)
    for(uint32_t k=0;k<n/2;++k) {
        s->twiddle[2*k]=(float)cos(-2.0*PI*k/n);
        s->twiddle[2*k+1]=(float)sin(-2.0*PI*k/n);
    }
#endif
    s->edges[0]=1;
    for(unsigned b=1;b<STREAMOO_SPECTRUM_BANDS;++b) {
        unsigned edge=(unsigned)lround(pow((double)(n/2), (double)b/STREAMOO_SPECTRUM_BANDS));
        if(edge<=s->edges[b-1])edge=s->edges[b-1]+1;
        unsigned last=n/2+1-(STREAMOO_SPECTRUM_BANDS-b);
        if(edge>last)edge=last;
        s->edges[b]=(uint16_t)edge;
    }
    s->edges[STREAMOO_SPECTRUM_BANDS]=(uint16_t)(n/2+1);
    return true;
}
#if !(defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD)
static void fft(streamoo_spectrum_t *s) {
    unsigned n=s->size;
    for(unsigned i=1,j=0;i<n;++i) {
        unsigned bit=n>>1;
        for(;j&bit;bit>>=1)j^=bit;
        j^=bit;
        if(i<j)for(unsigned c=0;c<2;++c) {
            float tmp=s->work[2*i+c];s->work[2*i+c]=s->work[2*j+c];s->work[2*j+c]=tmp;
        }
    }
    for(unsigned length=2;length<=n;length*=2) {
        unsigned half=length/2,step=n/length;
        for(unsigned base=0;base<n;base+=length)for(unsigned k=0;k<half;++k) {
            unsigned a=2*(base+k),b=2*(base+k+half),w=2*k*step;
            float re=s->work[b]*s->twiddle[w]-s->work[b+1]*s->twiddle[w+1];
            float im=s->work[b]*s->twiddle[w+1]+s->work[b+1]*s->twiddle[w];
            float ar=s->work[a],ai=s->work[a+1];
            s->work[a]=ar+re;s->work[a+1]=ai+im;
            s->work[b]=ar-re;s->work[b+1]=ai-im;
        }
    }
}
#endif
bool streamoo_spectrum_analyze(streamoo_spectrum_t *s, const int32_t *lr, size_t frames) {
    if(!s || !lr || (s->size!=1024 && s->size!=2048) || frames!=s->size) return false;
    unsigned n=s->size;
    for(unsigned k=0;k<=n/2;++k)s->amplitude[k]=0;
    for(unsigned ch=0;ch<2;++ch) {
        float base=(float)lr[ch]*(1.0f/2147483648.0f);
        float weighted_sum=0;
        for(unsigned i=0;i<n;++i)
            weighted_sum+=(((float)lr[2*i+ch]*(1.0f/2147483648.0f))-base)*s->window[i];
        float mean=weighted_sum/s->window_sum;
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
        /* Per-channel block scaling preserves quiet signals and gives the
         * windowed, DC-removed signal headroom without clipping transients.
         * Undo this scale after the normalized (DFT/N) fixed-point FFT. */
        float peak=0;
        for(unsigned i=0;i<n;++i) {
            float value=fabsf((((float)lr[2*i+ch]*(1.0f/2147483648.0f))-base-mean)*s->window[i]);
            if(value>peak)peak=value;
        }
        if(peak==0)continue;
        float to_fixed=16383.0f/peak;
        float from_fixed=peak/16383.0f;
#endif
        for(unsigned i=0;i<n;++i) {
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
            float value=(((float)lr[2*i+ch]*(1.0f/2147483648.0f))-base-mean)*s->window[i];
            long q=lroundf(value*to_fixed);
            if(q>16383)q=16383;
            if(q<-16383)q=-16383;
            s->work[2*i]=(int16_t)q;
            s->work[2*i+1]=0;
#else
            s->work[2*i]=(((float)lr[2*i+ch]*(1.0f/2147483648.0f))-base-mean)*s->window[i];
            s->work[2*i+1]=0;
#endif
        }
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
        if(dsps_fft2r_sc16_aes3(s->work,(int)n)!=ESP_OK ||
           dsps_bit_rev_sc16_ansi(s->work,(int)n)!=ESP_OK)return false;
#else
        fft(s);
#endif
        for(unsigned k=1;k<=n/2;++k) {
#if defined(ESP_PLATFORM) && CONFIG_STREAMOO_DSP_ENABLE_SIMD
            /* sc16 scales every radix-2 stage, so its result is DFT/N. */
            float scale=(k==n/2?(float)n:2.0f*(float)n)*from_fixed/s->window_sum;
#else
            float scale=(k==n/2?1.0f:2.0f)/s->window_sum;
#endif
            float re=s->work[2*k]*scale,im=s->work[2*k+1]*scale;
            s->amplitude[k]+=0.5f*(re*re+im*im);
        }
    }
    for(unsigned k=1;k<=n/2;++k)s->amplitude[k]=sqrtf(s->amplitude[k]);
    for(unsigned b=0;b<STREAMOO_SPECTRUM_BANDS;++b) {
        float peak=0;
        for(unsigned k=s->edges[b];k<s->edges[b+1];++k)
            if(s->amplitude[k]>peak)peak=s->amplitude[k];
        s->bands[b]=peak>=1.0f ? 32767 : (uint16_t)lroundf(peak*32767.0f);
    }
    return true;
}
