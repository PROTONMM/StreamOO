#include "streamoo_audio_output.h"
#include "driver/i2s_std.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

struct streamoo_audio_output {
    i2s_chan_handle_t channel;
    streamoo_audio_fill_fn fill;
    void *context;
    portMUX_TYPE lock;
    streamoo_audio_stats_t stats;
};

static bool IRAM_ATTR audio_dma_sent(i2s_chan_handle_t channel, i2s_event_data_t *event, void *context)
{
    (void)channel;
    streamoo_audio_output_t *s=context;
    const uint32_t start=(uint32_t)esp_timer_get_time();
    bool invalid=!event->dma_buf || event->size!=STREAMOO_AUDIO_DMA_FRAMES*2*sizeof(int32_t);
    if (!invalid) s->fill(s->context,event->dma_buf,STREAMOO_AUDIO_DMA_FRAMES);
    const uint32_t elapsed=(uint32_t)esp_timer_get_time()-start;
    portENTER_CRITICAL_ISR(&s->lock);
    ++s->stats.buffers;
    if (invalid) ++s->stats.invalid_buffers;
    if (elapsed>s->stats.max_fill_us) s->stats.max_fill_us=elapsed;
    portEXIT_CRITICAL_ISR(&s->lock);
    return false;
}

esp_err_t streamoo_audio_output_open(streamoo_audio_pins_t pins, streamoo_audio_fill_fn fill,
                                      void *context, streamoo_audio_output_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out=NULL;
    if (!fill || !context || !esp_ptr_in_iram((const void *)fill) || !esp_ptr_internal(context))
        return ESP_ERR_INVALID_ARG;
    streamoo_audio_output_t *s=heap_caps_calloc(1,sizeof(*s),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if (!s) return ESP_ERR_NO_MEM;
    s->lock=(portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    s->fill=fill; s->context=context;
    i2s_chan_config_t channel=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
    channel.dma_desc_num=STREAMOO_AUDIO_DMA_BUFFERS;
    channel.dma_frame_num=STREAMOO_AUDIO_DMA_FRAMES;
    channel.auto_clear_before_cb=true;
    channel.auto_clear_after_cb=false;
    esp_err_t error=i2s_new_channel(&channel,&s->channel,NULL);
    if (error!=ESP_OK) { heap_caps_free(s); return error; }
    const i2s_std_config_t config={
        .clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(STREAMOO_AUDIO_RATE),
        .slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,I2S_SLOT_MODE_STEREO),
        .gpio_cfg={.mclk=I2S_GPIO_UNUSED,.bclk=pins.bclk,.ws=pins.ws,
                   .dout=pins.dout,.din=I2S_GPIO_UNUSED},
    };
    error=i2s_channel_init_std_mode(s->channel,&config);
    const i2s_event_callbacks_t callbacks={.on_sent=audio_dma_sent};
    if (error==ESP_OK) error=i2s_channel_register_event_callback(s->channel,&callbacks,s);
    /* Explicitly preload silence in every descriptor before clocks start. */
    static const int32_t silence[STREAMOO_AUDIO_DMA_FRAMES*2]={0};
    for (unsigned i=0; error==ESP_OK && i<STREAMOO_AUDIO_DMA_BUFFERS; ++i) {
        size_t loaded=0;
        error=i2s_channel_preload_data(s->channel,silence,sizeof silence,&loaded);
        if (error==ESP_OK && loaded!=sizeof silence) error=ESP_ERR_INVALID_SIZE;
    }
    if (error==ESP_OK) error=i2s_channel_enable(s->channel);
    if (error!=ESP_OK) { i2s_del_channel(s->channel); heap_caps_free(s); return error; }
    *out=s;
    return ESP_OK;
}

bool streamoo_audio_output_stats(streamoo_audio_output_t *s, streamoo_audio_stats_t *out)
{
    if (!s || !out) return false;
    portENTER_CRITICAL(&s->lock);
    *out=s->stats;
    portEXIT_CRITICAL(&s->lock);
    return true;
}
esp_err_t streamoo_audio_output_close(streamoo_audio_output_t *s)
{
    if (!s) return ESP_ERR_INVALID_ARG;
    esp_err_t error=i2s_channel_disable(s->channel);
    if (error!=ESP_OK) return error;
    error=i2s_del_channel(s->channel);
    if (error==ESP_OK) heap_caps_free(s);
    return error;
}
