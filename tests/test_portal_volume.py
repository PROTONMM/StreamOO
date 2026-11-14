"""Exercise the real volume scheduler with delayed HTTP and a monotonic fake clock."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const elements=new Map(),timers=new Map(),calls=[];
let now=0,serial=0,active=0;
function element(id){if(!elements.has(id))elements.set(id,{value:'50',textContent:'',disabled:false,events:{},addEventListener(name,fn){this.events[name]=fn;}});return elements.get(id);}
const context=vm.createContext({document:{getElementById:element},performance:{now:()=>now},
 setTimeout(fn,delay){const id=++serial;timers.set(id,{fn,at:now+delay});return id;},clearTimeout:id=>timers.delete(id),
 request(url,options){assert.equal(url,'/api/v1/player/control');assert.equal(options.method,'POST');assert.equal(++active,1,'only one HTTP request');
  const call={command:JSON.parse(options.body),at:now};calls.push(call);
  return new Promise((resolve,reject)=>{call.finish=error=>{--active;error?reject(Error('network failed')):resolve({accepted:true});};});}
});
vm.runInContext(fs.readFileSync('components/portal/web/portal.js','utf8').split('\nsetupTheme();')[0],context);
vm.runInContext('api=request;playerAvailable=true;setupPlayer();refreshPlayerControls();',context);
const slider=element('player-volume'),state=code=>vm.runInContext(code,context);
const input=value=>{slider.value=String(value);slider.oninput();};
const settle=async()=>{for(let i=0;i<8;i++)await Promise.resolve();};
async function finish(index,error=false){calls[index].finish(error);await settle();}
async function tick(ms){const end=now+ms;for(;;){const next=[...timers].filter(([,t])=>t.at<=end).sort((a,b)=>a[1].at-b[1].at)[0];if(!next)break;now=next[1].at;timers.delete(next[0]);next[1].fn();await settle();}now=end;await settle();}
(async()=>{
 const mode=process.argv[1];
 if(mode==='throttle'){
  input(61);assert.equal(calls.length,1,'input sends before change');assert.equal(calls[0].command.value,61);
  assert(!slider.disabled,'slider remains enabled');assert(element('player-play').disabled);
  input(62);input(63);slider.onchange();assert.equal(calls.length,1);
  await finish(0);assert(state('volumePending&&volumeEditing'));assert.equal(slider.value,'63');
  await tick(99);assert.equal(calls.length,1);await tick(1);assert.equal(calls.length,2);
  assert.equal(calls[1].command.value,63);assert.equal(calls[1].at-calls[0].at,100);
  await finish(1);assert(state('!volumePending&&!volumeEditing'));assert(!element('player-play').disabled);
 } else if(mode==='slow'){
  input(1);for(let i=2;i<=100;i++)input(i);input(0);slider.onchange();
  await tick(600);assert.equal(calls.length,1);assert(!slider.disabled);
  await finish(0);assert.equal(calls.length,2);assert.equal(calls[1].command.value,0);
  await finish(1);await tick(1000);assert.equal(calls.length,2);assert(state('!volumeEditing&&!volumePending'));
 } else if(mode==='release'){
  input(70);await finish(0);assert(state('volumeEditing'),'poll cannot overwrite a held slider');
  await tick(100);slider.onchange();await settle();assert.equal(calls.length,1,'release does not duplicate accepted value');
  assert(state('!volumeEditing&&!volumePending'));
  input(80);await finish(1);await tick(100);slider.events.blur();await settle();assert(state('!volumeEditing'));
  input(90);slider.events.pointercancel();await finish(2);assert(state('!volumeEditing&&!volumePending'));
 } else if(mode==='failure'){
  input(50);input(80);await finish(0,true);await tick(500);assert.equal(calls.length,1);
  assert(element('player-result').textContent.includes('network failed'));assert(state('!volumeEditing&&!volumePending'));
  input(55);input(75);state('playerAvailable=false;refreshPlayerControls();');assert(slider.disabled);
  await finish(1);await tick(500);state('playerAvailable=true;refreshPlayerControls();');await tick(500);
  assert.equal(calls.length,2,'offline queue never replays after reconnect');
  input(25);slider.onchange();await finish(2);assert.equal(calls[2].command.value,25);
 }
 assert.equal(active,0);assert.equal(timers.size,0);
})().catch(error=>{console.error(error);process.exitCode=1;});
'''


class PortalVolumeTests(unittest.TestCase):
    def run_case(self, mode):
        subprocess.run(['node', '-e', HARNESS, mode], cwd=ROOT, check=True, timeout=5)

    def test_input_immediate_throttle_and_latest_final_value(self): self.run_case('throttle')
    def test_slow_http_keeps_only_latest_value_including_mute(self): self.run_case('slow')
    def test_release_blur_cancel_and_no_duplicate(self): self.run_case('release')
    def test_error_and_offline_drop_pending_values(self): self.run_case('failure')
