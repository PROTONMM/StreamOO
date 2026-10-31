'use strict';
const text = (id, value) => { document.getElementById(id).textContent = value; };
let connectedIP = '', pendingConnection = false, configGeneration = 0;
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
    document.body.classList.remove('offline');
    text('connection', network.ready ? 'Połączono z urządzeniem.' : 'Trwa uruchamianie sieci.');
    text('ssid', network.ssid || '—'); text('clients', network.clients);
    text('mode', network.connected ? (network.ap_ready ? 'Wi-Fi + AP (kończenie konfiguracji)' : 'Wi-Fi') : 'Punkt dostępu');
    text('device-ip', network.ip || '192.168.4.1');
    if (network.connected && network.ip) {
      connectedIP = network.ip;
      text('sta-result', `Połączono z Wi-Fi. Adres radia: ${connectedIP}. Punkt AP wyłączy się. Połącz telefon z domową siecią.`);
      const link = document.getElementById('sta-link'); link.hidden = false; link.href = `http://${connectedIP}/`;
      link.textContent = `Otwórz http://${connectedIP}/`;
    }
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
  } catch (_) {
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
      previewFile = selected; commit.disabled = false; commit.textContent = `Zatwierdź i zapisz ${report.rows} stacji`;
      text('playlist-result', `Poprawny plik: ${report.rows} stacji. Adresy oczyszczone z niewidocznych znaków: ${report.normalized}. Powtórzone adresy: ${report.duplicates} (zostaną zachowane). Zatwierdzenie zastąpi poprzednią listę.`);
    } catch (error) { text('playlist-result', error.message); }
  };
  commit.onclick = async () => {
    try {
      const selected = file(); if (selected !== previewFile) throw new Error('Najpierw sprawdź wybrany plik.'); commit.disabled = true;
      const report = await api('/api/v1/stations/import', {method: 'POST', headers: {'Content-Type': 'text/plain; charset=utf-8'}, body: selected});
      text('playlist-result', `Zapisano ${report.rows} stacji. Lista pozostanie po ponownym uruchomieniu radia.`);
      text('catalog-status', `Zapisana playlista: ${report.rows} stacji.`); previewFile = null;
    } catch (error) { text('playlist-result', error.message); commit.disabled = !previewFile; }
  };
  document.getElementById('load-settings').onclick = loadSettings;
  add(null);
  loadSettings();
}
function setupTabs() {
  const status = document.getElementById('show-status'), config = document.getElementById('show-config');
  if (!status?.addEventListener) return;
  const show = configuration => {
    document.getElementById('status-panel').hidden = configuration;
    document.getElementById('config-panel').hidden = !configuration;
    status.setAttribute('aria-pressed', String(!configuration));
    config.setAttribute('aria-pressed', String(configuration));
    status.classList.toggle('secondary', configuration);
    config.classList.toggle('secondary', !configuration);
  };
  status.onclick = () => show(false); config.onclick = () => show(true);
  show(true);
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
setupTabs();
if (document.getElementById('wifi-form')?.addEventListener) setupForms();
setupClock();
update();
