#include "streamoo_clock.h"
#include <string.h>
static const char *const zones[]={"UTC","Europe/Warsaw","Europe/London","Europe/Berlin",
    "Europe/Paris","Europe/Helsinki","Europe/Madrid","Europe/Rome"};
const char *streamoo_clock_zone_name(streamoo_clock_zone_t z){return (unsigned)z<STREAMOO_CLOCK_ZONE_COUNT ? zones[z]:NULL;}
bool streamoo_clock_zone_parse(const char *name,streamoo_clock_zone_t *out){
    if(!name||!out)return false;
    for(unsigned i=0;i<STREAMOO_CLOCK_ZONE_COUNT;++i)if(!strcmp(name,zones[i])){*out=(streamoo_clock_zone_t)i;return true;}
    return false;
}
static unsigned leap(unsigned y){return y%4==0 && (y%100!=0 || y%400==0);}
static unsigned month_days(unsigned y,unsigned m){
    static const unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    return days[m-1]+(m==2 && leap(y));
}
static int64_t year_days(unsigned y){
    int64_t days=0;
    for(unsigned i=1970;i<y;++i)days+=365+leap(i);
    return days;
}
static streamoo_clock_civil_t civil(int64_t seconds){
    int64_t days=seconds/86400;unsigned rest=(unsigned)(seconds%86400),y=1970,m=1;
    while(days>=365+leap(y)){days-=365+leap(y);++y;}
    while(days>=month_days(y,m)){days-=month_days(y,m);++m;}
    return (streamoo_clock_civil_t){(uint16_t)y,(uint8_t)m,(uint8_t)(days+1),
        (uint8_t)(rest/3600),(uint8_t)(rest/60%60),(uint8_t)(rest%60)};
}
static int64_t transition(unsigned year,unsigned month){
    int64_t days=year_days(year);
    for(unsigned m=1;m<month;++m)days+=month_days(year,m);
    days+=30; /* March/October31, then back to last Sunday. */
    days-=(days+4)%7; /*1970-01-01 was Thursday; Sunday=0. */
    return days*86400+3600;
}
bool streamoo_clock_project(int64_t utc,bool synced,streamoo_clock_zone_t zone,
                            streamoo_clock_civil_t *out){
    if(!out)return false;
    *out=(streamoo_clock_civil_t){0};
    if(!synced || utc<1577836800 || utc>=4102444800LL ||
        (unsigned)zone>=STREAMOO_CLOCK_ZONE_COUNT)return false;
    int offset=0;
    if(zone!=STREAMOO_CLOCK_UTC){
        unsigned y=civil(utc).year;
        int winter=zone==STREAMOO_CLOCK_LONDON ? 0 : zone==STREAMOO_CLOCK_HELSINKI ? 7200:3600;
        offset=winter+(utc>=transition(y,3) && utc<transition(y,10) ? 3600:0);
    }
    *out=civil(utc+offset);
    return true;
}
