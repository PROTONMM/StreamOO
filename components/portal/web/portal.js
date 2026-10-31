'use strict';
const text = (id, value) => { document.getElementById(id).textContent = value; };
async function update() {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 3000);
  try {
    const response = await fetch('/api/v1/status', {cache: 'no-store', signal: controller.signal});
    if (!response.ok) throw new Error('status');
    const state = await response.json();
    if (state.schema_version !== 1 || !state.network || !state.audio) throw new Error('schema');
    document.body.classList.remove('offline');
    text('connection', state.network.ready ? 'Połączono z urządzeniem.' : 'Punkt dostępu nie jest gotowy.');
    text('ssid', state.network.ssid || '—');
    text('clients', state.network.clients);
    text('playback', state.network.ready && state.network.mode === 'ap' && !state.audio.available ?
      'Audio wyłączone w trybie AP' : !state.audio.available ? 'Dźwięk niedostępny' :
      !state.audio.running ? 'Zatrzymane' : state.audio.buffering ? 'Buforowanie…' : 'Odtwarzanie');
    text('volume', state.audio.available ? `${state.audio.volume} / 100` : '—');
  } catch (_) {
    document.body.classList.add('offline');
    text('connection', 'Brak aktualnego połączenia. Sprawdź sieć StreamOO.');
  } finally {
    clearTimeout(timeout);
    setTimeout(update, 2000);
  }
}
update();
