#include "encoder_demo.h"
#include "streamoo_font5x8.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned commands, selections;
static bool accept=true;
static streamoo_control_command_t command;
static bool post(void *ctx, const streamoo_control_command_t *c)
{ (void)ctx; ++commands; command=*c; return accept; }
static bool select_station(void *ctx, const streamoo_ui_select_command_t *c)
{ (void)ctx; assert(c->station_id==12 && c->catalog_generation==8); ++selections; return accept; }
static bool send(streamoo_ui_t *ui, int kind, int delta, int mode)
{ return streamoo_ui_handle_input(ui,(streamoo_input_event_t){kind,delta},mode,post,NULL,1000); }
int main(int argc, char **argv)
{
    assert(argc==2 || argc==3);
    int scenario=atoi(argv[1]);
    streamoo_ui_t ui;
    assert(streamoo_ui_init(&ui,160,select_station,NULL));
    static const streamoo_ui_station_t stations[]={{11,(const uint8_t *)"one",3},{12,(const uint8_t *)"two",3}};
    assert(streamoo_ui_set_catalog(&ui,stations,2,8));
    streamoo_ui_set_playing(&ui,11);
    if (scenario==0) {
        assert(send(&ui,STREAMOO_INPUT_ROTATE,-4,0));
        assert(commands==1 && command.kind==STREAMOO_CONTROL_VOLUME_DELTA && command.delta==-4);
        assert(send(&ui,STREAMOO_INPUT_LONG,0,0));
        assert(command.kind==STREAMOO_CONTROL_TOGGLE && commands==2);
        assert(send(&ui,STREAMOO_INPUT_SHORT,0,0)); assert(ui.screen==STREAMOO_UI_STATIONS);
    } else if (scenario==1) {
        assert(send(&ui,STREAMOO_INPUT_SHORT,0,1)); assert(command.kind==STREAMOO_CONTROL_TOGGLE);
        assert(send(&ui,STREAMOO_INPUT_LONG,0,1)); assert(ui.screen==STREAMOO_UI_STATIONS);
    } else if (scenario==2) {
        assert(send(&ui,STREAMOO_INPUT_PRESSED_ROTATE,-1,0)); assert(ui.visualization==2);
        assert(send(&ui,STREAMOO_INPUT_PRESSED_ROTATE,1,0)); assert(ui.visualization==0);
        assert(send(&ui,STREAMOO_INPUT_PRESSED_ROTATE,INT_MIN,0));
        assert(ui.visualization>=0 && ui.visualization<=2 && commands==0);
    } else if (scenario==3) {
        for (int mode=0;mode<2;++mode) {
            assert(streamoo_ui_open_stations(&ui,0));
            assert(send(&ui,STREAMOO_INPUT_ROTATE,1,mode)); assert(ui.selected==1 && ui.playing_id==11);
            assert(send(&ui,STREAMOO_INPUT_PRESSED_ROTATE,-1,mode)); assert(ui.selected==1 && ui.visualization==0);
            assert(send(&ui,STREAMOO_INPUT_SHORT,0,mode)); assert(ui.screen==STREAMOO_UI_HOME && ui.playing_id==11);
            assert(streamoo_ui_open_stations(&ui,0));
            assert(send(&ui,STREAMOO_INPUT_LONG,0,mode)); assert(ui.screen==STREAMOO_UI_HOME);
        }
        assert(selections==2 && commands==0);
    } else if (scenario==4) {
        accept=false; streamoo_ui_t saved=ui;
        assert(!send(&ui,STREAMOO_INPUT_ROTATE,1,0)); assert(!memcmp(&saved,&ui,sizeof ui));
        assert(!send(&ui,STREAMOO_INPUT_LONG,0,0)); assert(!memcmp(&saved,&ui,sizeof ui));
        assert(streamoo_ui_open_stations(&ui,0)); assert(streamoo_ui_move(&ui,1,0));
        saved=ui; assert(!send(&ui,STREAMOO_INPUT_SHORT,0,0)); assert(!memcmp(&saved,&ui,sizeof ui));
    } else if (scenario==5) {
        assert(!send(&ui,99,0,0)); assert(!send(&ui,1,1,9));
        assert(send(&ui,STREAMOO_INPUT_NONE,0,0)); assert(send(&ui,STREAMOO_INPUT_ROTATE,0,0));
        assert(commands==0);
        assert(streamoo_ui_set_catalog(&ui,NULL,0,9)); assert(streamoo_ui_open_stations(&ui,0));
        assert(send(&ui,STREAMOO_INPUT_ROTATE,1,0)); assert(!send(&ui,STREAMOO_INPUT_SHORT,0,0));
        assert(send(&ui,STREAMOO_INPUT_LONG,0,0));
    } else if (scenario==7) {
        streamoo_encoder_demo_t d; assert(streamoo_encoder_demo_init(&d));
        assert(streamoo_encoder_demo_bind_audio(&d,post,NULL));
        assert(streamoo_encoder_demo_audio_state(&d,37,false));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_ROTATE,1},0,1));
        assert(commands==1 && command.kind==STREAMOO_CONTROL_VOLUME_DELTA && command.delta==1);
        assert(d.volume==37 && !d.running); /* wait for owner snapshot */
        assert(streamoo_encoder_demo_audio_state(&d,38,true));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_LONG,0},0,2));
        assert(command.kind==STREAMOO_CONTROL_TOGGLE && d.running);
        assert(streamoo_encoder_demo_audio_state(&d,38,false));
        accept=false;
        assert(!streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_ROTATE,1},0,3));
        assert(d.volume==38 && !d.running && !d.control_pending);
        assert(!streamoo_encoder_demo_audio_state(&d,101,true));
        assert(d.volume==38 && !d.running);
    } else if (scenario==8) {
        streamoo_encoder_demo_t d; assert(streamoo_encoder_demo_init(&d));
        uint8_t old[8192],frame[8192],expected[8*128]={0};
        assert(streamoo_encoder_demo_draw(&d,old,sizeof old,0));
        assert(streamoo_encoder_demo_set_ip(&d,"10.0.0.42"));
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,0));
        const char *label="IP: 10.0.0.42";size_t n=strlen(label);int start=0;
        for(size_t i=0;i<n;++i) {
            uint8_t glyph[5];assert(streamoo_font5x8(NULL,(uint8_t)label[i],glyph));
            for(unsigned x=0;x<5;++x)for(unsigned y=0;y<8;++y)if(glyph[x]&(1u<<y)) {
                unsigned px=start+i*6+x;expected[y*128+px/2]|=(uint8_t)(6u<<(px%2?0:4));
            }
        }
        const char *level="VOL 50";
        for(size_t i=0;i<strlen(level);++i){uint8_t glyph[5];assert(streamoo_font5x8(NULL,(uint8_t)level[i],glyph));
            for(unsigned x=0;x<5;++x)for(unsigned y=0;y<8;++y)if(glyph[x]&(1u<<y)){
                unsigned px=128+i*6+x;expected[y*128+px/2]|=(uint8_t)(15u<<(px%2?0:4));
            }
        }
        for(unsigned y=1;y<7;++y)for(unsigned x=176;x<256;++x)
            if(y==1||y==6||x==176||x==255||(x>=177&&x<216))expected[y*128+x/2]|=(uint8_t)(15u<<(x%2?0:4));
        if(argc==3){FILE *file=fopen(argv[2],"wb");assert(file);assert(fwrite(frame,1,sizeof frame,file)==sizeof frame);assert(!fclose(file));}
        assert(!memcmp(frame+24*128,expected,sizeof expected));
        assert(!memcmp(old,frame,24*128));assert(!memcmp(old+32*128,frame+32*128,32*128));
        assert(!streamoo_encoder_demo_set_ip(&d,"192.168.0.1234567"));
        assert(!streamoo_encoder_demo_set_ip(&d,"bad<ip>"));assert(!strcmp(d.network_ip,"10.0.0.42"));
        assert(streamoo_encoder_demo_set_ip(&d,NULL));
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,0));assert(!memcmp(old,frame,sizeof old));
    } else if (scenario==9) {
        streamoo_encoder_demo_t d;assert(streamoo_encoder_demo_init(&d));
        uint8_t frame[8192];
        assert(streamoo_encoder_demo_set_catalog(&d,NULL,0,1));
        assert(!d.station_id&&!d.running&&d.imported_catalog);
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,0));
        uint8_t old_name[]="One";
        streamoo_ui_station_t first[]={{91,old_name,3},{92,(const uint8_t *)"Two",3}};
        assert(streamoo_encoder_demo_set_catalog(&d,first,2,2));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,1));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,2));
        assert(d.station_id==91);
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,3));
        assert(d.ui.home.station_name.data==old_name);
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,4));
        streamoo_ui_station_t next[]={{92,(const uint8_t *)"Two",3},{91,(const uint8_t *)"One",3}};
        assert(streamoo_encoder_demo_set_catalog(&d,next,2,3));
        memset(old_name,0,sizeof old_name);
        assert(d.ui.screen==STREAMOO_UI_HOME&&d.station_id==91&&!d.ui.home.station_name.data);
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,5));
        assert(d.ui.home.station_name.data==next[1].name);
        assert(!d.ui.home.visualization_valid);
        assert(streamoo_encoder_demo_set_catalog(&d,next,1,4));
        assert(!d.station_id&&!d.running);
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,6));
    } else if (scenario==11) {
        streamoo_encoder_demo_t d; assert(streamoo_encoder_demo_init(&d));
        const streamoo_ui_station_t list[]={{12,(const uint8_t *)"Live",4}};
        assert(streamoo_encoder_demo_set_catalog(&d,list,1,8));
        d.station_id=12; ++d.generation;
        uint8_t frame[8192];
        streamoo_visualization_model_t levels={.rms={1000,15000},.peak={2000,25000}};
        assert(streamoo_encoder_demo_levels(&d,&levels));
        for(unsigned kind=0;kind<3;++kind){
            d.ui.visualization=kind;
            assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,40*kind));
            assert(d.ui.home.visualization_valid==(kind!=STREAMOO_VIS_SPECTRUM));
            if(kind){assert(d.ui.home.visualization.rms[1]==15000);assert(!d.ui.home.visualization.band_count);}
        }
        levels.band_count=24;levels.bands[5]=24000;
        assert(streamoo_encoder_demo_levels(&d,&levels));
        d.ui.visualization=STREAMOO_VIS_SPECTRUM;
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,150));
        assert(d.ui.home.visualization_valid&&d.ui.home.visualization.band_count==24);
        assert(d.ui.home.visualization.bands[5]==24000);
        levels.rms[0]=40000;
        assert(!streamoo_encoder_demo_levels(&d,&levels));
        assert(d.levels.rms[0]==1000);
        assert(streamoo_encoder_demo_levels(&d,NULL));
        assert(streamoo_encoder_demo_draw(&d,frame,sizeof frame,200));
        assert(!d.ui.home.visualization_valid && !d.ui.home.visualization.rms[1]);
    } else if (scenario==10) {
        streamoo_encoder_demo_t d;assert(streamoo_encoder_demo_init(&d));
        const streamoo_ui_station_t list[]={{12,(const uint8_t *)"Network",7}};
        assert(streamoo_encoder_demo_set_catalog(&d,list,1,8));
        streamoo_encoder_demo_bind_station_owner(&d,select_station,NULL);
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,0));
        accept=false;assert(!streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,1));
        assert(d.ui.screen==STREAMOO_UI_STATIONS&&!d.station_id);
        accept=true;assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,2));
        assert(d.ui.screen==STREAMOO_UI_HOME&&!d.station_id); /* wait for owner snapshot */
    } else {
        streamoo_encoder_demo_t d; assert(streamoo_encoder_demo_init(&d));
        uint8_t frame[8194]; memset(frame,0xa5,sizeof frame);
        for (unsigned i=0;i<1000;++i) {
            uint64_t now=i*40;
            assert(streamoo_encoder_demo_draw(&d,frame+1,8192,now));
            assert(frame[0]==0xa5 && frame[8193]==0xa5);
            streamoo_input_event_t e={STREAMOO_INPUT_ROTATE,i&1?INT_MAX:INT_MIN};
            assert(streamoo_encoder_demo_input(&d,e,0,now));
            assert(d.volume>=0 && d.volume<=100);
            e=(streamoo_input_event_t){STREAMOO_INPUT_PRESSED_ROTATE,1};
            assert(streamoo_encoder_demo_input(&d,e,0,now));
        }
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,40000));
        assert(d.ui.selected==1); /* current synthetic station */
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_ROTATE,1},0,40001));
        assert(d.station_id==1002);
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_LONG,0},0,40002));
        assert(d.station_id==1002); /* cancel */
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,40003));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_ROTATE,1},0,40004));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},0,40005));
        assert(d.station_id==1003 && d.ui.screen==STREAMOO_UI_HOME);
        assert(streamoo_encoder_demo_draw(&d,frame+1,8192,40006));
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_LONG,0},0,40007));
        assert(!d.running);
        assert(streamoo_encoder_demo_draw(&d,frame+1,8192,40008));
        for (unsigned i=0;i<24;++i) assert(d.ui.home.visualization.bands[i]==0);
        assert(streamoo_encoder_demo_input(&d,(streamoo_input_event_t){STREAMOO_INPUT_SHORT,0},1,40009));
        assert(d.running && d.ui.screen==STREAMOO_UI_HOME);
        assert(streamoo_encoder_demo_draw(&d,frame+1,8192,40010));
        if (argc==3) {
            FILE *file=fopen(argv[2],"wb"); assert(file);
            assert(fwrite(frame+1,1,8192,file)==8192); assert(!fclose(file));
        }
    }
    return 0;
}
