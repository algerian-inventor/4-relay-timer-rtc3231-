#pragma once

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#111418">
<title>Relay Control</title>
<style>
:root{
  --bg:#0d1013;
  --panel:#15191e;
  --panel2:#1b2026;
  --line:#2a3139;
  --text:#f3f6f8;
  --muted:#9da7b1;
  --good:#47d16c;
  --warn:#f0b74a;
  --danger:#ff6767;
  --radius:20px;
}
*{box-sizing:border-box}
body{
  margin:0;
  font-family:Inter,ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
  background:linear-gradient(180deg,#101418 0%,var(--bg) 45%,#090b0d 100%);
  color:var(--text);
  min-height:100vh;
}
.wrap{max-width:1040px;margin:0 auto;padding:20px 16px 40px}
.top{
  display:flex;align-items:center;justify-content:space-between;gap:16px;
  padding:18px 0 22px
}
.brand h1{font-size:1.15rem;margin:0;font-weight:750;letter-spacing:.02em}
.brand p{margin:5px 0 0;color:var(--muted);font-size:.82rem}
.status{
  display:flex;align-items:center;gap:8px;
  color:var(--muted);font-size:.82rem
}
.dot{width:9px;height:9px;border-radius:50%;background:var(--good);box-shadow:0 0 14px rgba(71,209,108,.7)}
.hero{
  border:1px solid var(--line);
  background:rgba(21,25,30,.88);
  border-radius:var(--radius);
  padding:20px;
  display:grid;
  grid-template-columns:1.2fr .8fr;
  gap:18px;
  margin-bottom:16px
}
.clock{font-size:2.4rem;font-weight:760;letter-spacing:-.04em}
.date,.temp{color:var(--muted);margin-top:5px}
.hero-right{display:flex;flex-direction:column;justify-content:center;align-items:flex-end}
.hero-right b{font-size:1.1rem}
.hero-right span{color:var(--muted);font-size:.8rem;margin-top:4px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}
.card{
  border:1px solid var(--line);
  background:rgba(21,25,30,.92);
  border-radius:var(--radius);
  padding:18px
}
.row{display:flex;justify-content:space-between;align-items:center;gap:10px}
.title{font-size:1rem;font-weight:700}
.sub{color:var(--muted);font-size:.8rem;margin-top:3px}
.badge{
  font-size:.72rem;font-weight:800;letter-spacing:.06em;
  padding:7px 10px;border-radius:999px;background:#242a31;color:var(--muted)
}
.badge.on{background:rgba(71,209,108,.14);color:var(--good)}
.schedule{
  margin:16px 0 14px;
  background:var(--panel2);
  border:1px solid var(--line);
  border-radius:14px;
  padding:12px
}
.schedule .times{font-weight:700}
.schedule .label{color:var(--muted);font-size:.75rem;margin-bottom:5px}
.mode{
  display:grid;grid-template-columns:repeat(3,1fr);
  background:#0f1215;border:1px solid var(--line);border-radius:13px;padding:4px;gap:4px
}
.mode button{
  border:0;border-radius:10px;padding:10px 7px;
  color:var(--muted);background:transparent;font-weight:700;cursor:pointer
}
.mode button.active{background:#262d34;color:white}
.mode button:active{transform:scale(.98)}
.edit{
  width:100%;margin-top:10px;padding:11px 12px;border-radius:12px;
  border:1px solid var(--line);background:#20262c;color:white;font-weight:700;cursor:pointer
}
dialog{
  width:min(92vw,430px);border:1px solid var(--line);border-radius:20px;
  background:#15191e;color:white;padding:20px
}
dialog::backdrop{background:rgba(0,0,0,.7);backdrop-filter:blur(4px)}
dialog h2{margin:0 0 16px;font-size:1.1rem}
.form{display:grid;grid-template-columns:1fr 1fr;gap:12px}
label{display:flex;flex-direction:column;gap:6px;color:var(--muted);font-size:.8rem}
input[type=time]{
  background:#0f1215;color:white;border:1px solid var(--line);border-radius:12px;padding:12px;font:inherit
}
.check{display:flex;align-items:center;gap:8px;margin:16px 0;color:white}
.actions{display:flex;gap:10px}
.actions button{
  flex:1;border:1px solid var(--line);border-radius:12px;padding:11px;
  font-weight:750;cursor:pointer;background:#20262c;color:white
}
.actions .save{background:white;color:#111;border-color:white}
.foot{color:var(--muted);font-size:.72rem;text-align:center;margin-top:20px}
@media(max-width:720px){
  .hero{grid-template-columns:1fr}
  .hero-right{align-items:flex-start}
  .grid{grid-template-columns:1fr}
  .clock{font-size:2rem}
}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="brand">
      <h1>4-Relay Timer</h1>
      <p>ESP32 local control panel</p>
    </div>
    <div class="status"><span class="dot"></span><span id="net">Connected</span></div>
  </div>

  <section class="hero">
    <div>
      <div class="clock" id="clock">--:--:--</div>
      <div class="date" id="date">Loading RTC...</div>
    </div>
    <div class="hero-right">
      <b id="temp">--.- °C</b>
      <span>DS3231 temperature</span>
    </div>
  </section>

  <main class="grid" id="relayGrid"></main>
  <div class="foot" id="fw">ESP32 relay controller</div>
</div>

<dialog id="editor">
  <h2 id="editorTitle">Edit schedule</h2>
  <div class="form">
    <label>Start time<input id="startTime" type="time"></label>
    <label>End time<input id="endTime" type="time"></label>
  </div>
  <label class="check"><input id="scheduleEnabled" type="checkbox"> Enable schedule</label>
  <div class="actions">
    <button onclick="closeEditor()">Cancel</button>
    <button class="save" onclick="saveSchedule()">Save</button>
  </div>
</dialog>

<script>
let state=null;
let editingRelay=0;

function modeName(v){
  if(v===1)return 'ON';
  if(v===2)return 'OFF';
  return 'AUTO';
}

function render(){
  if(!state)return;
  document.getElementById('clock').textContent=state.time;
  document.getElementById('date').textContent=state.date;
  document.getElementById('temp').textContent=state.temperature.toFixed(1)+' °C';
  document.getElementById('fw').textContent=state.device+' • '+state.firmware+' • '+state.ip;

  const grid=document.getElementById('relayGrid');
  grid.innerHTML='';
  state.relays.forEach((r,i)=>{
    const card=document.createElement('section');
    card.className='card';
    const activeClass=r.state?' on':'';
    card.innerHTML=
      '<div class="row">'+
        '<div><div class="title">'+escapeHtml(r.name)+'</div><div class="sub">Relay '+(i+1)+'</div></div>'+
        '<span class="badge'+activeClass+'">'+(r.state?'ON':'OFF')+'</span>'+
      '</div>'+
      '<div class="schedule">'+
        '<div class="label">'+(r.schedule.enabled?'ACTIVE SCHEDULE':'SCHEDULE DISABLED')+'</div>'+
        '<div class="times">'+pad(r.schedule.startHour)+':'+pad(r.schedule.startMinute)+' → '+pad(r.schedule.endHour)+':'+pad(r.schedule.endMinute)+'</div>'+
      '</div>'+
      '<div class="mode">'+
        modeButton(i,0,'AUTO',r.mode)+
        modeButton(i,1,'ON',r.mode)+
        modeButton(i,2,'OFF',r.mode)+
      '</div>'+
      '<button class="edit" onclick="openEditor('+i+')">Edit schedule</button>';
    grid.appendChild(card);
  });
}

function modeButton(i,value,label,current){
  return '<button class="'+(value===current?'active':'')+'" onclick="setMode('+i+','+value+')">'+label+'</button>';
}

function pad(v){return String(v).padStart(2,'0')}
function escapeHtml(s){
  const d=document.createElement('div');d.textContent=s;return d.innerHTML;
}

async function refresh(){
  try{
    const res=await fetch('/api/status',{cache:'no-store'});
    if(!res.ok)throw new Error('status');
    state=await res.json();
    document.getElementById('net').textContent='Connected';
    render();
  }catch(e){
    document.getElementById('net').textContent='Reconnecting';
  }
}

async function setMode(index,mode){
  await fetch('/api/relay/mode',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({relay:index,mode:mode})
  });
  await refresh();
}

function openEditor(index){
  editingRelay=index;
  const r=state.relays[index];
  document.getElementById('editorTitle').textContent='Relay '+(index+1)+' schedule';
  document.getElementById('startTime').value=pad(r.schedule.startHour)+':'+pad(r.schedule.startMinute);
  document.getElementById('endTime').value=pad(r.schedule.endHour)+':'+pad(r.schedule.endMinute);
  document.getElementById('scheduleEnabled').checked=r.schedule.enabled;
  document.getElementById('editor').showModal();
}

function closeEditor(){document.getElementById('editor').close()}

async function saveSchedule(){
  const start=document.getElementById('startTime').value.split(':');
  const end=document.getElementById('endTime').value.split(':');
  const payload={
    relay:editingRelay,
    enabled:document.getElementById('scheduleEnabled').checked,
    startHour:Number(start[0]),
    startMinute:Number(start[1]),
    endHour:Number(end[0]),
    endMinute:Number(end[1])
  };
  await fetch('/api/relay/schedule',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(payload)
  });
  closeEditor();
  await refresh();
}

refresh();
setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";
