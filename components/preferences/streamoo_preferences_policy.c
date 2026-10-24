#include "streamoo_preferences_policy.h"

bool streamoo_preferences_value_valid(streamoo_preferences_value_t v)
{ return v.volume <= 100 && v.visualization <= 2; }
bool streamoo_preferences_equal(streamoo_preferences_value_t a, streamoo_preferences_value_t b)
{ return a.volume == b.volume && a.visualization == b.visualization; }
bool streamoo_preferences_encode(streamoo_preferences_value_t v, uint8_t *out, size_t bytes)
{
    if (!out || bytes != STREAMOO_PREFERENCES_BYTES || !streamoo_preferences_value_valid(v)) return false;
    const uint8_t data[8] = {'S','O',1,v.volume,v.visualization,0,0,0};
    for (unsigned i=0; i<8; ++i) out[i]=data[i];
    return true;
}
bool streamoo_preferences_decode(const uint8_t *data, size_t bytes, streamoo_preferences_value_t *out)
{
    if (!data || !out || bytes != 8 || data[0]!='S' || data[1]!='O' || data[2]!=1 ||
        data[5] || data[6] || data[7]) return false;
    streamoo_preferences_value_t value={data[3],data[4]};
    if (!streamoo_preferences_value_valid(value)) return false;
    *out=value;
    return true;
}
bool streamoo_preferences_policy_init(streamoo_preferences_policy_t *p, streamoo_preferences_value_t saved)
{
    if (!p || !streamoo_preferences_value_valid(saved)) return false;
    *p=(streamoo_preferences_policy_t){.current=saved,.saved=saved};
    return true;
}
bool streamoo_preferences_observe(streamoo_preferences_policy_t *p, streamoo_preferences_value_t v, uint64_t now)
{
    if (!p || !streamoo_preferences_value_valid(v)) return false;
    p->current=v;
    p->changed_ms=now;
    p->dirty=!streamoo_preferences_equal(p->saved,v);
    return true;
}
bool streamoo_preferences_due(const streamoo_preferences_policy_t *p, uint64_t now)
{
    return p && p->dirty && now>=p->changed_ms && now-p->changed_ms>=STREAMOO_PREFERENCES_DELAY_MS &&
        (!p->attempted || (now>=p->attempted_ms && now-p->attempted_ms>=STREAMOO_PREFERENCES_DELAY_MS));
}
void streamoo_preferences_result(streamoo_preferences_policy_t *p, bool success, uint64_t now)
{
    if (!p) return;
    p->attempted=true;
    p->attempted_ms=now;
    if (success) { p->saved=p->current; p->dirty=false; }
}
