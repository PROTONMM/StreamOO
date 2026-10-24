#include "encoder_demo.h"
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
