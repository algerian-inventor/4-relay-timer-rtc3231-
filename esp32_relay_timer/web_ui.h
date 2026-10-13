#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0e11">
<title>School Bell Controller</title>
<style>
:root{--bg:#0a0d10;--panel:#11161b;--panel2:#171d23;--line:#29323a;--text:#f4f7f8;--muted:#95a0aa;--green:#5ad37a;--amber:#e8b856;--red:#ef7373;--radius:18px}
*{box-sizing:border-box}html{background:var(--bg)}body{margin:0;font-family:Inter,system-ui,-apple-system,"Segoe UI",sans-serif;background:var(--bg);color:var(--text)}
button,input{font:inherit}.app{max-width:980px;margin:auto;padding:16px 14px 42px}.top{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:14px}.top h1{font-size:1.05rem;margin:0}.top p{font-size:.78rem;color:var(--muted);margin:4px 0 0}.net{font-size:.76rem;color:var(--muted)}
.nav{display:flex;gap:6px;overflow:auto;border:1px solid var(--line);padding:5px;border-radius:14px;margin-bottom:14px}.nav button{border:0;background:transparent;color:var(--muted);padding:10px 14px;border-radius:10px;font-weight:750;white-space:nowrap}.nav button.active{background:#20272d;color:#fff}
.page{display:none}.page.active{display:block}.panel{background:var(--panel);border:1px solid var(--line);border-radius:var(--radius);padding:16px}.hero{display:grid;grid-template-columns:1fr 1fr;gap:12px}.clock{font-size:2.6rem;font-weight:800;letter-spacing:-.04em}.muted{color:var(--muted)}.status{display:grid;grid-template-columns:repeat(3,1fr);gap:9px;margin-top:12px}.metric{background:var(--panel2);border:1px solid var(--line);border-radius:13px;padding:12px}.metric b{display:block;font-size:1rem}.metric span{font-size:.7rem;color:var(--muted)}
.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px}.btn{border:1px solid var(--line);border-radius:12px;padding:12px;background:#20272e;color:#fff;font-weight:800;min-height:44px}.btn.primary{background:#f4f7f8;color:#111}.btn.danger{color:#ffaaaa;background:rgba(239,115,115,.08)}.btn.warn{color:#ffd98b}.btn:disabled{opacity:.45}
.notice{margin-top:12px;border:1px solid rgba(232,184,86,.35);background:rgba(232,184,86,.08);color:#f3d38f;padding:12px;border-radius:12px;font-size:.82rem}
.sectionhead{display:flex;justify-content:space-between;align-items:end;gap:10px;margin:4px 0 12px}.sectionhead h2{margin:0;font-size:1.1rem}.sectionhead p{margin:4px 0 0;color:var(--muted);font-size:.78rem}.list{display:flex;flex-direction:column;gap:8px}.event{display:grid;grid-template-columns:90px 1fr auto;gap:12px;align-items:center;background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:12px}.etime{font-size:1.2rem;font-weight:850}.days{display:flex;gap:4px;flex-wrap:wrap;margin-top:5px}.day{font-size:.64rem;padding:4px 6px;border-radius:6px;background:#20262c;color:var(--muted)}.day.on{color:#fff;background:#303941}.pill{font-size:.68rem;color:var(--green);background:rgba(90,211,122,.1);padding:6px 8px;border-radius:999px}.event button{width:auto;margin:0}.empty{padding:24px;text-align:center;border:1px dashed var(--line);border-radius:14px;color:var(--muted)}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}.history{display:flex;flex-direction:column;gap:7px}.log{display:grid;grid-template-columns:90px 1fr;gap:10px;padding:10px 12px;border:1px solid var(--line);border-radius:12px;background:var(--panel)}.log time{font-size:.73rem;color:var(--muted)}
dialog{width:min(92vw,500px);border:1px solid var(--line);border-radius:18px;background:#12171c;color:#fff;padding:18px}dialog::backdrop{background:rgba(0,0,0,.72)}dialog h2{margin:0 0 14px;font-size:1.05rem}label{display:flex;flex-direction:column;gap:6px;font-size:.75rem;color:var(--muted);margin-bottom:10px}input[type=time],input[type=number]{width:100%;background:#0c1013;color:#fff;border:1px solid var(--line);border-radius:11px;padding:11px;font-size:16px}.week{display:grid;grid-template-columns:repeat(7,1fr);gap:5px;margin:12px 0}.week button{border:1px solid var(--line);background:#171d23;color:var(--muted);border-radius:9px;padding:9px 3px}.week button.on{background:#303941;color:#fff}.check{display:flex;flex-direction:row;align-items:center;color:#fff;gap:8px}.dialogactions{display:flex;gap:8px;margin-top:14px}.dialogactions button{flex:1}
@media(max-width:720px){.hero,.grid2{grid-template-columns:1fr}.status{grid-template-columns:repeat(3,1fr)}.event{grid-template-columns:76px 1fr}.event>button{grid-column:1/-1;width:100%}.clock{font-size:2.2rem}}
</style>
</head>
<body>
<div class="app">
  <header class="top">
    <div><h1>School Bell Controller</h1><p>ESP32 + DS3231 · standalone access point</p></div>
    <div class="net" id="net">Connecting…</div>
  </header>

  <nav class="nav">
    <button class="active" data-page="home">Home</button>
    <button data-page="schedule">Schedule</button>
    <button data-page="history">History</button>
    <button data-page="device">Device</button>
  </nav>

  <section id="home" class="page active">
    <div class="hero">
      <div class="panel">
        <div class="clock" id="clock">--:--:--</div>
        <div class="muted" id="date">Loading RTC…</div>
        <div id="rtcNotice" class="notice" style="display:none">RTC time is not valid. Synchronize it before relying on automatic bells.</div>
      </div>
      <div class="panel">
        <div class="muted" style="font-size:.72rem">NEXT BELL</div>
        <div id="nextBell" style="font-size:1.45rem;font-weight:850;margin-top:5px">--</div>
        <div id="nextBellMeta" class="muted" style="margin-top:5px;font-size:.8rem">Waiting for schedule</div>
        <div class="status">
          <div class="metric"><b id="relayState">OFF</b><span>Relay</span></div>
          <div class="metric"><b id="autoState">ON</b><span>Automation</span></div>
          <div class="metric"><b id="clients">0</b><span>Clients</span></div>
        </div>
      </div>
    </div>

    <div class="actions">
      <button class="btn primary" id="ringBtn">Ring now · 3s</button>
      <button class="btn" id="autoBtn">Disable automation</button>
    </div>
  </section>

  <section id="schedule" class="page">
    <div class="sectionhead">
      <div><h2>Bell schedule</h2><p>Each entry is a short one-shot relay pulse.</p></div>
      <button class="btn" id="addBtn" style="width:auto">Add bell</button>
    </div>
    <div class="list" id="eventList"></div>
  </section>

  <section id="history" class="page">
    <div class="sectionhead">
      <div><h2>History</h2><p>Recent runtime events.</p></div>
      <button class="btn" id="refreshHistory" style="width:auto">Refresh</button>
    </div>
    <div class="history" id="historyList"></div>
  </section>

  <section id="device" class="page">
    <div class="grid2">
      <div class="panel">
        <h3 style="margin-top:0">RTC synchronization</h3>
        <p class="muted">Copy the current date and time from this phone or computer to the DS3231.</p>
        <button class="btn" id="syncBtn">Sync RTC from this device</button>
      </div>
      <div class="panel">
        <h3 style="margin-top:0">Network</h3>
        <p class="muted" id="networkInfo">Loading…</p>
      </div>
      <div class="panel">
        <h3 style="margin-top:0">Firmware</h3>
        <p class="muted" id="firmwareInfo">Loading…</p>
      </div>
      <div class="panel">
        <h3 style="margin-top:0">Factory timetable</h3>
        <p class="muted">Restore the original 12 default bell times and Sunday–Thursday schedule.</p>
        <button class="btn danger" id="resetBtn">Restore defaults</button>
      </div>
    </div>
  </section>
</div>

<dialog id="eventDialog">
  <h2 id="dialogTitle">Edit bell</h2>
  <label>Bell time<input id="bellTime" type="time"></label>
  <label>Pulse duration in seconds<input id="durationSec" type="number" min="1" max="15" inputmode="numeric"></label>
  <label class="check"><input id="eventEnabled" type="checkbox"> Enabled</label>
  <div class="week" id="week"></div>
  <div class="dialogactions">
    <button class="btn" id="cancelEvent">Cancel</button>
    <button class="btn danger" id="deleteEvent">Disable</button>
    <button class="btn primary" id="saveEvent">Save</button>
  </div>
</dialog>

<script>
let state=null;
let editSlot=-1;
let editDays=31;
const dayNames=['Su','Mo','Tu','We','Th','Fr','Sa'];

const $=id=>document.getElementById(id);
const pad=n=>String(n).padStart(2,'0');
const esc=s=>{const d=document.createElement('div');d.textContent=s;return d.innerHTML};

document.querySelectorAll('.nav button').forEach(btn=>{
  btn.addEventListener('click',()=>{
    document.querySelectorAll('.nav button').forEach(x=>x.classList.remove('active'));
    document.querySelectorAll('.page').forEach(x=>x.classList.remove('active'));
    btn.classList.add('active');
    $(btn.dataset.page).classList.add('active');
    if(btn.dataset.page==='history') loadHistory();
  });
});

async function post(url,data){
  const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
  if(!r.ok) throw new Error(await r.text());
  return r.json();
}

function activeEvents(){
  return state ? state.events.filter(e=>e.enabled) : [];
}

function computeNextBell(){
  if(!state || !state.rtcValid || !state.automationEnabled) return null;
  let best=null;
  for(let offset=0;offset<7;offset++){
    const dow=(state.dayOfWeek+offset)%7;
    state.events.forEach(e=>{
      if(!e.enabled || !(e.daysMask&(1<<dow))) return;
      if(offset===0 && e.minuteOfDay<=state.minuteOfDay) return;
      const delta=offset*1440 + e.minuteOfDay - state.minuteOfDay;
      if(best===null || delta<best.delta) best={event:e,offset,delta,dow};
    });
  }
  return best;
}

function render(){
  if(!state) return;

  $('clock').textContent=state.time;
  $('date').textContent=state.date;
  $('rtcNotice').style.display=state.rtcValid?'none':'block';
  $('relayState').textContent=state.relayOn?'ON':'OFF';
  $('autoState').textContent=state.automationEnabled?'ON':'OFF';
  $('clients').textContent=state.clients;
  $('autoBtn').textContent=state.automationEnabled?'Disable automation':'Enable automation';
  $('autoBtn').className='btn '+(state.automationEnabled?'warn':'');
  $('networkInfo').textContent=state.ssid+' · '+state.ip+' · '+state.clients+' client(s)';
  $('firmwareInfo').textContent=state.firmware+' · schema '+state.schema;

  const next=computeNextBell();
  if(next){
    $('nextBell').textContent=next.event.time;
    $('nextBellMeta').textContent=(next.offset===0?'Today':next.offset===1?'Tomorrow':dayNames[next.dow])+' · '+next.event.durationSec+'s pulse';
  }else{
    $('nextBell').textContent='--';
    $('nextBellMeta').textContent=!state.rtcValid?'RTC not ready':!state.automationEnabled?'Automation disabled':'No upcoming enabled bell';
  }

  renderEvents();
}

function renderEvents(){
  if(!state) return;
  const rows=activeEvents().slice().sort((a,b)=>a.minuteOfDay-b.minuteOfDay);
  if(!rows.length){
    $('eventList').innerHTML='<div class="empty">No enabled bell events.</div>';
    return;
  }

  $('eventList').innerHTML=rows.map(e=>{
    let days='';
    dayNames.forEach((d,i)=>days+='<span class="day '+((e.daysMask&(1<<i))?'on':'')+'">'+d+'</span>');
    return '<div class="event">'+
      '<div class="etime">'+e.time+'</div>'+
      '<div><div><span class="pill">'+e.durationSec+'s pulse</span></div><div class="days">'+days+'</div></div>'+
      '<button class="btn" data-edit="'+e.slot+'">Edit</button>'+
      '</div>';
  }).join('');

  document.querySelectorAll('[data-edit]').forEach(b=>b.addEventListener('click',()=>openEvent(Number(b.dataset.edit))));
}

function renderWeek(){
  $('week').innerHTML='';
  dayNames.forEach((name,i)=>{
    const b=document.createElement('button');
    b.type='button';
    b.textContent=name;
    b.className=(editDays&(1<<i))?'on':'';
    b.addEventListener('click',()=>{editDays^=(1<<i);renderWeek()});
    $('week').appendChild(b);
  });
}

function openEvent(slot){
  const e=state.events.find(x=>x.slot===slot);
  if(!e) return;
  editSlot=slot;
  editDays=e.daysMask;
  $('bellTime').value=e.time;
  $('durationSec').value=e.durationSec;
  $('eventEnabled').checked=e.enabled;
  $('dialogTitle').textContent='Edit bell '+pad(slot+1);
  $('deleteEvent').style.display=e.enabled?'block':'none';
  renderWeek();
  $('eventDialog').showModal();
}

function addEvent(){
  const free=state.events.find(e=>!e.enabled);
  if(!free){alert('Maximum of '+state.events.length+' bell events reached.');return}
  editSlot=free.slot;
  editDays=31;
  $('bellTime').value='08:00';
  $('durationSec').value=3;
  $('eventEnabled').checked=true;
  $('dialogTitle').textContent='Add bell';
  $('deleteEvent').style.display='none';
  renderWeek();
  $('eventDialog').showModal();
}

async function saveEvent(){
  const parts=$('bellTime').value.split(':');
  if(parts.length!==2){alert('Choose a valid time.');return}
  const minuteOfDay=(+parts[0])*60+(+parts[1]);
  const duration=Number($('durationSec').value);
  if(!Number.isInteger(duration)||duration<1||duration>15){alert('Duration must be 1 to 15 seconds.');return}

  await post('/api/event',{
    slot:editSlot,
    enabled:$('eventEnabled').checked,
    daysMask:editDays,
    minuteOfDay,
    durationSec:duration
  });
  $('eventDialog').close();
  await refresh();
}

async function disableEvent(){
  const e=state.events.find(x=>x.slot===editSlot);
  if(!e) return;
  await post('/api/event',{
    slot:e.slot,enabled:false,daysMask:e.daysMask,
    minuteOfDay:e.minuteOfDay,durationSec:e.durationSec
  });
  $('eventDialog').close();
  await refresh();
}

async function refresh(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    if(!r.ok) throw new Error('status');
    state=await r.json();
    $('net').textContent='Connected';
    render();
  }catch(e){
    $('net').textContent='Reconnecting…';
  }
}

async function loadHistory(){
  try{
    const r=await fetch('/api/events',{cache:'no-store'});
    const d=await r.json();
    $('historyList').innerHTML=d.events.length
      ?d.events.map(e=>'<div class="log"><time>'+esc(e.time)+'</time><div>'+esc(e.message)+'</div></div>').join('')
      :'<div class="empty">No events yet.</div>';
  }catch(e){
    $('historyList').innerHTML='<div class="empty">Could not load history.</div>';
  }
}

$('ringBtn').addEventListener('click',async()=>{
  try{
    await post('/api/ring',{durationSec:3});
    await refresh();
  }catch(e){alert('Could not ring the bell.')}
});

$('autoBtn').addEventListener('click',async()=>{
  if(!state) return;
  try{
    await post('/api/automation',{enabled:!state.automationEnabled});
    await refresh();
  }catch(e){alert('Could not change automation state.')}
});

$('addBtn').addEventListener('click',addEvent);
$('cancelEvent').addEventListener('click',()=>$('eventDialog').close());
$('saveEvent').addEventListener('click',()=>saveEvent().catch(()=>alert('Could not save bell event.')));
$('deleteEvent').addEventListener('click',()=>disableEvent().catch(()=>alert('Could not disable bell event.')));
$('refreshHistory').addEventListener('click',loadHistory);

$('syncBtn').addEventListener('click',async()=>{
  const d=new Date();
  try{
    await post('/api/time',{
      year:d.getFullYear(),month:d.getMonth()+1,day:d.getDate(),
      hour:d.getHours(),minute:d.getMinutes(),second:d.getSeconds()
    });
    await refresh();
    alert('RTC synchronized.');
  }catch(e){alert('RTC synchronization failed.')}
});

$('resetBtn').addEventListener('click',async()=>{
  if(!confirm('Restore the original default timetable?')) return;
  try{
    await post('/api/factory-reset',{confirm:true});
    await refresh();
    alert('Default timetable restored.');
  }catch(e){alert('Factory reset failed.')}
});

refresh();
setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";
