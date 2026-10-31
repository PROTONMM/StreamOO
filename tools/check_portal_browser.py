"""Optional Chromium integration check against synthetic local HTTP endpoints."""
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
import json,subprocess,threading
root=Path(__file__).resolve().parents[1]/'components/portal/web'
requests=[];zone="Europe/Warsaw";lan=False;running=False;volume=87;catalog=[];generation=1;selected='0000000000000000';exports=0

def canonical():return ''.join(f'{s["name"]}\t{s["url"]}\t{s["ovol"]}\n' for s in catalog)
def replace_catalog(data):
 global catalog,generation,selected,running
 import hashlib
 rows=[]
 for line in data.decode().splitlines():
  name,url,ovol=line.split('\t');url=url.lstrip('\u200b')
  identity=hashlib.sha256((name+'\0'+url+'\0'+str(sum(s['name']==name and s['url']==url for s in rows))).encode()).hexdigest()[:16]
  rows.append({'id':identity,'name':name,'url':url,'ovol':int(ovol)})
 catalog=rows;generation+=1
 if not any(s['id']==selected for s in rows):selected='0000000000000000';running=False
script=r'''
(async()=>{try{
 const $=id=>document.getElementById(id),assert=(v,s)=>{if(!v)throw Error(s);};window.confirm=()=>true;
 const wait=async f=>{for(let i=0;i<300;i++){if(f())return;await new Promise(r=>setTimeout(r,20));}throw Error('timeout');};
 await wait(()=>$('settings-result').textContent.includes('odczytane')&&!$('save-clock').disabled);
 assert($('panel-navigation').hidden&&$('status-panel').hidden&&$('editor-panel').hidden&&!$('config-panel').hidden,'AP setup only');
 assert($('import-panel').parentElement.id==='config-panel','AP import');
 $('show-status').click();assert($('status-panel').hidden,'AP cannot show player');
 $('timezone').value='Europe/London';$('clock-form').requestSubmit();await wait(()=>$('clock-result').textContent.includes('Zapisano'));
 const rows=$('wifi-rows');rows.children[0].querySelector('.wifi-ssid').value='Synthetic';rows.children[0].querySelector('.wifi-password').value='synthetic-pass';
 $('wifi-form').requestSubmit();await wait(()=>$('wifi-result').textContent.includes('Przyjęto'));
 async function upload(csv,cancelFirst=false){
  const data=new DataTransfer();data.items.add(new File([csv],'playlist.csv',{type:'text/plain'}));
  $('playlist-file').files=data.files;$('playlist-file').dispatchEvent(new Event('change'));$('playlist-form').requestSubmit();
  await wait(()=>!$('commit-playlist').disabled);
  if(cancelFirst){let asked=false;window.confirm=()=>{asked=true;return false;};$('commit-playlist').click();assert(asked&&!$('commit-playlist').disabled,'cancel import preserves draft');window.confirm=()=>true;}
  $('commit-playlist').click();await wait(()=>$('playlist-result').textContent.includes('Zapisano'));
 }
 await upload('A\thttp://radio.invalid/a\t0\nB\thttp://radio.invalid/b\t-5');
 await fetch('/test-lan');await wait(()=>!$('status-panel').hidden&&$('station-list').children.length===2);
 assert(!$('panel-navigation').hidden&&$('config-panel').hidden,'LAN defaults player');
 assert($('import-panel').parentElement.id==='editor-panel','LAN import in editor');
 $('station-list').children[0].click();await wait(()=>$('current-station').textContent==='A');
 $('station-list').children[1].click();await wait(()=>$('current-station').textContent==='B');
 $('player-volume').value='63';$('player-volume').dispatchEvent(new Event('input'));$('player-volume').dispatchEvent(new Event('change'));
 await wait(()=>$('volume').textContent==='63 / 100');$('player-stop').click();await wait(()=>$('playback').textContent==='Zatrzymane');
 $('player-play').click();await wait(()=>$('playback').textContent==='Odtwarzanie');
 $('show-editor').click();assert(!$('editor-panel').hidden,'editor tab');
 $('new-station-name').value='C';$('new-station-url').value='http://radio.invalid/c';$('station-add-form').requestSubmit();
 assert($('editor-rows').children.length===3,'add draft');$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 3'));
 const first=$('editor-rows').children[0].querySelector('input');first.value='A poprawione';first.dispatchEvent(new Event('input'));
 $('editor-save').click();await wait(()=>$('station-list').children[0].textContent==='A poprawione');
 $('editor-rows').children[0].querySelector('button').click();$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 2'));
 const originalClick=HTMLAnchorElement.prototype.click;let downloaded=false;
 HTMLAnchorElement.prototype.click=function(){if(this.download==='playlist.csv')downloaded=true;else originalClick.call(this);};
 $('editor-export').click();await wait(()=>downloaded);HTMLAnchorElement.prototype.click=originalClick;
 $('editor-rows').children[0].querySelector('input').dispatchEvent(new Event('input'));
 await upload('D\thttp://radio.invalid/d\t12',true);await wait(()=>$('station-list').children.length===1&&$('station-list').children[0].textContent==='D');
 $('editor-clear').click();await wait(()=>$('editor-result').textContent.includes('Zapisano 0'));
 $('new-station-name').value='Draft';$('new-station-url').value='http://radio.invalid/draft';$('station-add-form').requestSubmit();
 await fetch('/test-bump');$('editor-save').click();await wait(()=>$('editor-result').textContent.includes('Lista została zmieniona'));
 assert($('editor-rows').children.length===1,'conflict keeps draft');$('editor-reload').click();await wait(()=>$('editor-rows').children.length===0);
 await fetch('/test-ap');await wait(()=>$('panel-navigation').hidden&&$('status-panel').hidden&&$('editor-panel').hidden);
 assert($('import-panel').parentElement.id==='config-panel','return AP setup');
 assert(!localStorage.length&&!sessionStorage.length,'no browser credentials');
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
 assert result.returncode==0,'browser failed'
 assert 'data-test="PASS"' in result.stdout, result.stdout[-1800:]
 assert not catalog and exports==1 and zone=="Europe/London"
 selections=[json.loads(data) for path,data in requests if path.endswith('/select')]
 assert len(selections)==2 and selections[0]!=selections[1]
 print('Chromium390px: AP setup only, LAN player/select/volume/stop/play, editor add/edit/delete/export/import/clear/conflict, timezone and Wi-Fi: PASS')
 server.shutdown()
