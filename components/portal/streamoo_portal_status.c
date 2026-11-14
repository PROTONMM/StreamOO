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
    size_t ip_bytes=0;while(ip_bytes<sizeof s->sta_ip&&s->sta_ip[ip_bytes])++ip_bytes;
    if(ip_bytes==sizeof s->sta_ip||s->saved_profiles>8)return false;
    for(size_t i=0;i<ip_bytes;++i)if((s->sta_ip[i]<'0'||s->sta_ip[i]>'9')&&s->sta_ip[i]!='.')return false;
    size_t codec_bytes=0;
    while(codec_bytes<sizeof s->codec && s->codec[codec_bytes]) {
        char c=s->codec[codec_bytes++];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='?'))return false;
    }
    if(codec_bytes==sizeof s->codec || s->bitrate_kbps>4096 || s->sample_rate>192000)return false;
    const char *mode=s->sta_connected?(s->ap_ready?"apsta":"sta"):"ap";
    const char *ip=s->sta_connected?s->sta_ip:(s->ap_ready?"192.168.4.1":"");
    int count=snprintf(out,capacity,
        "{\"schema_version\":1,\"network\":{\"mode\":\"%s\",\"ready\":%s,\"ssid\":\"%s\",\"clients\":%u,\"ap_ready\":%s,\"connected\":%s,\"configuring\":%s,\"ip\":\"%s\",\"profiles\":%u,\"error\":%u},"
        "\"audio\":{\"available\":%s,\"source\":\"%s\",\"running\":%s,\"buffering\":%s,\"volume\":%u,\"station_id\":\"%016" PRIx64 "\",\"codec\":\"%s\",\"bitrate_kbps\":%" PRIu32 ",\"sample_rate\":%" PRIu32 ",\"bitrate_measured\":%s,\"bitrate_variable\":%s},"
        "\"uptime_ms\":%" PRIu64 "}",mode,(s->ap_ready||s->sta_connected) ? "true":"false",s->ap_ssid,(unsigned)s->clients,
        s->ap_ready?"true":"false",s->sta_connected?"true":"false",s->configuring?"true":"false",ip,(unsigned)s->saved_profiles,(unsigned)s->network_error,
        s->audio_available ? "true":"false",s->network_source?"network_stream":"test_fixture",s->running ? "true":"false",s->buffering ? "true":"false",
        (unsigned)s->volume,s->station_id,s->codec,s->bitrate_kbps,s->sample_rate,
        s->bitrate_measured?"true":"false",s->bitrate_variable?"true":"false",s->uptime_ms);
    return count>=0 && (size_t)count<capacity;
}
