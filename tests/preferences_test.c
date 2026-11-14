#include "streamoo_preferences_policy.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    assert(argc==2);
    int scenario=atoi(argv[1]);
    streamoo_preferences_policy_t p;
    streamoo_preferences_value_t base={50,0}, a={51,0}, b={51,2};
    assert(streamoo_preferences_policy_init(&p,base));
    if (scenario==0) {
        for (unsigned v=0;v<=100;++v) for (unsigned w=0;w<4;++w) {
            uint8_t blob[8]; streamoo_preferences_value_t out;
            streamoo_preferences_value_t in={v,w};
            assert(streamoo_preferences_encode(in,blob,8));
            assert(streamoo_preferences_decode(blob,8,&out));
            assert(streamoo_preferences_equal(in,out));
        }
    } else if (scenario==1) {
        uint8_t blob[8]; assert(streamoo_preferences_encode(base,blob,8));
        streamoo_preferences_value_t out={99,2}, old=out;
        for (unsigned i=0;i<8;++i) {
            uint8_t copy[8]; memcpy(copy,blob,8); copy[i]=255;
            assert(!streamoo_preferences_decode(copy,8,&out));
            assert(streamoo_preferences_equal(out,old));
        }
        assert(!streamoo_preferences_decode(blob,7,&out));
        assert(!streamoo_preferences_decode(NULL,8,&out));
        assert(!streamoo_preferences_decode(blob,8,NULL));
        assert(!streamoo_preferences_encode((streamoo_preferences_value_t){101,0},blob,8));
        assert(!streamoo_preferences_encode(base,blob,7));
    } else if (scenario==2) {
        assert(!streamoo_preferences_due(&p,100000)); /* clean boot no write */
        assert(streamoo_preferences_observe(&p,a,100));
        assert(!streamoo_preferences_due(&p,5099)); assert(streamoo_preferences_due(&p,5100));
        streamoo_preferences_result(&p,true,5101);
        assert(!streamoo_preferences_due(&p,99999)); assert(streamoo_preferences_equal(p.saved,a));
    } else if (scenario==3) {
        streamoo_preferences_observe(&p,a,100);
        streamoo_preferences_observe(&p,b,5000);
        assert(!streamoo_preferences_due(&p,9999)); assert(streamoo_preferences_due(&p,10000));
        streamoo_preferences_observe(&p,base,10001);
        assert(!p.dirty && !streamoo_preferences_due(&p,20000));
    } else if (scenario==4) {
        streamoo_preferences_observe(&p,a,100);
        /* Worker sees a again after producer went a -> b -> a in mailbox. */
        streamoo_preferences_observe(&p,a,5000);
        assert(!streamoo_preferences_due(&p,9999)); assert(streamoo_preferences_due(&p,10000));
    } else if (scenario==5) {
        streamoo_preferences_observe(&p,a,0); assert(streamoo_preferences_due(&p,5000));
        streamoo_preferences_result(&p,false,5001);
        assert(p.dirty && streamoo_preferences_equal(p.saved,base));
        assert(!streamoo_preferences_due(&p,10000)); assert(streamoo_preferences_due(&p,10001));
        streamoo_preferences_result(&p,true,10002); assert(!p.dirty);
    } else if (scenario==6) {
        uint64_t now=1ULL<<40; streamoo_preferences_observe(&p,a,now);
        assert(!streamoo_preferences_due(&p,now-1)); assert(!streamoo_preferences_due(&p,now+4999));
        assert(streamoo_preferences_due(&p,now+5000));
    } else {
        streamoo_preferences_policy_t old=p;
        assert(!streamoo_preferences_observe(&p,(streamoo_preferences_value_t){50,4},100));
        assert(!memcmp(&old,&p,sizeof p));
        assert(!streamoo_preferences_policy_init(NULL,base));
        assert(!streamoo_preferences_due(NULL,0));
        streamoo_preferences_result(NULL,true,0);
    }
    return 0;
}
