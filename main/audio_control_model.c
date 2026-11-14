#include "audio_control_model.h"
#include "streamoo_audio_gain.h"
bool streamoo_audio_control_model_init(streamoo_audio_control_model_t *m, uint8_t volume)
{
    if (!m || volume>100) return false;
    *m=(streamoo_audio_control_model_t){.volume=volume};
    return true;
}
bool streamoo_audio_control_model_apply(streamoo_audio_control_model_t *m, const streamoo_control_command_t *c)
{
    if (!m || !c || m->volume>100) return false;
    if (c->kind==STREAMOO_CONTROL_TOGGLE) m->running=!m->running;
    else if(c->kind==STREAMOO_CONTROL_PLAY)m->running=true;
    else if(c->kind==STREAMOO_CONTROL_STOP)m->running=false;
    else if (c->kind==STREAMOO_CONTROL_VOLUME_DELTA) {
        int64_t volume=(int64_t)m->volume+c->delta;
        uint8_t next=volume<0 ? 0 : volume>100 ? 100 : (uint8_t)volume;
        if (next!=m->volume) { m->volume=next; ++m->volume_revision; }
    } else return false;
    return true;
}
bool streamoo_audio_control_model_gain(const streamoo_audio_control_model_t *m, uint32_t *gain)
{
    if (!m || !gain || m->volume>100) return false;
    if (!m->running) { *gain=0; return true; }
    return streamoo_audio_gain_volume(m->volume,gain);
}
