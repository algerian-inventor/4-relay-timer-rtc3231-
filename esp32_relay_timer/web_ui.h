#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0f13">
<title>School Bell Controller</title>
<style>
:root{
  --bg:#0b0f13;--panel:#12181e;--panel2:#182028;--line:#29343e;
  --text:#f5f7f8;--muted:#94a0aa;--green:#62d681;--amber:#e4b24d;
  --red:#ef7777;--blue:#7daaff;--radius:18px;
}
*{box-sizing:border-box}
html{background:var(--bg)}
body{margin:0;font-family:Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif;color:var(--text);background:var(--bg)}
button,input{font:inherit}
button{touch-action:manipulation}
.app{max-width:980px;margin:auto;padding:16px 14px 42px}
.topbar{display:flex;justify-content:space-between;align-items:center;gap:14px;padding:6px 2px 18px}
.identity h1{font-size:1.04rem;margin:0;font-weight:780}
.identity p{margin:4px 0 0;color:var(--muted);font-size:.78rem}
.statusline{display:flex;align-items:center;gap:7px;color:var(--muted);font-size:.76rem}
.dot{width:8px;height:8px;border-radius:50%;background:var(--green)}
.dot.bad{background:var(--red)}
.nav{display:flex;gap:6px;padding:5px;background:#0f1419;border:1px solid var(--line);border-radius:14px;overflow:auto;margin-bottom:14px}
.nav button{border:0;background:transparent;color:var(--muted);font-weight:720;padding:10px 13px;border-radius:10px;white-space:nowrap}
.nav button.active{background:#222b33;color:#fff}
.page{display:none}.page.active{display:block}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:var(--radius);padding:17px}
.hero{display:grid;grid-template-columns:1.15fr .85fr;gap:12px;margin-bottom:12px}
.clock{font-size:2.65rem;line-height:1;font-weight:800;letter-spacing:-.045em}
.date{margin-top:8px;color:var(--muted);font-size:.9rem}
.warn{margin-top:12px;padding:10px 12px;border:1px solid rgba(239,119,119,.35);background:rgba(239,119,119,.08);border-radius:12px;color:#ffc0c0;font-size:.82rem}
.next-label{color:var(--muted);font-size:.7rem;font-weight:800;letter-spacing:.08em}
.next-time{font-size:1.65rem;font-weight:800;margin-top:6px}
.next-meta{color:var(--muted);font-size:.8rem;margin-top:3px}
.controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:12px}
.action{border:1px solid var(--line);background:#1d252c;color:#fff;border-radius:14px;padding:14px;font-weight:760;min-height:52px}
.action.primary{background:#f5f7f8;color:#101418;border-color:#f5f7f8}
.action.danger{background:rgba(239,119,119,.09);color:#ffc0c0;border-color:rgba(239,119,119,.32)}
.action:disabled{opacity:.48}
.metrics{display:grid;grid-template-columns:repeat(3,1fr);gap:9px}
.metric{background:var(--panel2);border:1px solid var(--line);border-radius:13px;padding:12px}
.metric b{display:block;font-size:1rem}
.metric span{display:block;color:var(--muted);font-size:.69rem;margin-top:3px}
.section-head{display:flex;justify-content:space-between;align-items:end;gap:12px;margin:2px 0 12px}
.section-head h2{margin:0;font-size:1.08rem}
.section-head p{margin:4px 0 0;color:var(--muted);font-size:.78rem}
.schedule-list{display:grid;gap:9px}
.schedule-card{display:grid;grid-template-columns:82px 1fr auto;gap:12px;align-items:center;background:var(--panel);border:1px solid var(--line);border-radius:15px;padding:13px}
.schedule-time{font-size:1.2rem;font-weight:800}
.schedule-meta{min-width:0}
.days{display:flex;gap:4px;flex-wrap:wrap}
.day{font-size:.64rem;padding:4px 6px;border-radius:7px;background:#202831;color:#71808b}
.day.on{background:#303b45;color:#fff}
.duration{margin-top:6px;color:var(--muted);font-size:.72rem}
.badge{font-size:.67rem;font-weight:800;padding:6px 8px;border-radius:999px;background:#232b32;color:var(--muted)}
.badge.on{background:rgba(98,214,129,.12);color:var(--green)}
.editbtn{margin-top:7px;width:100%;border:1px solid var(--line);background:#202830;color:#fff;padding:8px 10px;border-radius:10px;font-weight:700}
.events{display:grid;gap:8px}
.event{display:grid;grid-template-columns:90px 1fr;gap:10px;background:var(--panel);border:1px solid var(--line);border-radius:13px;padding:11px}
.event time{color:var(--muted);font-size:.74rem}
.settings-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
.setting h3{margin:0 0 5px;font-size:.94rem}
.setting p{margin:0 0 12px;color:var(--muted);font-size:.78rem;line-height:1.45}
.smallbtn{width:100%;border:1px solid var(--line);background:#202830;color:#fff;padding:11px;border-radius:11px;font-weight:720}
.empty{padding:24px;text-align:center;color:var(--muted);border:1px dashed var(--line);border-radius:14px}
dialog{width:min(92vw,500px);border:1px solid var(--line);border-radius:18px;background:#12181e;color:#fff;padding:18px}
dialog::backdrop{background:rgba(0,0,0,.72)}
dialog h2{margin:0 0 14px;font-size:1.02rem}
.form{display:grid;grid-template-columns:1fr 1fr;gap:10px}
label{display:flex;flex-direction:column;gap:6px;color:var(--muted);font-size:.75rem}
input[type=time],input[type=number]{width:100%;background:#0d1216;color:#fff;border:1px solid var(--line);border-radius:11px;padding:11px;font-size:16px}
.check{display:flex;flex-direction:row;align-items:center;gap:8px;color:#fff;margin:14px 0}
.week{display:grid;grid-template-columns:repeat(7,1fr);gap:5px;margin:10px 0}
.week button{border:1px solid var(--line);background:#182028;color:var(--muted);border-radius:9px;padding:9px 2px;font-size:.67rem}
.week button.on{background:#34414c;color:#fff}
.actions{display:flex;gap:8px;margin-top:14px}
.actions button{flex:1;border:1px solid var(--line);border-radius:11px;padding:11px;background:#202830;color:#fff;font-weight:750}
.actions .primary{background:#fff;color:#111;border-color:#fff}
@media(max-width:700px){
  .hero,.settings-grid{grid-template-columns:1fr}
  .controls{grid-template-columns:1fr}
  .schedule-card{grid-template-columns:74px 1fr}
  .schedule-card>div:last-child{grid-column:1/-1;display:flex;align-items:center;gap:8px}
  .editbtn{width:auto;margin:0 0 0 auto}
  .metrics{grid-template-columns:repeat(3,1fr)}
  .clock{font-size:2.3rem}
}
</style>
</head>
<body>
<div class="app">
  <header class="topbar">
    <div class="identity">
      <h1>School Bell Controller</h1>
      <p>ESP32 + DS3231 • standalone access point</p>
    </div>
    <div class="statusline"><span id="statusDot" class="dot"></span><span id="networkStatus">Connecting…</span></div>
  </header>

  <nav class="nav">
    <button class="active" data-page="overview">Overview</button>
    <button data-page="schedule">Schedule</button>
    <button data-page="history">History</button>
    <button data-page="device">Device</button>
  </nav>

  <section id="overview" class="page active">
    <div class="hero">
      <div class="panel">
        <div id="clock" class="clock">--:--:--</div>
        <div id="date" class="date">Loading RTC…</div>
        <div id="rtcWarning" class="warn" hidden>RTC time is not valid. Synchronize it before enabling automatic ringing.</div>
      </div>
      <div class="panel">
        <div class="next-label">NEXT BELL</div>
        <div id="nextTime" class="next-time">--:--</div>
        <div id="nextMeta" class="next-meta">Checking schedule…</div>
      </div>
    </div>

    <div class="controls">
      <button id="ringBtn" class="action primary">Ring now · 3 s</button>
      <button id="autoBtn" class="action">Pause automatic schedule</button>
    </div>

    <div class="metrics">
      <div class="metric"><b id="bellState">OFF</b><span>Bell output</span></div>
      <div class="metric"><b id="clients">0</b><span>Connected devices</span></div>
      <div class="metric"><b id="temp">--.-°</b><span>RTC temperature</span></div>
    </div>
  </section>

  <section id="schedule" class="page">
    <div class="section-head">
      <div><h2>Bell schedule</h2><p>Each event produces a short relay pulse. Default school days are Sunday–Thursday.</p></div>
    </div>
    <div id="scheduleList" class="schedule-list"></div>
  </section>

  <section id="history" class="page">
    <div class="section-head">
      <div><h2>History</h2><p>Recent runtime events since the last restart.</p></div>
      <button id="refreshHistory" class="smallbtn" style="width:auto">Refresh</button>
    </div>
    <div id="historyList" class="events"></div>
  </section>

  <section id="device" class="page">
    <div class="settings-grid">
      <div class="panel setting">
        <h3>RTC synchronization</h3>
        <p>Copy the current date and time from this phone or computer to the DS3231.</p>
        <button id="syncBtn" class="smallbtn">Sync RTC from this device</button>
      </div>
      <div class="panel setting">
        <h3>Local network</h3>
        <p id="networkInfo">Loading…</p>
      </div>
      <div class="panel setting">
        <h3>Firmware</h3>
        <p id="firmwareInfo">Loading…</p>
      </div>
      <div class="panel setting">
        <h3>Factory defaults</h3>
        <p>Restore the 12 default bell times and Sunday–Thursday schedule.</p>
        <button id="factoryBtn" class="smallbtn">Restore default timetable</button>
      </div>
    </div>
  </section>
</div>

<dialog id="eventDialog">
  <h2 id="eventTitle">Edit bell event</h2>
  <div class="form">
    <label>Time<input id="eventTime" type="time" required></label>
    <label>Ring duration (seconds)<input id="eventDuration" type="number" min="1" max="30" step="1" required></label>
  </div>
  <label class="check"><input id="eventEnabled" type="checkbox"> Event enabled</label>
  <div class="week" id="weekButtons"></div>
  <div class="actions">
    <button id="cancelEdit">Cancel</button>
    <button id="saveEdit" class="primary">Save</button>
  </div>
</dialog>

<script>
let state = null;
let editSlot = -1;
let editDays = 0x1F;
const dayShort = ['Su','Mo','Tu','We','Th','Fr','Sa'];
const dayLong = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];

const $ = id => document.getElementById(id);
const pad = n => String(n).padStart(2,'0');
const esc = value => {
  const d = document.createElement('div');
  d.textContent = String(value);
  return d.innerHTML;
};

function showPage(name) {
  document.querySelectorAll('.nav button').forEach(b => b.classList.toggle('active', b.dataset.page === name));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === name));
  if (name === 'history') loadHistory();
}

document.querySelectorAll('.nav button').forEach(b => {
  b.addEventListener('click', () => showPage(b.dataset.page));
});

async function post(url, data) {
  const response = await fetch(url, {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(data)
  });

  let payload = {};
  try { payload = await response.json(); } catch (_) {}

  if (!response.ok) {
    const message = payload.error ? payload.error.replaceAll('_',' ') : 'request failed';
    throw new Error(message);
  }

  return payload;
}

function minuteFromTime(text) {
  const parts = String(text).split(':');
  if (parts.length !== 2) return null;
  const h = Number(parts[0]), m = Number(parts[1]);
  if (!Number.isInteger(h) || !Number.isInteger(m) || h < 0 || h > 23 || m < 0 || m > 59) return null;
  return h * 60 + m;
}

function nextBell() {
  if (!state || !state.rtcValid || !state.bellEvents) return null;

  const timeParts = state.time.split(':').map(Number);
  const nowMinute = timeParts[0] * 60 + timeParts[1];

  let best = null;

  for (let dayOffset = 0; dayOffset < 7; dayOffset++) {
    const day = (state.dayOfWeek + dayOffset) % 7;

    state.bellEvents.forEach(event => {
      if (!event.enabled || !(event.daysMask & (1 << day))) return;
      if (dayOffset === 0 && event.minuteOfDay <= nowMinute) return;

      const delta = dayOffset * 1440 + event.minuteOfDay - (dayOffset === 0 ? nowMinute : 0);
      if (!best || delta < best.delta) best = {event, day, dayOffset, delta};
    });

    if (best && best.dayOffset === dayOffset) break;
  }

  return best;
}

function renderOverview() {
  $('clock').textContent = state.time;
  $('date').textContent = state.date;
  $('rtcWarning').hidden = state.rtcValid;

  $('bellState').textContent = state.bellRinging ? 'RINGING' : 'OFF';
  $('clients').textContent = state.clients;
  $('temp').textContent = Number(state.temperature || 0).toFixed(1) + '°';

  $('ringBtn').disabled = false;
  $('ringBtn').textContent = state.bellRinging ? 'Stop bell' : 'Ring now · 3 s';
  $('ringBtn').classList.toggle('danger', state.bellRinging);
  $('ringBtn').classList.toggle('primary', !state.bellRinging);

  $('autoBtn').textContent = state.automationEnabled ? 'Pause automatic schedule' : 'Resume automatic schedule';
  $('autoBtn').classList.toggle('danger', state.automationEnabled);

  const next = nextBell();
  if (!state.rtcValid) {
    $('nextTime').textContent = '--:--';
    $('nextMeta').textContent = 'Synchronize RTC first';
  } else if (!next) {
    $('nextTime').textContent = 'None';
    $('nextMeta').textContent = 'No enabled event in the next 7 days';
  } else {
    $('nextTime').textContent = next.event.time;
    const when = next.dayOffset === 0 ? 'Today' : (next.dayOffset === 1 ? 'Tomorrow' : dayLong[next.day]);
    $('nextMeta').textContent = when + ' · ' + next.event.durationSeconds + ' s pulse';
  }
}

function renderSchedule() {
  const host = $('scheduleList');
  host.innerHTML = '';

  state.bellEvents.forEach((event, index) => {
    const card = document.createElement('article');
    card.className = 'schedule-card';

    let days = '';
    dayShort.forEach((name, day) => {
      days += '<span class="day ' + ((event.daysMask & (1 << day)) ? 'on' : '') + '">' + name + '</span>';
    });

    card.innerHTML =
      '<div class="schedule-time">' + esc(event.time) + '</div>' +
      '<div class="schedule-meta">' +
        '<div class="days">' + days + '</div>' +
        '<div class="duration">Event ' + pad(index + 1) + ' · ' + event.durationSeconds + ' second pulse</div>' +
      '</div>' +
      '<div>' +
        '<span class="badge ' + (event.enabled ? 'on' : '') + '">' + (event.enabled ? 'ENABLED' : 'DISABLED') + '</span>' +
        '<button class="editbtn" data-slot="' + index + '">Edit</button>' +
      '</div>';

    host.appendChild(card);
  });

  host.querySelectorAll('.editbtn').forEach(button => {
    button.addEventListener('click', () => openEvent(Number(button.dataset.slot)));
  });
}

function renderDevice() {
  $('networkInfo').textContent =
    state.ssid + ' · ' + state.ip + ' · ' + state.clients + ' connected device(s)' +
    (state.apReady ? '' : ' · AP fault');

  $('firmwareInfo').textContent =
    state.firmware + ' · settings schema ' + state.schema +
    (state.rtcValid ? ' · RTC OK' : ' · RTC needs attention');
}

function render() {
  if (!state) return;

  $('networkStatus').textContent = state.apReady ? 'Connected' : 'AP fault';
  $('statusDot').classList.toggle('bad', !state.apReady);

  renderOverview();
  renderSchedule();
  renderDevice();
}

async function refresh() {
  try {
    const response = await fetch('/api/status', {cache:'no-store'});
    if (!response.ok) throw new Error('status unavailable');
    state = await response.json();
    render();
  } catch (_) {
    $('networkStatus').textContent = 'Reconnecting…';
    $('statusDot').classList.add('bad');
  }
}

function renderWeekButtons() {
  const host = $('weekButtons');
  host.innerHTML = '';

  dayShort.forEach((name, day) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = name;
    button.className = (editDays & (1 << day)) ? 'on' : '';
    button.addEventListener('click', () => {
      editDays ^= (1 << day);
      renderWeekButtons();
    });
    host.appendChild(button);
  });
}

function openEvent(slot) {
  const event = state.bellEvents[slot];
  editSlot = slot;
  editDays = event.daysMask;
  $('eventTitle').textContent = 'Edit bell event ' + (slot + 1);
  $('eventTime').value = event.time;
  $('eventDuration').value = event.durationSeconds;
  $('eventEnabled').checked = event.enabled;
  renderWeekButtons();
  $('eventDialog').showModal();
}

$('cancelEdit').addEventListener('click', () => $('eventDialog').close());

$('saveEdit').addEventListener('click', async () => {
  const minuteOfDay = minuteFromTime($('eventTime').value);
  const durationSeconds = Number($('eventDuration').value);

  if (minuteOfDay === null) {
    alert('Choose a valid time.');
    return;
  }

  if (!Number.isInteger(durationSeconds) || durationSeconds < 1 || durationSeconds > 30) {
    alert('Ring duration must be between 1 and 30 seconds.');
    return;
  }

  if ($('eventEnabled').checked && editDays === 0) {
    alert('Select at least one day, or disable the event.');
    return;
  }

  try {
    await post('/api/bell-event', {
      slot:editSlot,
      minuteOfDay,
      durationSeconds,
      daysMask:editDays,
      enabled:$('eventEnabled').checked
    });
    $('eventDialog').close();
    await refresh();
  } catch (error) {
    alert('Could not save: ' + error.message);
  }
});

$('ringBtn').addEventListener('click', async () => {
  try {
    if (state && state.bellRinging) {
      await post('/api/stop', {});
    } else {
      await post('/api/ring', {durationSeconds:3});
    }
    await refresh();
  } catch (error) {
    alert('Bell control failed: ' + error.message);
  }
});

$('autoBtn').addEventListener('click', async () => {
  if (!state) return;
  try {
    await post('/api/automation', {enabled:!state.automationEnabled});
    await refresh();
  } catch (error) {
    alert('Could not change automation: ' + error.message);
  }
});

$('syncBtn').addEventListener('click', async () => {
  const now = new Date();
  try {
    await post('/api/time', {
      year:now.getFullYear(),
      month:now.getMonth()+1,
      day:now.getDate(),
      hour:now.getHours(),
      minute:now.getMinutes(),
      second:now.getSeconds()
    });
    await refresh();
    alert('RTC synchronized.');
  } catch (error) {
    alert('RTC synchronization failed: ' + error.message);
  }
});

$('factoryBtn').addEventListener('click', async () => {
  if (!confirm('Restore the 12 default bell times? Your schedule changes will be lost.')) return;

  try {
    await post('/api/factory-reset', {confirm:true});
    await refresh();
    alert('Default timetable restored.');
  } catch (error) {
    alert('Factory reset failed: ' + error.message);
  }
});

$('refreshHistory').addEventListener('click', loadHistory);

async function loadHistory() {
  try {
    const response = await fetch('/api/history', {cache:'no-store'});
    const data = await response.json();
    $('historyList').innerHTML = data.events.length
      ? data.events.map(e => '<div class="event"><time>' + esc(e.time) + '</time><div>' + esc(e.message) + '</div></div>').join('')
      : '<div class="empty">No events yet.</div>';
  } catch (_) {
    $('historyList').innerHTML = '<div class="empty">Could not load history.</div>';
  }
}

refresh();
setInterval(refresh, 1000);
</script>
</body>
</html>
)HTML";
