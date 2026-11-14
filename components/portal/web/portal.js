'use strict';
const text = (id, value) => { document.getElementById(id).textContent = value; };
let connectedIP = '', pendingConnection = false, configGeneration = 0;
let webPlayerMode=false,applyWebMode=()=>{},libraryModeChanged=()=>{},stationStateChanged=()=>{},catalogUpdated=()=>{},beforeCatalogImport=()=>true;
let playerPending=false, volumeEditing=false, playerAvailable=false;
function refreshPlayerControls() {
  for(const id of ['player-play','player-stop','player-volume'])
    document.getElementById(id).disabled=!playerAvailable||playerPending;
}
async function update() {
  const generation = configGeneration;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 3000);
  try {
    const response = await fetch('/api/v1/status', {cache: 'no-store', signal: controller.signal});
    if (!response.ok) throw new Error('status');
    const state = await response.json();
    if (state.schema_version !== 1 || !state.network || !state.audio) throw new Error('schema');
    if (generation !== configGeneration) return;
    const network = state.network;
    applyWebMode(Boolean(network.connected&&!network.ap_ready));
    const systemPanel=document.getElementById('system-panel');if(systemPanel)systemPanel.hidden=!network.connected||network.ap_ready;
    stationStateChanged(state.audio.station_id||'0000000000000000');
    document.body.classList.remove('offline');
    text('connection', network.ready ? 'Połączono z urządzeniem.' : 'Trwa uruchamianie sieci.');
    text('ssid', network.ssid || '—'); text('clients', network.clients);
    text('mode', network.connected ? (network.ap_ready ? 'Wi-Fi + AP (kończenie konfiguracji)' : 'Wi-Fi') : 'Punkt dostępu');
    text('device-ip', network.ip || '192.168.4.1');
    if (network.connected && network.ip && network.ap_ready) {
      connectedIP = network.ip;
      text('sta-result', `Połączono z Wi-Fi. Adres radia: ${connectedIP}. Punkt AP wyłączy się. Połącz telefon z domową siecią.`);
      const link = document.getElementById('sta-link'); link.hidden = false; link.href = `http://${connectedIP}/`;
      link.textContent = `Otwórz http://${connectedIP}/`;
    }
    if(!network.ap_ready){text('sta-result','');document.getElementById('sta-link').hidden=true;}
    if (network.configuring) { pendingConnection = true; text('wifi-result', 'Sprawdzanie sieci w kolejności listy… Pozostaw urządzenie włączone.'); }
    else if (pendingConnection) {
      pendingConnection = false;
      text('wifi-result', network.error ? 'Nie udało się połączyć lub zapisać. Poprzednia lista pozostaje bez zmian. Sprawdź nazwy i hasła.' :
        network.connected ? 'Połączenie udane. Lista sieci została zapisana.' : 'Lista została zapisana. Tryb AP pozostaje aktywny.');
    }
    document.getElementById('save-wifi').disabled = pendingConnection;
    text('playback', network.ready && network.mode === 'ap' && !state.audio.available ?
      'Audio wyłączone w trybie AP' : !state.audio.available ? 'Odtwarzacz nie jest uruchomiony' :
      !state.audio.running ? 'Zatrzymane' : state.audio.buffering ? 'Buforowanie…' : 'Odtwarzanie');
    text('volume', state.audio.available ? `${state.audio.volume} / 100` : '—');
    playerAvailable=Boolean(state.audio.available&&network.connected&&!network.ap_ready&&state.audio.source==='network_stream');
    if(!volumeEditing&&!playerPending){
      document.getElementById('player-volume').value=state.audio.volume ?? 50;
      text('player-volume-value',state.audio.available ? String(state.audio.volume):'—');
    }
    refreshPlayerControls();
  } catch (_) {
    playerAvailable=false;refreshPlayerControls();
    document.body.classList.add('offline');
    text('connection', connectedIP ? `AP zostało wyłączone. Połącz się z domową siecią i otwórz http://${connectedIP}/` :
      'Brak aktualnego połączenia. Sprawdź sieć StreamOO.');
  } finally { clearTimeout(timeout); setTimeout(update, 2000); }
}
const messages = {
  invalid_profiles: 'Sprawdź nazwy i hasła sieci (SSID do 32 bajtów, hasło 8–63 bajtów; bez duplikatów).',
  connection_test_in_progress: 'Trwa poprzednia próba połączenia. Poczekaj na wynik.',
  playlist_storage_unavailable: 'Pamięć playlisty niedostępna. Sprawdź log urządzenia i tablicę partycji.',
  profiles_unavailable: 'Nie można odczytać zapisanych sieci.',
  catalog_changed: 'Lista została zmieniona. Odczytaj ją ponownie przed zapisem lub wyborem stacji.',
  player_mode_required: 'Ta funkcja jest dostępna po połączeniu radia z Wi-Fi i wyłączeniu AP.',
  invalid_station: 'Niepoprawna stacja.',
  invalid_player_command: 'Niepoprawne polecenie odtwarzacza.',
  player_unavailable: 'Odtwarzacz niedostępny. Połącz Wi-Fi i wybierz stację na radiu.',
  player_busy: 'Kolejka poleceń jest zajęta. Spróbuj ponownie.',
  invalid_timezone: 'Wybierz jedną z dostępnych stref czasowych.',
  clock_save_failed: 'Nie udało się zapisać strefy. Poprzednie ustawienie pozostaje aktywne.',
};
async function api(url, options = {}) {
  const controller = new AbortController(), timeout = setTimeout(() => controller.abort(), 30000);
  try {
    const response = await fetch(url, {...options, cache: 'no-store', signal: controller.signal,
      headers: {'X-StreamOO-Request': '1', ...options.headers}});
    const value = await response.json();
    if (!response.ok) throw new Error(messages[value.error] || (value.error_line ? `Błąd w wierszu ${value.error_line}: ${value.error}. Poprzednia playlista nie została zmieniona.` : 'Operacja nie powiodła się.'));
    return value;
  } catch (error) {
    if (error.name === 'AbortError') throw new Error('Przekroczono czas oczekiwania. Sprawdź połączenie.');
    throw error;
  } finally { clearTimeout(timeout); }
}
function setupForms() {
  const rows = document.getElementById('wifi-rows');
  function refreshRows() {
    [...rows.children].forEach((row, i) => {
      row.querySelector('legend').textContent = `Sieć ${i + 1} — priorytet ${i + 1}`;
      row.querySelector('.move-up').disabled = i === 0;
      row.querySelector('.move-down').disabled = i === rows.children.length - 1;
    });
    document.getElementById('add-network').disabled = rows.children.length >= 8;
  }
  function add(profile) {
    if (rows.children.length >= 8) return;
    const fragment = document.getElementById('wifi-row').content.cloneNode(true), row = fragment.querySelector('fieldset');
    row.previousSSID = profile ? profile.ssid : null;
    row.keepPassword = Boolean(profile && profile.has_password);
    row.querySelector('.wifi-ssid').value = profile ? profile.ssid : '';
    row.querySelector('.wifi-password').placeholder = row.keepPassword ? 'Zapisane — pozostaw puste, aby zachować' : 'Hasło sieci';
    row.querySelector('.wifi-open').checked = Boolean(profile && !profile.has_password);
    row.querySelector('.move-up').onclick = () => { if (row.previousElementSibling) rows.insertBefore(row, row.previousElementSibling); refreshRows(); };
    row.querySelector('.move-down').onclick = () => { if (row.nextElementSibling) rows.insertBefore(row.nextElementSibling, row); refreshRows(); };
    row.querySelector('.remove-network').onclick = () => { row.remove(); refreshRows(); };
    rows.appendChild(fragment); refreshRows();
  }
  document.getElementById('add-network').onclick = () => add(null);
  const loadSettings = async () => {
    try {
      const profiles = await api('/api/v1/wifi/profiles'); rows.replaceChildren(); profiles.profiles.forEach(add);
      if (!profiles.profiles.length) add(null);
      text('settings-result', 'Ustawienia odczytane. Zapisane hasła pozostają ukryte.');
      const catalog = await api('/api/v1/stations/import/status'); text('catalog-status', `Zapisana playlista: ${catalog.rows} stacji.`);
    } catch (error) { text('settings-result', error.message); }
  };
  document.getElementById('wifi-form').onsubmit = async event => {
    event.preventDefault();
    try {
      const profiles = [];
      for (const row of rows.children) {
        const ssid = row.querySelector('.wifi-ssid').value, password = row.querySelector('.wifi-password').value;
        if (!ssid) throw new Error('Uzupełnij nazwę sieci lub usuń pusty wiersz.');
        const profile = {ssid};
        if (row.querySelector('.wifi-open').checked) profile.password = '';
        else if (password) profile.password = password;
        else if (!(row.keepPassword && row.previousSSID === ssid)) throw new Error(`Podaj hasło sieci ${profiles.length + 1} lub zaznacz sieć otwartą.`);
        profiles.push(profile);
      }
      if (!profiles.length && !confirm('Usunąć wszystkie zapisane sieci i pozostać w trybie AP?')) return;
      await api('/api/v1/wifi/config', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({profiles})});
      ++configGeneration; connectedIP = ''; document.getElementById('sta-link').hidden = true;
      pendingConnection = true; document.getElementById('save-wifi').disabled = true;
      text('wifi-result', 'Przyjęto listę. Trwa próba połączenia; poprzednie ustawienia pozostają bezpieczne.');
    } catch (error) { text('wifi-result', error.message); }
  };
  let previewFile = null;
  const commit = document.getElementById('commit-playlist'), input = document.getElementById('playlist-file');
  input.onchange = () => { previewFile = null; commit.disabled = true; text('playlist-result', ''); };
  function file() {
    const selected = input.files[0];
    if (!selected || !selected.size || selected.size > 262144) throw new Error('Wybierz plik playlist.csv do 256 KiB.');
    return selected;
  }
  document.getElementById('playlist-form').onsubmit = async event => {
    event.preventDefault(); commit.disabled = true; previewFile = null;
    try {
      const selected = file(); const report = await api('/api/v1/stations/import/preview', {method: 'POST', headers: {'Content-Type': 'text/plain; charset=utf-8'}, body: selected});
      previewFile = selected; commit.disabled = false; commit.textContent = `Zatwierdź i dodaj ${report.rows} stacji`;
      text('playlist-result', `Poprawny plik: ${report.rows} stacji. Adresy oczyszczone z niewidocznych znaków: ${report.normalized}. Powtórzone adresy: ${report.duplicates} (zostaną zachowane). Zatwierdzenie doda pozycje do obecnej listy.`);
    } catch (error) { text('playlist-result', error.message); }
  };
  commit.onclick = async () => {
    try {
      const selected = file(); if (selected !== previewFile) throw new Error('Najpierw sprawdź wybrany plik.'); if(!beforeCatalogImport())return; commit.disabled = true;
      const report = await api('/api/v1/stations/import', {method: 'POST', headers: {'Content-Type': 'text/plain; charset=utf-8'}, body: selected});
      text('playlist-result', `Dodano stacje. Łącznie na liście: ${report.rows}. Lista pozostanie po ponownym uruchomieniu radia.`);
      text('catalog-status', `Zapisana playlista: ${report.rows} stacji.`); previewFile = null;catalogUpdated();
    } catch (error) { text('playlist-result', error.message); commit.disabled = !previewFile; }
  };
  document.getElementById('load-settings').onclick = loadSettings;
  add(null);
  loadSettings();
}
function setupTabs() {
  const status=document.getElementById('show-status'),editor=document.getElementById('show-editor'),config=document.getElementById('show-config');
  if(!status?.addEventListener)return;
  function show(panel){
    if(!webPlayerMode)panel='config';
    for(const [name,button] of [['status',status],['editor',editor],['config',config]]){
      document.getElementById(name+'-panel').hidden=name!==panel;
      button.setAttribute('aria-pressed',String(name===panel));button.classList.toggle('secondary',name!==panel);
    }
  }
  status.onclick=()=>show('status');editor.onclick=()=>show('editor');config.onclick=()=>show('config');
  let initialized=false;
  applyWebMode=enabled=>{
    if(initialized&&enabled===webPlayerMode)return;
    initialized=true;webPlayerMode=enabled;
    document.getElementById('panel-navigation').hidden=!enabled;
    document.getElementById(enabled?'editor-panel':'config-panel').appendChild(document.getElementById('import-panel'));
    text('page-title',enabled?'Twoje radio.':'Ustaw swoje radio.');
    show(enabled?'status':'config');libraryModeChanged(enabled);
  };
  applyWebMode(false);
}
async function setupClock() {
  const form = document.getElementById('clock-form');
  if (!form?.addEventListener) return;
  const select = document.getElementById('timezone'), save = document.getElementById('save-clock');
  const labels = {'UTC':'UTC', 'Europe/Warsaw':'Polska — Warszawa', 'Europe/London':'Wielka Brytania — Londyn',
    'Europe/Berlin':'Niemcy — Berlin', 'Europe/Paris':'Francja — Paryż', 'Europe/Helsinki':'Finlandia — Helsinki',
    'Europe/Madrid':'Hiszpania — Madryt', 'Europe/Rome':'Włochy — Rzym'};
  try {
    const state = await api('/api/v1/clock');
    select.replaceChildren(...state.zones.map(zone => {
      const option = document.createElement('option'); option.value = zone;
      option.textContent = labels[zone] || zone; return option;
    }));
    select.value = state.timezone; select.disabled = save.disabled = false;
    text('clock-result', 'Ustawienie odczytane. Po synchronizacji czas pojawi się na ekranie radia.');
  } catch (error) { text('clock-result', error.message); }
  form.onsubmit = async event => {
    event.preventDefault(); select.disabled = save.disabled = true;
    try {
      await api('/api/v1/clock', {method:'POST', headers:{'Content-Type':'application/json'},
        body:JSON.stringify({timezone:select.value})});
      text('clock-result', 'Zapisano strefę czasową. Zmiana działa od razu i pozostanie po restarcie.');
    } catch (error) { text('clock-result', error.message); }
    finally { select.disabled = save.disabled = false; }
  };
}
function setupPlayer() {
  const play=document.getElementById('player-play'), stop=document.getElementById('player-stop');
  const slider=document.getElementById('player-volume');
  if(!play?.addEventListener)return;
  async function send(command){
    if(playerPending)return;
    if(!playerAvailable){volumeEditing=false;return;}
    playerPending=true;++configGeneration;refreshPlayerControls();
    try{
      await api('/api/v1/player/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(command)});
      text('player-result','Polecenie przyjęte. Stan odświeży się automatycznie.');
    }catch(error){text('player-result',error.message);}
    finally{playerPending=false;volumeEditing=false;refreshPlayerControls();}
  }
  play.onclick=()=>send({action:'play'});stop.onclick=()=>send({action:'stop'});
  slider.oninput=()=>{volumeEditing=true;text('player-volume-value',slider.value);};
  slider.onchange=()=>send({action:'volume',value:Number(slider.value)});
}
function setupLibrary() {
  if(!document.getElementById('station-add-form')?.addEventListener)return;
  let catalog=[],draft=[],generation='',loaded=false,dirty=false,writing=false,loadSerial=0,editRevision=0;
  let editorPage=0,selected='0000000000000000';
  const copy=rows=>rows.map(row=>({...row}));
  const button=(label,click)=>{const b=document.createElement('button');b.type='button';b.className='secondary';b.textContent=label;b.onclick=click;return b;};
  const message=(id,error)=>text(id,error.message||String(error));
  function editState(){
    document.getElementById('editor-save').disabled=!loaded||!dirty||writing;
    for(const id of ['editor-clear','editor-export','editor-reload','stations-refresh'])document.getElementById(id).disabled=!loaded||writing;
    for(const field of document.querySelectorAll('#editor-rows input,#editor-rows button,#station-add-form input,#station-add-form button'))field.disabled=!loaded||writing;
  }
  function updateStationSelection(){
    for(const button of document.getElementById('station-list').children)
      button.setAttribute('aria-pressed',String(button.getAttribute('data-station-id')===selected));
    text('current-station',catalog.find(row=>row.id===selected)?.name||'Wybierz stację z listy.');
  }
  function renderStations(resetScroll=false){
    const filter=document.getElementById('station-search').value.toLocaleLowerCase();
    const rows=catalog.filter(row=>row.name.toLocaleLowerCase().includes(filter));
    const container=document.getElementById('station-list'),scrollTop=container.scrollTop;
    const buttons=[];
    for(const row of rows){
      const b=button(row.name,async()=>{
        if(!webPlayerMode||!playerAvailable){text('station-result','Brak połączenia z odtwarzaczem.');return;}
        b.disabled=true;
        try{
          await api('/api/v1/stations/select',{method:'POST',headers:{'Content-Type':'application/json','X-StreamOO-Catalog-Generation':generation},body:JSON.stringify({station_id:row.id})});
          text('station-result','Wybrano stację. Trwa łączenie…');
        }catch(error){message('station-result',error);}
        finally{b.disabled=false;}
      });
      b.setAttribute('data-station-id',row.id);
      b.setAttribute('aria-pressed',String(row.id===selected));buttons.push(b);
    }
    container.replaceChildren(...buttons);
    container.scrollTop=resetScroll ? 0:scrollTop;
    text('station-count',filter ? `${rows.length} / ${catalog.length} stacji`:`${catalog.length} stacji`);
    updateStationSelection();
    const emptyNotice='Lista jest pusta. Dodaj stacje lub wgraj plik w zakładce Playlista.';
    if(!catalog.length)text('station-result',emptyNotice);
    else if(document.getElementById('station-result').textContent===emptyNotice)text('station-result','');
  }
  function renderEditor(){
    const pages=Math.max(1,Math.ceil(draft.length/25));editorPage=Math.min(editorPage,pages-1);
    const container=document.getElementById('editor-rows');container.replaceChildren();
    draft.slice(editorPage*25,(editorPage+1)*25).forEach((row,position)=>{
      const index=editorPage*25+position,box=document.createElement('div');box.className='editor-row';
      for(const [key,label,max] of [['name','Nazwa stacji',128],['url','Adres strumienia',2048]]){
        const wrap=document.createElement('label'),input=document.createElement('input');wrap.textContent=label;
        input.value=row[key];input.maxLength=max;input.type=key==='url'?'url':'text';
        input.oninput=()=>{draft[index][key]=input.value;dirty=true;++editRevision;editState();};wrap.appendChild(input);box.appendChild(wrap);
      }
      box.appendChild(button('Usuń stację',()=>{draft.splice(index,1);dirty=true;++editRevision;renderEditor();}));container.appendChild(box);
    });
    text('editor-page',`${draft.length ? editorPage+1:0} / ${draft.length ? pages:0} · ${draft.length} stacji`);
    document.getElementById('editor-prev').disabled=!editorPage;
    document.getElementById('editor-next').disabled=editorPage>=pages-1;editState();
  }
  async function load(){
    if(!webPlayerMode)return;
    const serial=++loadSerial,revision=editRevision;
    try{
      const result=await api('/api/v1/stations');
      if(serial!==loadSerial||!webPlayerMode)return;
      if(revision!==editRevision){text('editor-result','Zachowano nowe, niezapisane zmiany. Odczytaj listę ponownie, aby je odrzucić.');return;}
      if(!Array.isArray(result.stations)||typeof result.generation!=='string')throw Error('Niepoprawna lista stacji.');
      catalog=result.stations;generation=result.generation;draft=copy(catalog);loaded=true;dirty=false;
      renderStations();renderEditor();text('editor-result','Lista odczytana.');
    }catch(error){if(serial===loadSerial)message('editor-result',error);}
  }
  function csv(rows){
    if(rows.length>2048)throw Error('Lista może mieć najwyżej 2048 stacji.');
    const data=rows.map((row,index)=>{
      const name=row.name,url=row.url,bytes=value=>new TextEncoder().encode(value).length;
      if(!name||bytes(name)>128||/[\p{Cc}\p{Cf}]/u.test(name))throw Error(`Sprawdź nazwę w wierszu ${index+1}.`);
      let parsed;try{parsed=new URL(url);}catch(_){throw Error(`Sprawdź adres w wierszu ${index+1}.`);}
      if(!['http:','https:'].includes(parsed.protocol)||parsed.username||parsed.password||parsed.hash||/[\s\p{Cc}\p{Cf}]/u.test(url)||bytes(url)>2048)
        throw Error(`Sprawdź adres HTTP/HTTPS w wierszu ${index+1}.`);
      return `${row.id||'0000000000000000'}\t${name}\t${url}\t${row.ovol ?? 0}\n`;
    }).join('');
    if(new TextEncoder().encode(data).length>296960)throw Error('Lista przekracza 256 KiB.');return data;
  }
  async function save(rows,confirmed=false){
    if(!loaded||writing||!webPlayerMode)return;
    if(!rows.length&&!confirmed&&!confirm('Usunąć całą zapisaną listę stacji?'))return;
    writing=true;++loadSerial;editState();
    try{
      const data=csv(rows);
      const result=await api('/api/v1/stations/replace',{method:'POST',headers:{'Content-Type':'application/vnd.streamoo.stations+tsv','X-StreamOO-Catalog-Generation':generation},body:data});
      dirty=false;await load();text('editor-result',`Zapisano ${result.rows} stacji.`);
    }catch(error){message('editor-result',error);}
    finally{writing=false;editState();}
  }
  document.getElementById('station-add-form').onsubmit=event=>{
    event.preventDefault();if(!loaded||writing)return;
    const row={name:document.getElementById('new-station-name').value,url:document.getElementById('new-station-url').value,ovol:0};
    try{csv([row]);if(draft.length>=2048)throw Error('Osiągnięto limit stacji.');draft.push(row);dirty=true;++editRevision;editorPage=Math.floor((draft.length-1)/25);renderEditor();event.target.reset();text('editor-result','Dodano do edycji. Kliknij Zapisz zmiany.');}
    catch(error){message('editor-result',error);}
  };
  document.getElementById('editor-save').onclick=()=>save(draft);
  document.getElementById('editor-clear').onclick=()=>{if(confirm('Usunąć całą listę z urządzenia? Odtwarzanie zostanie zatrzymane.'))save([],true);};
  const reload=()=>{if(!dirty||confirm('Odrzucić niezapisane zmiany i odczytać listę?'))load();};
  document.getElementById('editor-reload').onclick=reload;document.getElementById('stations-refresh').onclick=reload;
  document.getElementById('station-search').oninput=()=>renderStations(true);
  document.getElementById('editor-prev').onclick=()=>{--editorPage;renderEditor();};
  document.getElementById('editor-next').onclick=()=>{++editorPage;renderEditor();};
  document.getElementById('editor-export').onclick=async()=>{
    if(!loaded||!webPlayerMode)return;
    if(dirty&&!confirm('Eksport obejmie zapisaną listę, bez niezapisanych zmian. Kontynuować?'))return;
    const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),30000);
    try{
      const response=await fetch('/api/v1/stations/export',{cache:'no-store',signal:controller.signal});
      if(!response.ok)throw Error('Nie udało się wyeksportować listy.');
      const link=document.createElement('a'),url=URL.createObjectURL(await response.blob());
      link.href=url;link.download='playlist.csv';document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);
    }catch(error){message('editor-result',error);}finally{clearTimeout(timeout);}
  };
  stationStateChanged=id=>{if(selected!==id){selected=id;updateStationSelection();}};
  libraryModeChanged=enabled=>{
    ++loadSerial;
    if(enabled){if(!dirty)load();else{renderStations();renderEditor();text('editor-result','Niezapisane zmiany zachowane. Przed zapisem sprawdzimy wersję listy.');}}
    else{document.getElementById('station-list').replaceChildren();document.getElementById('editor-rows').replaceChildren();}
  };
  beforeCatalogImport=()=>!dirty||confirm('Import doda stacje do zapisanej listy. Niezapisane zmiany edytora zostaną odrzucone. Kontynuować?');
  catalogUpdated=()=>{dirty=false;if(webPlayerMode)load();};
  editState();
}

setupPlayer();
setupTabs();
if (document.getElementById('wifi-form')?.addEventListener) setupForms();
setupClock();
setupLibrary();
update();

async function setupSystemSettings(){
  const form=document.getElementById('system-form');if(!form?.addEventListener)return;
  const timeout=document.getElementById('list-timeout'),action=document.getElementById('list-action'),
    boot=document.getElementById('boot-action'),hostname=document.getElementById('hostname');
  const save=form.querySelector('button');save.disabled=true;
  try{const s=await api('/api/v1/settings');timeout.value=s.list_timeout_s;
    action.value=s.list_autoselect?'select':'cancel';boot.value=s.autoplay?'play':'stop';hostname.value=s.hostname;save.disabled=false;
  }catch(e){text('system-result',e.message);}
  form.addEventListener('submit',async e=>{e.preventDefault();save.disabled=true;
    try{await api('/api/v1/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
      list_timeout_s:Number(timeout.value),list_autoselect:action.value==='select',autoplay:boot.value==='play',hostname:hostname.value.trim()})});
      text('system-result','Zapisano. Zachowanie listy i nazwa działają od razu; tryb startu po ponownym włączeniu.');
    }catch(error){text('system-result',error.message);}finally{save.disabled=false;}
  });
}
setupSystemSettings();
