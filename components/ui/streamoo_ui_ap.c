#include "streamoo_ui_ap.h"
#include "streamoo_text.h"
#include "streamoo_font5x8.h"
static bool printable(const uint8_t *text,size_t bytes)
{
    if (!text) return false;
    for (size_t i=0;i<bytes;++i) if (text[i]<32 || text[i]>126) return false;
    return true;
}
static bool line(streamoo_canvas_t *c,int y,const uint8_t *text,size_t bytes,uint8_t gray)
{
    return streamoo_text_draw(c,0,y,text,bytes,gray,streamoo_font5x8,NULL);
}
bool streamoo_ui_ap_draw(uint8_t *frame,size_t bytes,const uint8_t *ssid,size_t ssid_bytes,
    const uint8_t *password,size_t password_bytes)
{
    if (!ssid_bytes || ssid_bytes>32 || password_bytes<8 || password_bytes>63 ||
        !printable(ssid,ssid_bytes) || !printable(password,password_bytes)) return false;
    streamoo_canvas_t c;
    if (!streamoo_canvas_init(&c,frame,bytes,256,64,128)) return false;
    streamoo_canvas_fill(&c,(streamoo_rect_t){0,0,256,64},0);
    static const uint8_t title[]="StreamOO / konfiguracja", label[]="Sieć Wi-Fi:", key[]="Hasło AP:";
    static const uint8_t url[]="http://192.168.4.1";
    size_t first=password_bytes>42 ? 42:password_bytes;
    return line(&c,0,title,sizeof title-1,10) && line(&c,8,label,sizeof label-1,10) &&
        line(&c,16,ssid,ssid_bytes,15) && line(&c,24,key,sizeof key-1,10) &&
        line(&c,32,password,first,15) && line(&c,40,password+first,password_bytes-first,15) &&
        line(&c,56,url,sizeof url-1,12);
}
