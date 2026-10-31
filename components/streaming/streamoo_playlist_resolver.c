#include "streamoo_playlist_resolver.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdint.h>

bool streamoo_stream_url_allowed(const char *url)
{
    if(!url)return false;
    unsigned prefix=!strncmp(url,"http://",7)?7:!strncmp(url,"https://",8)?8:0;
    size_t n=0;while(n<=STREAMOO_URL_LIMIT && url[n])++n;
    if(!prefix || n>STREAMOO_URL_LIMIT || n<=prefix)return false;
    const char *end=url+prefix;
    while(*end && *end!='/' && *end!='?')++end;
    if(end==url+prefix || url[prefix]==':')return false;
    for(size_t i=0;i<n;++i)
        if((unsigned char)url[i]<=32 || (unsigned char)url[i]>=127 ||
           url[i]=='@'||url[i]=='#'||url[i]=='\\')return false;
    return true;
}
bool streamoo_playlist_url(const char *url)
{
    if(!url)return false;
    size_t n=strcspn(url,"?");
    return (n>=4 && (!strncasecmp(url+n-4,".m3u",4)||!strncasecmp(url+n-4,".pls",4))) ||
           (n>=5 && !strncasecmp(url+n-5,".m3u8",5));
}
static bool resolve(const char *base,const char *entry,char *out)
{
    if(!*entry)return false;
    int n;
    if(!strncmp(entry,"http://",7)||!strncmp(entry,"https://",8))n=snprintf(out,STREAMOO_URL_LIMIT+1,"%s",entry);
    else {
        const char *authority=strstr(base,"://");
        if(!authority)return false;
        size_t scheme=(size_t)(authority-base);
        const char *path=authority+3;
        while(*path && *path!='/' && *path!='?')++path;
        size_t root=(size_t)(path-base),dir=root;
        const char *query=strchr(path,'?');
        const char *end=query?query:base+strlen(base);
        for(const char *p=path;p<end;++p)if(*p=='/')dir=(size_t)(p-base)+1;
        /* Colons in a relative first path component could introduce another scheme. */
        const char *colon=strchr(entry,':');
        size_t first=strcspn(entry,"/?");
        if(colon && colon<entry+first)return false;
        if(!strncmp(entry,"//",2))n=snprintf(out,STREAMOO_URL_LIMIT+1,"%.*s:%s",(int)scheme,base,entry);
        else if(*entry=='/')n=snprintf(out,STREAMOO_URL_LIMIT+1,"%.*s%s",(int)root,base,entry);
        else if(*entry=='?')n=snprintf(out,STREAMOO_URL_LIMIT+1,"%.*s%s",(int)(end-base),base,entry);
        else n=snprintf(out,STREAMOO_URL_LIMIT+1,"%.*s%s%s",(int)dir,base,dir==root?"/":"",entry);
    }
    if(n<0 || (unsigned)n>STREAMOO_URL_LIMIT || !streamoo_stream_url_allowed(out) ||
       (!strncmp(base,"https://",8) && strncmp(out,"https://",8)))return false;
    /* Dot segments in the path; never inspect or rewrite the query. */
    char *path=strstr(out,"://")+3;
    while(*path && *path!='/' && *path!='?')++path;
    char *query=strchr(path,'?');
    if(!query)query=out+strlen(out);
    if(*path!='/')return true;
    char *read=path+1,*write=read;
    while(read<query){
        char *end=read;while(end<query && *end!='/')++end;
        size_t len=(size_t)(end-read);
        if(len==1 && read[0]=='.'){}
        else if(len==2 && read[0]=='.' && read[1]=='.'){
            if(write>path+1){--write;while(write>path+1 && write[-1]!='/')--write;}
        }else{
            memmove(write,read,len);write+=len;
            if(end<query)*write++='/';
        }
        read=end+(end<query);
    }
    memmove(write,query,strlen(query)+1);
    return true;
}
streamoo_playlist_result_t streamoo_playlist_resolve(char *doc,size_t bytes,const char *base,char out[STREAMOO_URL_LIMIT+1])
{
    if(!doc||!out||!bytes||bytes>STREAMOO_PLAYLIST_LIMIT||!streamoo_stream_url_allowed(base))return STREAMOO_PLAYLIST_INVALID;
    out[0]=0;
    if(memchr(doc,0,bytes))return STREAMOO_PLAYLIST_INVALID;
    doc[bytes]=0;size_t at=bytes>=3&&!memcmp(doc,"\xef\xbb\xbf",3)?3:0;
    bool pls=false;unsigned best=UINT32_MAX;char *chosen=NULL;
    while(at<bytes){
        char *line=doc+at;
        while(at<bytes && doc[at]!='\r' && doc[at]!='\n')++at;
        if(at<bytes)doc[at++]=0;
        while(*line==' '||*line=='\t')++line;
        size_t len=strlen(line);
        while(len && (line[len-1]==' '||line[len-1]=='\t'))line[--len]=0;
        if(!len)continue;
        if(!strncasecmp(line,"#EXT-X-",7))return STREAMOO_PLAYLIST_HLS;
        if(!strcasecmp(line,"[playlist]")){pls=true;chosen=NULL;best=UINT32_MAX;continue;}
        if(*line=='#'||*line==';')continue;
        if(pls){
            if(strncasecmp(line,"File",4))continue;
            char *p=line+4;unsigned index=0;
            while(*p>='0'&&*p<='9'){
                index=index*10u+(unsigned)(*p++-'0');if(index>64)return STREAMOO_PLAYLIST_INVALID;
            }
            while(*p==' '||*p=='\t')++p;
            if(*p!='='||!index)return STREAMOO_PLAYLIST_INVALID;
            ++p;while(*p==' '||*p=='\t')++p;
            if(index<best){best=index;chosen=p;}
        }else if(!chosen)chosen=line;
    }
    return chosen && resolve(base,chosen,out)?STREAMOO_PLAYLIST_OK:STREAMOO_PLAYLIST_INVALID;
}
