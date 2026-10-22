#pragma once

#include <pgmspace.h>

static const char kIndexHtml[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>PTZ Control</title>
<style>
  :root { color-scheme: dark; --bg:#111; --btn:#2a2a2a; --active:#0a84ff; --fg:#eee; }
  * { box-sizing: border-box; -webkit-user-select: none; user-select: none; -webkit-tap-highlight-color: transparent; }
  body { margin:0; padding:16px; font-family:-apple-system,system-ui,sans-serif; background:var(--bg); color:var(--fg);
         display:flex; flex-direction:column; align-items:center; gap:18px; }
  h1 { font-size:1.2rem; margin:0; }
  #status { font-size:.85rem; min-height:1.2em; opacity:.8; }
  .pad { display:grid; grid-template-columns:repeat(3,80px); grid-template-rows:repeat(3,80px); gap:8px; }
  .row { display:flex; gap:8px; align-items:center; }
  .row span { width:56px; font-size:.85rem; opacity:.8; }
  button { background:var(--btn); color:var(--fg); border:0; border-radius:12px; font-size:1.4rem; min-width:64px; min-height:56px;
           touch-action:none; }
  button.on, button:active { background:var(--active); }
  .pad button { width:80px; height:80px; }
  #stop { font-size:.9rem; font-weight:600; }
  .presets { display:grid; grid-template-columns:repeat(4,64px); gap:8px; }
  .presets button { font-size:1rem; }
  label { font-size:.9rem; display:flex; align-items:center; gap:8px; }
  input[type=range] { width:200px; }
  body.save .presets button { outline:2px solid #ff9f0a; }
</style>
</head>
<body>
<h1>PTZ Control</h1>
<div id="status">Ready</div>

<div class="pad">
  <button data-pan="-1" data-tilt="1">&#8598;</button>
  <button data-tilt="1">&#8593;</button>
  <button data-pan="1" data-tilt="1">&#8599;</button>
  <button data-pan="-1">&#8592;</button>
  <button id="stop">STOP</button>
  <button data-pan="1">&#8594;</button>
  <button data-pan="-1" data-tilt="-1">&#8601;</button>
  <button data-tilt="-1">&#8595;</button>
  <button data-pan="1" data-tilt="-1">&#8600;</button>
</div>

<div class="row"><span>Zoom</span><button data-zoom="-1">&minus;</button><button data-zoom="1">+</button></div>
<div class="row"><span>Focus</span><button data-focus="-1">Near</button><button data-focus="1">Far</button></div>
<div class="row"><span>Iris</span><button data-iris="-1">Close</button><button data-iris="1">Open</button></div>

<label>Speed <input id="speed" type="range" min="1" max="63" value="32"><output id="speedOut">32</output></label>

<div class="presets" id="presets"></div>
<label><input type="checkbox" id="saveMode"> Save current position to preset</label>

<script>
const statusEl = document.getElementById('status');
const speedEl = document.getElementById('speed');
const speedOut = document.getElementById('speedOut');
const saveMode = document.getElementById('saveMode');
let queue = Promise.resolve();
let pending = 0;
let current = null;
let timer = null;

speedEl.oninput = () => { speedOut.textContent = speedEl.value; };
saveMode.onchange = () => document.body.classList.toggle('save', saveMode.checked);

function setStatus(text) { statusEl.textContent = text; }

// Requests are serialised so a stop can never overtake an earlier move.
function post(path, params) {
  pending++;
  queue = queue.then(async () => {
    const ctrl = new AbortController();
    const t = setTimeout(() => ctrl.abort(), 2000);
    try {
      const r = await fetch(path, { method: 'POST', body: new URLSearchParams(params || {}), signal: ctrl.signal });
      setStatus(r.ok ? 'Connected' : 'Error ' + r.status);
    } catch (e) {
      setStatus('Connection lost');
    } finally {
      clearTimeout(t);
      pending--;
    }
  });
  return queue;
}

function start(btn) {
  const d = btn.dataset;
  current = { pan: d.pan || 0, tilt: d.tilt || 0, zoom: d.zoom || 0, focus: d.focus || 0, iris: d.iris || 0, speed: speedEl.value };
  btn.classList.add('on');
  post('/api/move', current);
  clearInterval(timer);
  // Keep-alive: the ESP32 stops the camera if these stop arriving.
  timer = setInterval(() => { if (current && pending === 0) post('/api/move', current); }, 250);
}

function stop() {
  clearInterval(timer);
  timer = null;
  document.querySelectorAll('button.on').forEach(b => b.classList.remove('on'));
  if (current) { current = null; post('/api/stop'); }
}

document.querySelectorAll('button[data-pan],button[data-tilt],button[data-zoom],button[data-focus],button[data-iris]').forEach(btn => {
  btn.addEventListener('pointerdown', e => { e.preventDefault(); btn.setPointerCapture(e.pointerId); start(btn); });
  ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => btn.addEventListener(ev, stop));
  btn.addEventListener('contextmenu', e => e.preventDefault());
});
document.getElementById('stop').onclick = () => { current = current || {}; stop(); };
document.addEventListener('visibilitychange', () => { if (document.hidden) stop(); });
window.addEventListener('blur', stop);

const presets = document.getElementById('presets');
for (let i = 1; i <= 8; i++) {
  const b = document.createElement('button');
  b.textContent = i;
  b.onclick = () => {
    if (saveMode.checked) {
      if (!confirm('Save current position to preset ' + i + '?')) return;
      post('/api/preset', { action: 'set', id: i });
      saveMode.checked = false;
      document.body.classList.remove('save');
    } else {
      post('/api/preset', { action: 'go', id: i });
    }
  };
  presets.appendChild(b);
}
</script>
</body>
</html>
)HTML";
