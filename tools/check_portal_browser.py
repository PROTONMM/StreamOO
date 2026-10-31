"""Optional Chromium integration check against synthetic local HTTP endpoints."""
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
import base64,json,subprocess,threading
root=Path(__file__).resolve().parents[1]/'components/portal/web'
requests=[];stored=0
script=r'''
(async()=>{try{
 const $=id=>document.getElementById(id),assert=(v,s)=>{if(!v)throw Error(s);};
 const wait=async f=>{for(let i=0;i<200;i++){if(f())return;await new Promise(r=>setTimeout(r,20));}throw Error('timeout');};
 $('ap-key').value='wrong-key';$('load-settings').click();await wait(()=>$('auth-result').textContent.includes('Nieprawidłowe'));
 $('ap-key').value='synthetic-key';$('load-settings').click();await wait(()=>$('auth-result').textContent.includes('odczytane'));
 await wait(()=>$('catalog-status').textContent.includes('0 stacji'));
 let rows=$('wifi-rows');assert(rows.children.length===1,'initial row');
 rows.children[0].querySelector('.wifi-ssid').value='Second';rows.children[0].querySelector('.wifi-password').value='synthetic-pass';
 $('add-network').click();rows.children[1].querySelector('.wifi-ssid').value='First';rows.children[1].querySelector('.wifi-open').checked=true;
 rows.children[1].querySelector('.move-up').click();assert(rows.children[0].querySelector('.wifi-ssid').value==='First','priority');
 $('wifi-form').requestSubmit();await wait(()=>$('wifi-result').textContent.includes('Przyjęto'));
 const data=new DataTransfer();data.items.add(new File(['A\thttp://radio.invalid/live\t0\nB\t\u200bhttp://radio.invalid/live\t0'],'playlist.csv',{type:'text/plain'}));
 $('playlist-file').files=data.files;$('playlist-file').dispatchEvent(new Event('change'));
 assert($('commit-playlist').disabled,'preview required');$('playlist-form').requestSubmit();
 await wait(()=>!$('commit-playlist').disabled);assert($('playlist-result').textContent.includes('2 stacji'),'preview report');
 $('commit-playlist').click();await wait(()=>$('playlist-result').textContent.includes('Zapisano 2'));
 assert($('catalog-status').textContent.includes('2 stacji'),'saved count');
 assert(!localStorage.length&&!sessionStorage.length,'no persistent browser credentials');
 $('playlist-file').dispatchEvent(new Event('change'));assert($('commit-playlist').disabled,'changed file needs preview');
 document.body.dataset.test='PASS';
 }catch(e){document.body.dataset.test='FAIL '+e.message;}})();
'''
class Handler(SimpleHTTPRequestHandler):
 def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(root),**kwargs)
 def log_message(self,*args):pass
 def send(self,body,status=200,kind='application/json'):
  if not isinstance(body,bytes):body=json.dumps(body).encode()
  self.send_response(status);self.send_header('Content-Type',kind);self.end_headers();self.wfile.write(body)
 def allowed(self):
  if self.headers.get('Authorization')!='Basic '+base64.b64encode(b'setup:synthetic-key').decode():self.send({'error':'authorization_required'},401);return False
  return True
 def do_GET(self):
  if self.path=='/':return self.send((root/'index.html').read_bytes().replace(b'</head>',b'<script src="/test.js" defer></script></head>'),kind='text/html; charset=utf-8')
  if self.path=='/test.js':return self.send(script.encode(),kind='application/javascript')
  if self.path=='/api/v1/status':return self.send({'schema_version':1,'network':{'mode':'ap','ready':True,'ssid':'StreamOO-ABC123','clients':1,'ap_ready':True,'connected':False,'configuring':False,'profiles':0,'error':0,'ip':'192.168.4.1'},'audio':{'available':False}})
  if self.path.startswith('/api/'):
   if not self.allowed():return
   return self.send({'profiles':[]} if 'wifi' in self.path else {'rows':stored})
  super().do_GET()
 def do_POST(self):
  global stored
  if not self.allowed():return
  assert self.headers.get('X-StreamOO-Request')=='1'
  data=self.rfile.read(int(self.headers['Content-Length']));requests.append((self.path,data))
  if 'wifi/config' in self.path:return self.send({'accepted':True},202)
  commit=self.path.endswith('/import')
  if commit:stored=2
  return self.send({'rows':2,'normalized':1,'duplicates':1,'committed':commit})
if __name__ == '__main__':
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 result=subprocess.run(['/usr/bin/google-chrome','--headless','--no-sandbox','--disable-gpu','--disable-background-networking','--no-first-run','--disable-sync','--hide-scrollbars','--window-size=390,950','--virtual-time-budget=12000','--screenshot=/tmp/streamoo-config-mobile.png','--dump-dom',f'http://127.0.0.1:{server.server_port}/'],capture_output=True,text=True,timeout=40)
 Path('/tmp/streamoo-config-browser.log').write_text(result.stderr)
 assert result.returncode==0,'browser failed'
 assert 'data-test="PASS"' in result.stdout, result.stdout[-1800:]
 assert len(requests)==3 and stored==2
 profiles=json.loads(requests[0][1])['profiles'];assert [x['ssid'] for x in profiles]==['First','Second'] and profiles[0]['password']==''
 assert requests[1][1]==requests[2][1]
 print('Chromium 390px: auth failure/recovery, Wi-Fi add/reorder/submit, CSV preview/commit/reset: PASS')
 server.shutdown()
