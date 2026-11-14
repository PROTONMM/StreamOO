"""Optional Chromium integration check against synthetic local HTTP endpoints."""
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
import json,subprocess,threading
root=Path(__file__).resolve().parents[1]/'components/portal/web'
system_settings={'list_timeout_s':5,'list_autoselect':True,'autoplay':False,'hostname':''}
requests=[];zone="Europe/Warsaw";lan=False;running=False;volume=87;catalog=[];generation=1;selected='0000000000000000';exports=0

def canonical():return ''.join(f'{s["name"]}\t{s["url"]}\t{s["ovol"]}\n' for s in catalog)
def replace_catalog(data):
 global catalog,generation,selected,running
 import hashlib
 rows=[]
 for line in data.decode().splitlines():
  fields=line.split('\t');existing=fields.pop(0) if len(fields)==4 else None
  name,url,ovol=fields;url=url.lstrip('\u200b')
  identity=hashlib.sha256((name+'\0'+url+'\0'+str(sum(s['name']==name and s['url']==url for s in rows))).encode()).hexdigest()[:16]
  rows.append({'id':existing if existing and existing!='0000000000000000' else identity,'name':name,'url':url,'ovol':int(ovol)})
 catalog=rows;generation+=1
 if not any(s['id']==selected for s in rows):selected='0000000000000000';running=False
script=r'''
(async()=>{try{
 const $=id=>document.getElementById(id),assert=(v,s)=>{if(!v)throw Error(s);};window.confirm=()=>true;
 const wait=async f=>{for(let i=0;i<300;i++){if(f())return;await new Promise(r=>setTimeout(r,20));}throw Error('timeout');};
 await wait(()=>$('settings-result').textContent.includes('odczytane')&&!$('save-clock').disabled);
 assert($('panel-navigation').hidden&&$('status-panel').hidden&&$('editor-panel').hidden&&!$('config-panel').hidden,'AP setup only');
 assert($('import-panel').parentElement.id==='config-panel','AP import');
 assert($('player-prev').disabled&&$('player-next').disabled,'AP navigation disabled');
 $('show-status').click();assert($('status-panel').hidden,'AP cannot show player');
 $('timezone').value='Europe/London';$('clock-form').requestSubmit();await wait(()=>$('clock-result').textContent.includes('Zapisano'));
 const rows=$('wifi-rows');rows.children[0].querySelector('.wifi-ssid').value='Synthetic';rows.children[0].querySelector('.wifi-password').value='synthetic-pass';
 $('wifi-form').requestSubmit();await wait(()=>$('wifi-result').textContent.includes('Przyjęto'));
 async function upload(csv,cancelFirst=false){
  $('import-panel').open=true;
  const data=new DataTransfer();data.items.add(new File([csv],'playlist.csv',{type:'text/plain'}));
  $('playlist-file').files=data.files;$('playlist-file').dispatchEvent(new Event('change'));$('playlist-form').requestSubmit();
  await wait(()=>!$('commit-playlist').disabled);
  if(cancelFirst){let asked=false;window.confirm=()=>{asked=true;return false;};$('commit-playlist').click();assert(asked&&!$('commit-playlist').disabled,'cancel import preserves draft');window.confirm=()=>true;}
  $('commit-playlist').click();await wait(()=>$('playlist-result').textContent.includes('Dodano stacje'));
 }
 await upload('A\thttp://radio.invalid/a\t0\nB\thttp://radio.invalid/b\t-5');
 await fetch('/test-lan');await wait(()=>!$('status-panel').hidden&&$('station-list').children.length===2);
 assert(!$('panel-navigation').hidden&&$('config-panel').hidden,'LAN defaults player');
 assert($('import-panel').parentElement.id==='editor-panel','LAN import in editor');
 $('station-list').children[0].click();await wait(()=>$('current-station').textContent==='A');
 $('station-list').children[1].click();await wait(()=>$('current-station').textContent==='B');
 $('station-search').value='B';$('station-search').dispatchEvent(new Event('input'));
 for(const [id,name] of [['player-next','A'],['player-prev','B'],['player-prev','A'],['player-next','B']]){
  $(id).click();assert($(id).disabled,'disable navigation during request');$(id).click();
  await wait(()=>$('current-station').textContent===name&&!$(id).disabled);
  assert($('station-list').children.length===1,'navigation preserves filter');
 }
 $('station-search').value='';$('station-search').dispatchEvent(new Event('input'));

 $('player-volume').value='63';$('player-volume').dispatchEvent(new Event('input'));
 assert(!$('player-volume').disabled,'volume remains draggable during request');
 await wait(()=>!$('player-stop').disabled);
 assert((await (await fetch('/api/v1/status')).json()).audio.volume===63,'volume applied before release');
 $('player-volume').dispatchEvent(new Event('change'));
 await wait(()=>$('volume').textContent==='63'&&!$('player-stop').disabled);$('player-stop').click();await wait(()=>$('playback').textContent==='Zatrzymane');
 $('player-play').click();await wait(()=>$('playback').textContent==='Odtwarzanie');
 $('show-config').click();$('system-panel').open=true;await wait(()=>!$('system-panel').hidden&&!$('save-system').disabled);
 $('list-timeout').value='9';$('list-action').value='cancel';$('boot-action').value='play';$('hostname').value='kitchen-radio';
 $('system-form').requestSubmit();await wait(()=>$('system-result').textContent.includes('Zapisano'));
 const settings=await (await fetch('/api/v1/settings')).json();assert(settings.list_timeout_s===9&&!settings.list_autoselect&&settings.autoplay&&settings.hostname==='kitchen-radio','system settings save');
 $('show-editor').click();assert(!$('editor-panel').hidden,'editor tab');
 document.querySelector('.add-station').open=true;$('new-station-name').value='C';$('new-station-url').value='http://radio.invalid/c';$('station-add-form').requestSubmit();
 assert($('editor-rows').children.length===3,'add draft');$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 3'));
 const first=$('editor-rows').children[0].querySelector('input');first.value='A poprawione';first.dispatchEvent(new Event('input'));
 $('editor-save').click();await wait(()=>$('station-list').children[0].textContent==='A poprawione');
 $('editor-rows').children[0].querySelector('button').click();$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 2'));
 const originalClick=HTMLAnchorElement.prototype.click;let downloaded=false;
 HTMLAnchorElement.prototype.click=function(){if(this.download==='playlist.csv')downloaded=true;else originalClick.call(this);};
 $('editor-export').click();await wait(()=>downloaded);HTMLAnchorElement.prototype.click=originalClick;
 $('editor-rows').children[0].querySelector('input').dispatchEvent(new Event('input'));
 await upload('D\thttp://radio.invalid/d\t12',true);await wait(()=>$('station-list').children.length===1&&$('station-list').children[0].textContent==='D');
 assert($('player-prev').disabled&&$('player-next').disabled,'single station navigation disabled');
 $('editor-clear').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 0'));
 assert($('player-prev').disabled&&$('player-next').disabled,'empty catalog navigation disabled');
 $('new-station-name').value='Draft';$('new-station-url').value='http://radio.invalid/draft';$('station-add-form').requestSubmit();
 await fetch('/test-bump');$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Lista została zmieniona'));
 assert($('editor-rows').children.length===1,'conflict keeps draft');$('editor-reload').click();await wait(()=>$('editor-rows').children.length===0);
 await fetch('/test-ap');await wait(()=>$('panel-navigation').hidden&&$('status-panel').hidden&&$('editor-panel').hidden);
 assert($('import-panel').parentElement.id==='config-panel','return AP setup');
 assert(!sessionStorage.length,'no session credentials');
 for(const theme of ['dark','light','auto']){
  $('theme-select').value=theme;$('theme-select').dispatchEvent(new Event('change'));
  assert(document.documentElement.dataset.theme===theme&&localStorage.getItem('streamoo.theme')===theme,'theme choice persisted');
 }
 assert(localStorage.length===1,'only theme stored');
 assert(document.documentElement.scrollWidth<=window.innerWidth,'mobile overflow');
 document.body.dataset.test='PASS';
 }catch(e){document.body.dataset.test='FAIL '+e.message;}})();
'''
class Handler(SimpleHTTPRequestHandler):
 def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(root),**kwargs)
 def log_message(self,*args):pass
 def send(self,body,status=200,kind='application/json'):
  if not isinstance(body,bytes):body=json.dumps(body).encode()
  self.send_response(status);self.send_header('Content-Type',kind);self.end_headers();self.wfile.write(body)
 def do_GET(self):
  global lan,generation,exports
  if self.path=='/':return self.send((root/'index.html').read_bytes().replace(b'</head>',b'<script src="/test.js" defer></script></head>'),kind='text/html')
  if self.path=='/test.js':return self.send(script.encode(),kind='application/javascript')
  if self.path=='/test-lan':lan=True;return self.send({'ok':True})
  if self.path=='/test-ap':lan=False;return self.send({'ok':True})
  if self.path=='/test-bump':generation+=1;return self.send({'ok':True})
  if self.path=='/api/v1/settings':return self.send(system_settings)
  if self.path=='/api/v1/clock':return self.send({'timezone':zone,'zones':['UTC','Europe/Warsaw','Europe/London']})
  if self.path=='/api/v1/status':return self.send({'schema_version':1,'network':{'mode':'sta' if lan else 'ap','ready':True,'ssid':'StreamOO-ABC123','clients':0,'ap_ready':not lan,'connected':lan,'configuring':False,'profiles':1,'error':0,'ip':'10.0.0.42' if lan else '192.168.4.1'},'audio':{'available':lan,'source':'network_stream','running':running,'buffering':False,'volume':volume,'station_id':selected}})
  if self.path=='/api/v1/stations':
   assert lan,'AP must not fetch station list';return self.send({'generation':str(generation),'stations':catalog})
  if self.path=='/api/v1/stations/export':
   assert lan;exports+=1;return self.send(canonical().encode(),kind='text/plain')
  if self.path.startswith('/api/'):
   assert self.headers.get('Authorization') is None
   return self.send({'profiles':[]} if 'wifi' in self.path else {'rows':len(catalog)})
  super().do_GET()
 def do_POST(self):
  global zone,running,volume,selected
  assert self.headers.get('Authorization') is None
  assert self.headers.get('X-StreamOO-Request')=='1'
  data=self.rfile.read(int(self.headers['Content-Length']));requests.append((self.path,data))
  if self.path=='/api/v1/settings':system_settings.update(json.loads(data));return self.send({'saved':True})
  if self.path=='/api/v1/clock':zone=json.loads(data)['timezone'];return self.send({'saved':True})
  if self.path.endswith('/select'):
   assert lan and self.headers['X-StreamOO-Catalog-Generation']==str(generation)
   selected=json.loads(data)['station_id'];assert any(s['id']==selected for s in catalog);running=True
   return self.send({'accepted':True},202)
  if self.path.endswith('/replace'):
   assert lan
   if self.headers['X-StreamOO-Catalog-Generation']!=str(generation):return self.send({'error':'catalog_changed'},409)
   replace_catalog(data);return self.send({'rows':len(catalog),'committed':True})
  if self.path=='/api/v1/player/control':
   assert lan;command=json.loads(data)
   if command['action']=='volume':volume=command['value']
   else:running=command['action']=='play'
   return self.send({'accepted':True},202)
  if 'wifi/config' in self.path:return self.send({'accepted':True},202)
  commit=self.path.endswith('/import')
  if commit:replace_catalog(data)
  return self.send({'rows':len(data.decode().splitlines()),'normalized':0,'duplicates':0,'committed':commit})
if __name__ == '__main__':
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 result=subprocess.run(['/usr/bin/google-chrome','--headless','--no-sandbox','--disable-gpu','--disable-background-networking','--no-first-run','--disable-sync','--hide-scrollbars','--window-size=390,1600','--virtual-time-budget=40000','--screenshot=/tmp/streamoo-config-mobile.png','--dump-dom',f'http://127.0.0.1:{server.server_port}/'],capture_output=True,text=True,timeout=40)
 Path('/tmp/streamoo-config-browser.log').write_text(result.stderr)
 Path('/tmp/streamoo-config-browser.html').write_text(result.stdout)
 assert result.returncode==0,'browser failed'
 assert 'data-test="PASS"' in result.stdout, result.stdout[result.stdout.find('<body'):result.stdout.find('<body')+180]
 assert not catalog and exports==1 and zone=="Europe/London"
 selections=[json.loads(data) for path,data in requests if path.endswith('/select')]
 assert len(selections)==6 and selections[0]!=selections[1]
 assert selections==[selections[0],selections[1],selections[0],selections[1],selections[0],selections[1]]
 print('Chromium390px: AP setup only, LAN player/select/prev/next/wrap/filter/volume/stop/play, editor add/edit/delete/export/import/clear/conflict, timezone and Wi-Fi: PASS')
 server.shutdown()
