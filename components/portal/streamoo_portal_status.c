#include "streamoo_portal_status.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
bool streamoo_portal_status_json(const streamoo_portal_status_t *s,char *out,size_t capacity)
{
    if (!s || !out || !capacity || s->clients>4 || s->volume>100) return false;
    if (s->ap_ssid[0]) {
        if (memcmp(s->ap_ssid,"StreamOO-",9) || s->ap_ssid[15]) return false;
        for (unsigned i=9;i<15;++i) if (!((s->ap_ssid[i]>='0' && s->ap_ssid[i]<='9') ||
            (s->ap_ssid[i]>='A' && s->ap_ssid[i]<='F'))) return false;
    } else if (s->ap_ready) return false;
    int count=snprintf(out,capacity,
        "{\"schema_version\":1,\"network\":{\"mode\":\"ap\",\"ready\":%s,\"ssid\":\"%s\",\"clients\":%u},"
        "\"audio\":{\"available\":%s,\"source\":\"test_fixture\",\"running\":%s,\"buffering\":%s,\"volume\":%u},"
        "\"uptime_ms\":%" PRIu64 "}",s->ap_ready ? "true":"false",s->ap_ssid,(unsigned)s->clients,
        s->audio_available ? "true":"false",s->running ? "true":"false",s->buffering ? "true":"false",
        (unsigned)s->volume,s->uptime_ms);
    return count>=0 && (size_t)count<capacity;
}
