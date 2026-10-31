from pathlib import Path
import subprocess
import unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const mode=process.argv[1],elements=new Map(),classes=new Set();let scheduled=0;
const state={schema_version:1,network:{mode:'ap',ready:true,ssid:'StreamOO-123ABC',clients:1},
 audio:{available:true,running:true,buffering:false,volume:87}};
if(mode==='stopped')state.audio.running=false;
if(mode==='unavailable')state.audio.available=false;
const context={AbortController,document:{body:{classList:{add:x=>classes.add(x),remove:x=>classes.delete(x)}},
 getElementById:id=>{if(!elements.has(id))elements.set(id,{textContent:''});return elements.get(id)}},
 setTimeout:()=>{scheduled++;return scheduled},clearTimeout:()=>{},
 fetch:async(url,options)=>{assert.equal(url,'/api/v1/status');assert.equal(options.cache,'no-store');
 if(mode==='offline')throw new Error('offline');return {ok:true,json:async()=>state}}};
vm.runInNewContext(fs.readFileSync('components/portal/web/portal.js','utf8'),context);
setImmediate(()=>{
 assert(scheduled>=2); // abort timeout plus next nonoverlapping poll
 if(mode==='offline'){assert(classes.has('offline'));assert(elements.get('connection').textContent.includes('Brak'));}
 else {assert.equal(elements.get('ssid').textContent,state.network.ssid);assert(!classes.has('offline'));
 if(mode==='stopped')assert.equal(elements.get('playback').textContent,'Zatrzymane');
 else if(mode==='unavailable'){assert.equal(elements.get('volume').textContent,'—');assert.equal(elements.get('playback').textContent,'Audio wyłączone w trybie AP');}
 else {assert.equal(elements.get('volume').textContent,'87 / 100');assert.equal(elements.get('playback').textContent,'Odtwarzanie');}}
});
'''
class PortalFrontendTests(unittest.TestCase):
    def run_case(self,mode):subprocess.run(['node','-e',HARNESS,mode],cwd=ROOT,check=True,timeout=5)
    def test_live_values(self):self.run_case('live')
    def test_stopped(self):self.run_case('stopped')
    def test_audio_unavailable(self):self.run_case('unavailable')
    def test_connection_failure(self):self.run_case('offline')
