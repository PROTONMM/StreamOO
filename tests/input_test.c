#include "streamoo_input.h"
#include <assert.h>
static streamoo_input_t s;
static unsigned long long t;
static streamoo_input_event_t sample(int ab, bool pressed, unsigned dt)
{ t += dt; return streamoo_input_sample(&s, ab & 2, ab & 1, pressed, t); }
static void init(bool half, bool reverse, bool held)
{ t=0; streamoo_input_init(&s,1,1,held,half,reverse,0); }
int main(void)
{
    init(false,false,false);
    assert(sample(1,0,1).kind==0); assert(sample(0,0,1).kind==0);
    assert(sample(2,0,1).kind==0);
    streamoo_input_event_t e=sample(3,0,1);
    assert(e.kind==STREAMOO_INPUT_ROTATE && e.delta==1);
    sample(2,0,1); sample(0,0,1); sample(1,0,1);
    assert(sample(3,0,1).delta==-1);
    init(true,true,false); sample(1,0,1);
    assert(sample(0,0,1).delta==-1); sample(2,0,1);
    assert(sample(3,0,1).delta==-1);
    init(false,false,false); sample(1,0,1); sample(3,0,1);
    assert(sample(3,0,1).kind==0); /* bounce cancels */
    sample(0,0,1); assert(s.quarter_steps==0); /* invalid jump */
    init(false,false,false);
    sample(3,1,1); sample(3,0,5); sample(3,1,5);
    assert(sample(3,1,19).kind==0); sample(3,1,1);
    sample(3,0,100); assert(sample(3,0,19).kind==0);
    assert(sample(3,0,1).kind==STREAMOO_INPUT_SHORT);
    assert(sample(3,0,1).kind==0);
    sample(3,1,1); sample(3,1,20); sample(3,0,580);
    assert(sample(3,0,20).kind==STREAMOO_INPUT_LONG);
    init(true,false,false);
    sample(3,1,1); sample(1,1,1);
    assert(sample(0,1,1).kind==STREAMOO_INPUT_PRESSED_ROTATE);
    sample(0,0,1); sample(0,1,1); /* raw bounce must preserve suppression */
    sample(0,1,20); sample(0,0,20);
    assert(sample(0,0,20).kind==0); /* no click after held turn */
    init(false,false,true); sample(3,0,1);
    assert(sample(3,0,20).kind==0); /* boot held */
    sample(3,1,1); sample(3,1,20); sample(3,0,10);
    assert(sample(3,0,20).kind==STREAMOO_INPUT_SHORT);
    /* Long uptime above 32-bit milliseconds. */
    init(false,false,false); t=1ULL<<40;
    sample(3,1,1); sample(3,1,20); sample(3,0,600);
    assert(sample(3,0,20).kind==STREAMOO_INPUT_LONG);
    return 0;
}
