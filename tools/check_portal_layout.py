"""Chromium layout checks with synthetic data; no radio or external streams needed."""
from pathlib import Path
import importlib.util, json, subprocess, threading
from http.server import ThreadingHTTPServer
spec=importlib.util.spec_from_file_location('portal_browser',Path(__file__).resolve().with_name('check_portal_browser.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
m.lan=True;m.running=True
names=['Radio Jazz','Klasyka','Muzyka bez przerwy','Radio Świat','Elektronika']
m.catalog=[{'id':f'{i+1:016x}','name':names[i] if i<5 else f'Stacja {i+1:04d} · muzyka i wiadomości','url':f'https://radio.invalid/{i}','ovol':0} for i in range(2048)]
m.selected=m.catalog[0]['id']
class Handler(m.Handler):
 def do_GET(self):
  if self.path=='/api/v1/status':return self.send({'schema_version':1,'network':{'mode':'sta','ready':True,'ssid':'','clients':0,'ap_ready':False,'connected':True,'configuring':False,'profiles':1,'error':0,'ip':'192.168.1.42'},'audio':{'available':True,'source':'network_stream','running':True,'buffering':False,'volume':42,'station_id':m.selected,'codec':'FLAC','sample_rate':48000,'bitrate_kbps':1732,'bitrate_measured':True,'bitrate_variable':True}})
  return super().do_GET()
server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
try:
 for width,height,theme,panel in [(1280,900,'light','status'),(390,844,'dark','status'),(320,740,'light','status'),(844,390,'dark','status'),(390,844,'auto','status'),(390,844,'dark','editor')]:
  m.script=r'''(async()=>{try{
 const $=id=>document.getElementById(id),assert=(v,s)=>{if(!v)throw Error(s);};
 const wait=async f=>{for(let i=0;i<400;i++){if(f())return;await new Promise(r=>setTimeout(r,20));}throw Error('timeout');};
 await wait(()=>$('station-list').children.length===2048&&!$('player-play').disabled);
 $('theme-select').value=THEME;$('theme-select').dispatchEvent(new Event('change'));
 assert(localStorage.getItem('streamoo.theme')===THEME,'stored theme');
 document.documentElement.dataset.theme='wrong';setupTheme();assert(document.documentElement.dataset.theme===THEME,'restored theme');
 if(THEME!=='light')assert(getComputedStyle(document.documentElement).backgroundColor==='rgb(17, 24, 21)','dark palette / system preference');
 assert($('audio-codec').textContent==='FLAC'&&$('audio-bitrate').textContent==='Zmienny'&&$('audio-rate').textContent==='48 kHz','format/VBR');
 const list=$('station-list');assert(list.scrollHeight>list.clientHeight,'scrolling list');
 list.scrollTop=10000;const saved=list.scrollTop,first=list.firstChild;
 await new Promise(r=>setTimeout(r,2200));assert(list.firstChild===first&&list.scrollTop===saved,'poll preserves scroll and nodes');
 $('station-search').value='2048';$('station-search').dispatchEvent(new Event('input'));assert(list.children.length===1&&list.scrollTop===0,'full catalog search');
 $('station-search').value='';$('station-search').dispatchEvent(new Event('input'));assert(list.children.length===2048,'all rows restored');
 for(const tab of ['show-config','show-editor','show-status']){$(tab).click();assert(document.documentElement.scrollWidth<=innerWidth,'no horizontal overflow '+tab);if(tab==='show-config'){for(const section of document.querySelectorAll('#config-panel details'))section.open=true;assert(document.documentElement.scrollWidth<=innerWidth,'expanded settings overflow');}}
 for(const id of ['player-prev','player-play','player-stop','player-next']){assert($(id).getBoundingClientRect().height>=36,'icon target '+id);assert($(id).getAttribute('aria-label')&&!$(id).textContent.trim(),'accessible icon '+id);}
 if(innerWidth<=700&&innerHeight>=700){assert(list.getBoundingClientRect().height/innerHeight>=.66,'station list occupies at least two thirds of viewport');assert(document.documentElement.scrollHeight<=innerHeight+1,'player fits viewport');}
 assert(!$('mode')&&!$('device-ip'),'redundant network card removed');
 if(innerHeight>600)assert($('station-search').getBoundingClientRect().bottom<innerHeight,'search visible without page scroll');
 const descriptor=Object.getOwnPropertyDescriptor(window,'localStorage');Object.defineProperty(window,'localStorage',{configurable:true,get(){throw Error('blocked storage');}});setupTheme();$('theme-select').value=THEME;$('theme-select').dispatchEvent(new Event('change'));Object.defineProperty(window,'localStorage',descriptor);
 assert(document.documentElement.dataset.theme===THEME,'theme works with blocked storage');
 if(PANEL==='editor'){$('show-editor').click();for(const label of document.querySelectorAll('#editor-rows label'))assert(label.querySelector('.sr-only'),'editor labels visually hidden');}
 document.body.dataset.test='PASS';
 }catch(e){document.body.dataset.test='FAIL '+e.message;}})();'''.replace('THEME',json.dumps(theme)).replace('PANEL',json.dumps(panel))
  stem=f'/tmp/streamoo-{width}-{height}-{theme}'+('-editor' if panel=='editor' else '')
  result=subprocess.run(['/usr/bin/google-chrome','--headless','--no-sandbox','--disable-gpu','--disable-background-networking','--no-first-run','--disable-sync',*(['--force-dark-mode'] if theme=='auto' else []),f'--window-size={width},{height}','--virtual-time-budget=15000',f'--screenshot={stem}.png','--dump-dom',f'http://127.0.0.1:{server.server_port}/'],capture_output=True,text=True,timeout=30)
  Path(stem+'.html').write_text(result.stdout);Path(stem+'.log').write_text(result.stderr)
  assert result.returncode==0
  assert 'data-test="PASS"' in result.stdout,result.stdout[result.stdout.find('<body'):result.stdout.find('<body')+160]
  print(f'{width}x{height} {theme} {panel}: PASS',flush=True)
finally:server.shutdown()
