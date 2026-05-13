#ifndef WEBPAGE_H
#define WEBPAGE_H

/* ================================================================
 * webpage.h
 * The robot control web UI stored in Flash (PROGMEM) to save RAM.
 * Included once by wifi_manager.c — do not include elsewhere.
 *
 * Features:
 *   - Responsive dark dashboard
 *   - Real-time WebSocket sensor bars + canvas visualiser
 *   - D-Pad buttons + WASD / Arrow-key keyboard support
 *   - Touch-friendly (mobile / tablet)
 *   - Mode switch (Manual / Auto) and speed slider
 *   - Obstacle alert banner
 * ================================================================ */

#include <pgmspace.h>

/* TOSTRING helper for integer macros inside C string literals */
#define TOSTRING2(x) #x
#define TOSTRING(x)  TOSTRING2(x)

const char ROBOT_HTML[] PROGMEM = R"HTMLEOF(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>ESP32 Robot</title>
<style>
/* ── CSS VARIABLES ── */
:root{
  --bg:#0a0e1a;--panel:#111827;--panel2:#1a2235;
  --b1:#1e293b;--b2:#2d3f5a;
  --acc:#00e5ff;--acc2:#ff6b35;--acc3:#a855f7;
  --tx:#e2e8f0;--tx2:#94a3b8;--tx3:#475569;
  --ok:#22c55e;--warn:#f59e0b;--err:#ef4444;
  --r:10px;--r2:6px;
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
html,body{height:100%;overflow:hidden}
body{background:var(--bg);color:var(--tx);font-family:'Courier New',monospace;
     font-size:14px;display:flex;flex-direction:column}

/* ── HEADER ── */
header{background:var(--panel);border-bottom:1px solid var(--b2);
       padding:10px 16px;display:flex;align-items:center;
       justify-content:space-between;flex-shrink:0}
.logo{font-size:13px;letter-spacing:3px;color:var(--acc);font-weight:700}
.logo span{color:var(--tx2)}
.hright{display:flex;align-items:center;gap:8px}
.dot{width:8px;height:8px;border-radius:50%;background:var(--err);transition:background .3s}
.dot.on{background:var(--ok)}
#ip{font-size:11px;color:var(--tx3)}

/* ── MAIN GRID ── */
main{flex:1;overflow:auto;padding:12px;display:grid;gap:10px;
     grid-template-columns:1fr 1fr;align-content:start}

/* ── CARD ── */
.card{background:var(--panel);border:1px solid var(--b1);
      border-radius:var(--r);padding:12px}
.ctitle{font-size:10px;letter-spacing:2px;color:var(--tx3);
        text-transform:uppercase;margin-bottom:10px}

/* ── STATUS ── */
.srow{display:flex;justify-content:space-between;align-items:center;
      margin-bottom:7px;font-size:12px}
.badge{font-size:10px;font-weight:700;letter-spacing:1px;
       padding:2px 8px;border-radius:20px;border:1px solid}
.badge.manual{color:var(--acc);border-color:var(--acc);background:#00e5ff14}
.badge.auto  {color:var(--acc2);border-color:var(--acc2);background:#ff6b3514}
.badge.online{color:var(--ok);border-color:var(--ok);background:#22c55e14}
.badge.offline{color:var(--err);border-color:var(--err);background:#ef444414}
.uptime{font-size:10px;color:var(--tx3);margin-top:4px}

/* ── SENSOR BARS ── */
.sr{margin-bottom:9px}
.sl{display:flex;justify-content:space-between;font-size:11px;margin-bottom:3px}
.sv{font-weight:700;transition:color .3s}
.sv.ok{color:var(--ok)}.sv.warn{color:var(--warn)}.sv.err{color:var(--err)}
.track{background:#0d1525;border-radius:3px;height:6px;overflow:hidden}
.fill{height:100%;border-radius:3px;transition:width .35s,background .35s}
.fill.ok{background:var(--ok)}.fill.warn{background:var(--warn)}.fill.err{background:var(--err)}

/* ── MODE + SPEED ── */
#mode-card{grid-column:1/-1;display:flex;flex-wrap:wrap;gap:8px;align-items:center}
.mdbtn{flex:1;min-width:100px;padding:9px;border-radius:var(--r2);cursor:pointer;
       border:1px solid var(--b2);background:var(--panel2);color:var(--tx2);
       font-family:inherit;font-size:11px;letter-spacing:2px;transition:all .2s}
.mdbtn.am{border-color:var(--acc);color:var(--acc);background:#00e5ff0e}
.mdbtn.aa{border-color:var(--acc2);color:var(--acc2);background:#ff6b350e}
.spd-wrap{flex:2;min-width:140px}
.spd-wrap label{font-size:10px;color:var(--tx3);display:block;margin-bottom:5px}
input[type=range]{width:100%;-webkit-appearance:none;height:4px;
                  border-radius:2px;background:var(--b2);outline:none;cursor:pointer}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;
  border-radius:50%;background:var(--acc);cursor:pointer;border:2px solid var(--bg)}

/* ── D-PAD ── */
.dpad{display:grid;
      grid-template-areas:". u ." "l s r" ". d .";
      grid-template-columns:1fr 1fr 1fr;gap:6px;max-width:200px;margin:0 auto}
.dbtn{background:var(--panel2);border:1px solid var(--b2);border-radius:var(--r2);
      color:var(--tx);font-size:18px;padding:14px 8px;cursor:pointer;
      user-select:none;-webkit-user-select:none;transition:all .1s;text-align:center}
.dbtn:active,.dbtn.pr{background:#00e5ff14;border-color:var(--acc);
                       color:var(--acc);transform:scale(.93)}
.dbtn.stop-btn{background:#ef444410;border-color:#ef444440;font-size:10px;color:var(--err)}
.dbtn.stop-btn:active,.dbtn.stop-btn.pr{background:#ef444430}
[data-c=forward] {grid-area:u}[data-c=left]    {grid-area:l}
[data-c=stop]    {grid-area:s}[data-c=right]   {grid-area:r}
[data-c=backward]{grid-area:d}

/* ── CANVAS VISUALISER ── */
#vis-card{display:flex;align-items:center;justify-content:center}
#canvas{width:100%;height:160px}

/* ── LOG ── */
#log-card{grid-column:1/-1}
#logbox{font-size:11px;color:var(--tx2);line-height:1.9;max-height:78px;overflow-y:auto}
.le{color:var(--acc)}.le::before{content:"▸ "}
.le.w{color:var(--warn)}.le.e{color:var(--err)}

/* ── OBSTACLE ALERT ── */
#oa{display:none;grid-column:1/-1;background:#ef444418;border:1px solid var(--err);
    border-radius:var(--r2);padding:8px 12px;text-align:center;
    font-size:12px;color:var(--err);animation:pulse 1s ease-in-out infinite alternate}
#oa.show{display:block}
@keyframes pulse{to{opacity:.4}}

/* ── RESPONSIVE (single column on narrow screens) ── */
@media(max-width:480px){
  main{grid-template-columns:1fr}
  #mode-card,#log-card,#oa{grid-column:1}
}
</style>
</head>
<body>

<!-- HEADER -->
<header>
  <div class="logo">&#11041; ESP32 <span>ROBOT</span></div>
  <div class="hright">
    <div class="dot" id="dot"></div>
    <span id="ip">connecting...</span>
  </div>
</header>

<main>
  <!-- OBSTACLE ALERT BANNER -->
  <div id="oa">&#9888; OBSTACLE DETECTED AHEAD!</div>

  <!-- STATUS CARD -->
  <div class="card">
    <div class="ctitle">Status</div>
    <div class="srow"><span>WebSocket</span><span class="badge offline" id="ws-badge">OFFLINE</span></div>
    <div class="srow"><span>Mode</span><span class="badge manual" id="mode-badge">MANUAL</span></div>
    <div class="srow"><span>Speed</span><span id="spd-disp" style="color:var(--acc);font-weight:700">180</span></div>
    <div class="srow"><span>Last cmd</span><span id="last-cmd" style="color:var(--tx3);font-size:11px">STOP</span></div>
    <div class="uptime" id="upt">Uptime: --</div>
  </div>

  <!-- SENSOR CARD -->
  <div class="card">
    <div class="ctitle">HC-SR04 (cm)</div>
    <div class="sr"><div class="sl"><span>&#9668; LEFT</span><span class="sv ok" id="sv-l">--</span></div>
      <div class="track"><div class="fill ok" id="sf-l" style="width:0%"></div></div></div>
    <div class="sr"><div class="sl"><span>&#9650; CENTER</span><span class="sv ok" id="sv-c">--</span></div>
      <div class="track"><div class="fill ok" id="sf-c" style="width:0%"></div></div></div>
    <div class="sr"><div class="sl"><span>&#9658; RIGHT</span><span class="sv ok" id="sv-r">--</span></div>
      <div class="track"><div class="fill ok" id="sf-r" style="width:0%"></div></div></div>
  </div>

  <!-- MODE + SPEED -->
  <div class="card" id="mode-card">
    <button class="mdbtn am" id="btn-m" onclick="setMode('manual')">MANUAL</button>
    <button class="mdbtn"    id="btn-a" onclick="setMode('auto')">AUTO</button>
    <div class="spd-wrap">
      <label>SPEED &nbsp;<span id="sv-val">180</span>/255</label>
      <input type="range" min="80" max="255" value="180" id="srange" oninput="onSpeed(this.value)">
    </div>
  </div>

  <!-- D-PAD -->
  <div class="card">
    <div class="ctitle">Direction (WASD / Arrows)</div>
    <div class="dpad">
      <div class="dbtn" data-c="forward" >&#9650;</div>
      <div class="dbtn" data-c="left"    >&#9668;</div>
      <div class="dbtn stop-btn" data-c="stop">&#9632;<br>STOP</div>
      <div class="dbtn" data-c="right"   >&#9658;</div>
      <div class="dbtn" data-c="backward">&#9660;</div>
    </div>
  </div>

  <!-- CANVAS VISUALISER -->
  <div class="card" id="vis-card">
    <div class="ctitle">Sensor view</div>
    <canvas id="canvas"></canvas>
  </div>

  <!-- LOG -->
  <div class="card" id="log-card">
    <div class="ctitle">Event log</div>
    <div id="logbox"></div>
  </div>
</main>

<script>
/* ================================================================
 * WebSocket + UI logic
 * ================================================================ */
const MAX_CM = 150;
let ws, mode = 'manual', speed = 180;
let sd = {left:999, center:999, right:999};

/* -- WebSocket connection ---------------------------------------- */
function connect() {
  ws = new WebSocket('ws://' + location.hostname + '/ws');

  ws.onopen = function() {
    setOnline(true);
    document.getElementById('ip').textContent = location.hostname;
    addLog('WebSocket connected');
  };
  ws.onclose = function() {
    setOnline(false);
    addLog('Connection lost — retrying in 2 s...', 'w');
    setTimeout(connect, 2000);
  };
  ws.onmessage = function(e) {
    var d;
    try { d = JSON.parse(e.data); } catch(_) { return; }
    if (d.type === 'status') handleStatus(d);
  };
}

function wsSend(obj) {
  if (ws && ws.readyState === 1) ws.send(JSON.stringify(obj));
}

function setOnline(ok) {
  document.getElementById('dot').className        = ok ? 'dot on' : 'dot';
  document.getElementById('ws-badge').className   = 'badge ' + (ok ? 'online' : 'offline');
  document.getElementById('ws-badge').textContent = ok ? 'ONLINE' : 'OFFLINE';
}

/* -- Status handler --------------------------------------------- */
function handleStatus(d) {
  sd = {left: d.left, center: d.center, right: d.right};

  updateBar('l', d.left);
  updateBar('c', d.center);
  updateBar('r', d.right);

  var mb = document.getElementById('mode-badge');
  mb.textContent = (d.mode || 'manual').toUpperCase();
  mb.className   = 'badge ' + (d.mode || 'manual');

  document.getElementById('last-cmd').textContent  = (d.lastCmd || 'STOP').toUpperCase();
  document.getElementById('spd-disp').textContent  = d.speed || speed;

  if (d.uptime) {
    var s = Math.floor(d.uptime / 1000);
    document.getElementById('upt').textContent =
      'Uptime: ' + Math.floor(s/3600) + 'h ' +
      Math.floor((s % 3600) / 60) + 'm ' + (s % 60) + 's';
  }

  document.getElementById('oa').className = d.obstacle ? 'show' : '';
  drawCanvas();
}

/* -- Sensor bars ------------------------------------------------- */
function barClass(v) { return v < 20 ? 'err' : v < 40 ? 'warn' : 'ok'; }

function updateBar(id, val) {
  var lbl = val >= 999 ? '---' : Math.round(val) + ' cm';
  var pct = val >= 999 ? 0 : Math.min(100, (val / MAX_CM) * 100);
  var cls = barClass(val);
  document.getElementById('sv-' + id).textContent = lbl;
  document.getElementById('sv-' + id).className   = 'sv ' + cls;
  document.getElementById('sf-' + id).style.width = pct + '%';
  document.getElementById('sf-' + id).className   = 'fill ' + cls;
}

/* -- Canvas robot visualiser ------------------------------------ */
function drawCanvas() {
  var c   = document.getElementById('canvas');
  var ctx = c.getContext('2d');
  var W   = c.offsetWidth, H = c.offsetHeight;
  c.width = W; c.height = H;
  ctx.clearRect(0, 0, W, H);

  var cx = W / 2, cy = H * 0.62;
  var rw = 32, rh = 44;

  /* Subtle grid */
  ctx.strokeStyle = '#1e293b'; ctx.lineWidth = 0.5;
  for (var y = 0; y < H; y += 20) { ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke(); }
  for (var x = 0; x < W; x += 20) { ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,H); ctx.stroke(); }

  /* Sensor beams */
  var beams = [
    {dist: sd.left,   angle: -50},
    {dist: sd.center, angle: -90},
    {dist: sd.right,  angle: -130}
  ];

  beams.forEach(function(b) {
    var d   = Math.min(b.dist >= 999 ? MAX_CM : b.dist, MAX_CM);
    var rad = b.angle * Math.PI / 180;
    var ex  = cx + Math.cos(rad) * (d / MAX_CM) * (H * 0.55);
    var ey  = cy + Math.sin(rad) * (d / MAX_CM) * (H * 0.55);
    var col = b.dist < 20 ? '#ef4444' : b.dist < 40 ? '#f59e0b' : '#00e5ff';

    ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(ex, ey);
    ctx.strokeStyle = col; ctx.lineWidth = b.dist < 30 ? 2.5 : 1.2;
    ctx.setLineDash([4, 4]); ctx.stroke(); ctx.setLineDash([]);

    ctx.beginPath(); ctx.arc(ex, ey, b.dist < 30 ? 6 : 4, 0, Math.PI * 2);
    ctx.fillStyle = col; ctx.fill();

    if (b.dist < 999) {
      ctx.fillStyle = col; ctx.font = 'bold 10px Courier New';
      ctx.textAlign = 'center';
      ctx.fillText(Math.round(b.dist) + 'cm', ex, ey - 9);
    }
  });

  /* Robot body */
  ctx.fillStyle = '#1a2235'; ctx.strokeStyle = '#00e5ff'; ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(cx - rw + 6, cy - rh / 2);
  ctx.arcTo(cx + rw, cy - rh/2, cx + rw, cy + rh/2, 6);
  ctx.arcTo(cx + rw, cy + rh/2, cx - rw, cy + rh/2, 6);
  ctx.arcTo(cx - rw, cy + rh/2, cx - rw, cy - rh/2, 6);
  ctx.arcTo(cx - rw, cy - rh/2, cx + rw, cy - rh/2, 6);
  ctx.closePath(); ctx.fill(); ctx.stroke();

  /* Forward arrow */
  ctx.fillStyle = '#00e5ff';
  ctx.beginPath(); ctx.moveTo(cx, cy - rh/2 - 10);
  ctx.lineTo(cx - 7, cy - rh/2 + 2); ctx.lineTo(cx + 7, cy - rh/2 + 2);
  ctx.closePath(); ctx.fill();

  /* Label */
  ctx.fillStyle = '#00e5ff99'; ctx.fillRect(cx - 10, cy - 8, 20, 16);
  ctx.fillStyle = '#0a0e1a'; ctx.font = '8px Courier New';
  ctx.textAlign = 'center'; ctx.fillText('ESP32', cx, cy + 4);

  /* Wheels */
  [[cx - rw - 2, cy - 12],[cx + rw + 2, cy - 12],
   [cx - rw - 2, cy + 12],[cx + rw + 2, cy + 12]].forEach(function(p) {
    ctx.fillStyle = '#2d3f5a'; ctx.fillRect(p[0]-5, p[1]-8, 10, 16);
    ctx.strokeStyle = '#475569'; ctx.lineWidth = 0.5;
    ctx.strokeRect(p[0]-5, p[1]-8, 10, 16);
  });
}

/* -- Mode & speed controls -------------------------------------- */
function setMode(m) {
  mode = m;
  wsSend({type:'mode', value:m});
  document.getElementById('btn-m').className = 'mdbtn' + (m==='manual'?' am':'');
  document.getElementById('btn-a').className = 'mdbtn' + (m==='auto'?' aa':'');
  addLog('Mode set to ' + m.toUpperCase());
}

function onSpeed(v) {
  speed = parseInt(v);
  document.getElementById('sv-val').textContent  = v;
  document.getElementById('spd-disp').textContent = v;
  wsSend({type:'speed', value:speed});
}

function sendMove(cmd) {
  if (mode !== 'manual') return;
  wsSend({type:'move', cmd:cmd});
}

/* -- D-Pad (mouse + touch) -------------------------------------- */
document.querySelectorAll('.dbtn').forEach(function(btn) {
  var cmd = btn.dataset.c;

  function down() { btn.classList.add('pr'); sendMove(cmd); }
  function up()   { btn.classList.remove('pr'); if (cmd !== 'stop') sendMove('stop'); }

  btn.addEventListener('mousedown',  down);
  btn.addEventListener('mouseup',    up);
  btn.addEventListener('mouseleave', up);
  btn.addEventListener('touchstart', function(e){ e.preventDefault(); down(); }, {passive:false});
  btn.addEventListener('touchend',   function(e){ e.preventDefault(); up();   }, {passive:false});
});

/* -- Keyboard (WASD + Arrow keys) ------------------------------- */
var keyMap = {
  ArrowUp:'forward',  KeyW:'forward',
  ArrowDown:'backward', KeyS:'backward',
  ArrowLeft:'left',   KeyA:'left',
  ArrowRight:'right', KeyD:'right',
  Space:'stop',       KeyX:'stop'
};
var held = {};

document.addEventListener('keydown', function(e) {
  var cmd = keyMap[e.code];
  if (!cmd || held[e.code]) return;
  e.preventDefault();
  held[e.code] = true;
  sendMove(cmd);
  var b = document.querySelector('[data-c="' + cmd + '"]');
  if (b) b.classList.add('pr');
});

document.addEventListener('keyup', function(e) {
  var cmd = keyMap[e.code];
  if (!cmd) return;
  delete held[e.code];
  if (cmd !== 'stop') sendMove('stop');
  var b = document.querySelector('[data-c="' + cmd + '"]');
  if (b) b.classList.remove('pr');
});

/* -- Event log -------------------------------------------------- */
function addLog(msg, type) {
  var box = document.getElementById('logbox');
  var el  = document.createElement('div');
  el.className   = 'le' + (type ? ' ' + type : '');
  el.textContent = msg;
  box.prepend(el);
  while (box.children.length > 12) box.removeChild(box.lastChild);
}

/* -- Init ------------------------------------------------------- */
window.addEventListener('resize', drawCanvas);
connect();
drawCanvas();
</script>
</body>
</html>
)HTMLEOF";

#endif /* WEBPAGE_H */
