#include "streamoo_clock_settings.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static int scenario,commits;static bool has,fail_commit,locked;
static char stored[32],pending[32];
int nvs_flash_init(void){return scenario==2?ESP_FAIL:ESP_OK;}
void *xSemaphoreCreateMutex(void){return scenario==5?NULL:(void *)1;}
int xSemaphoreTake(void *p,unsigned timeout){assert(p&&timeout==5000&&!locked);locked=true;return 1;}
void xSemaphoreGive(void *p){assert(p&&locked);locked=false;}
int nvs_open(const char *name,int mode,int *handle){assert(!strcmp(name,"streamoo_clock"));*handle=1;return !has&&mode==0?ESP_ERR_NVS_NOT_FOUND:ESP_OK;}
int nvs_get_str(int h,const char *key,char *out,size_t *n){assert(h&&!strcmp(key,"zone"));if(scenario==4)return ESP_FAIL;assert(*n>strlen(stored));strcpy(out,stored);*n=strlen(stored)+1;return ESP_OK;}
int nvs_set_str(int h,const char *key,const char *value){assert(h&&!strcmp(key,"zone")&&locked);strcpy(pending,value);return ESP_OK;}
int nvs_commit(int h){assert(h);++commits;if(fail_commit)return ESP_FAIL;strcpy(stored,pending);has=true;
 const char *path=getenv("STREAMOO_CLOCK_TEST_NVS");
 if(path){FILE *f=fopen(path,"wb");assert(f);assert(fwrite(stored,1,strlen(stored)+1,f)==strlen(stored)+1);assert(!fclose(f));}
 return ESP_OK;}
void nvs_close(int h){assert(h);}
int main(int argc,char **argv){
 streamoo_clock_civil_t civil;
 for(int zone=0;zone<STREAMOO_CLOCK_ZONE_COUNT;++zone){
  assert(streamoo_clock_project(1793448000,true,zone,&civil));
  assert(civil.year==2026&&civil.month>=1&&civil.month<=12&&civil.day>=1&&civil.day<=31);
 }
 assert(!streamoo_clock_project(0,true,STREAMOO_CLOCK_POLAND,&civil)&&!civil.year);
 assert(argc==2);scenario=atoi(argv[1]);
 if(scenario==7){
  FILE *f=fopen(getenv("STREAMOO_CLOCK_TEST_NVS"),"rb");assert(f);
  assert(fread(stored,1,sizeof stored,f)>0);assert(!fclose(f));has=true;
 }
 if(scenario==1){has=true;strcpy(stored,"Europe/London");}
 if(scenario==3){has=true;strcpy(stored,"Unknown/Zone");}
 if(scenario==4){has=true;strcpy(stored,"UTC");}
 int result=streamoo_clock_settings_start();
 if(scenario==2||scenario==4||scenario==5){assert(result!=ESP_OK);assert(streamoo_clock_settings_get()==STREAMOO_CLOCK_POLAND);assert(streamoo_clock_settings_set("UTC")==ESP_ERR_INVALID_STATE);return 0;}
 if(scenario==6){assert(result==ESP_OK);assert(streamoo_clock_settings_set("Europe/London")==ESP_OK);return 0;}
 if(scenario==7){assert(result==ESP_OK);assert(streamoo_clock_settings_get()==STREAMOO_CLOCK_LONDON&&!commits);return 0;}
 assert(result==ESP_OK);assert(streamoo_clock_settings_start()==ESP_ERR_INVALID_STATE);
 assert(streamoo_clock_settings_get()==(scenario==1?STREAMOO_CLOCK_LONDON:STREAMOO_CLOCK_POLAND));
 assert(streamoo_clock_settings_set("UTC")==ESP_OK);assert(commits==1&&!strcmp(stored,"UTC"));
 assert(streamoo_clock_settings_get()==STREAMOO_CLOCK_UTC);
 assert(streamoo_clock_settings_set("UTC")==ESP_OK&&commits==1);
 fail_commit=true;assert(streamoo_clock_settings_set("Europe/London")==ESP_FAIL);
 assert(streamoo_clock_settings_get()==STREAMOO_CLOCK_UTC&&!strcmp(stored,"UTC"));
 assert(streamoo_clock_settings_set("bad")==ESP_ERR_INVALID_ARG&&commits==2);
 return 0;
}
