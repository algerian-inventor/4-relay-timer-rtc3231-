#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0e11">
<title>Relay Automation Controller</title>
<style>
:root{
  --bg:#0a0d10;--panel:#11161b;--panel2:#171d23;--line:#273039;
  --text:#f5f7f8;--muted:#8f9aa5;--green:#5ad37a;--amber:#e7b54f;
  --red:#ef6b6b;--blue:#79a7ff;--radius:18px;
}
*{box-sizing:border-box}html{background:var(--bg)}body{margin:0;font-family:Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif;color:var(--text);background:radial-gradient(circle at 20% -10%,#182129 0,transparent 35%),var(--bg);min-height:100vh}
button,input{font:inherit}.app{max-width:1180px;margin:auto;padding:18px 16px 48px}
.topbar{display:flex;justify-content:space-between;align-items:center;gap:16px;padding:8px 2px 20px}.identity h1{font-size:1.05rem;margin:0;font-weight:760;letter-spacing:.02em}.identity p{margin:4px 0 0;color:var(--muted);font-size:.78rem}.online{display:flex;align-items:center;gap:8px;color:var(--muted);font-size:.78rem}.dot{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 12px rgba(90,211,122,.6)}
.nav{display:flex;gap:6px;padding:5px;background:#0e1216;border:1px solid var(--line);border-radius:14px;overflow:auto;margin-bottom:16px}.nav button{border:0;background:transparent;color:var(--muted);font-weight:700;padding:10px 14px;border-radius:10px;white-space:nowrap;cursor:pointer}.nav button.active{background:#20272e;color:#fff}
.page{display:none}.page.active{display:block}.hero{display:grid;grid-template-columns:1.1fr .9fr;gap:14px;margin-bottom:14px}.panel{background:rgba(17,22,27,.94);border:1px solid var(--line);border-radius:var(--radius);padding:18px}.clock{font-size:2.5rem;letter-spacing:-.045em;font-weight:780}.date{margin-top:4px;color:var(--muted)}.metrics{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.metric{background:var(--panel2);border:1px solid var(--line);border-radius:14px;padding:14px}.metric b{display:block;font-size:1.1rem}.metric span{color:var(--muted);font-size:.72rem}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.channel{background:rgba(17,22,27,.95);border:1px solid var(--line);border-radius:var(--radius);padding:18px}.row{display:flex;justify-content:space-between;align-items:center;gap:12px}.eyebrow{color:var(--muted);font-size:.68rem;font-weight:800;letter-spacing:.1em}.channel h3{margin:4px 0 0;font-size:1.02rem}.state{font-size:.72rem;font-weight:850;padding:7px 10px;border-radius:999px;background:#222931;color:var(--muted)}.state.on{background:rgba(90,211,122,.13);color:var(--green)}.mode{display:grid;grid-template-columns:repeat(3,1fr);gap:4px;margin-top:15px;padding:4px;background:#0d1114;border:1px solid var(--line);border-radius:12px}.mode button{border:0;border-radius:9px;background:transparent;color:var(--muted);font-weight:750;padding:9px;cursor:pointer}.mode button.active{background:#252d34;color:#fff}.next{margin-top:14px;padding:12px;border:1px solid var(--line);border-radius:13px;background:var(--panel2)}.next span{display:block;color:var(--muted);font-size:.7rem;margin-bottom:4px}.next b{font-size:.92rem}.smallbtn{margin-top:10px;width:100%;border:1px solid var(--line);background:#20272e;color:#fff;padding:10px;border-radius:11px;font-weight:700;cursor:pointer}
.section-head{display:flex;justify-content:space-between;align-items:end;gap:12px;margin:4px 0 12px}.section-head h2{margin:0;font-size:1.15rem}.section-head p{margin:4px 0 0;color:var(--muted);font-size:.8rem}
.schedule-card{margin-bottom:10px;background:var(--panel);border:1px solid var(--line);border-radius:15px;padding:14px}.schedule-grid{display:grid;grid-template-columns:160px 1fr auto;gap:12px;align-items:center}.times{font-weight:780}.days{display:flex;gap:5px;flex-wrap:wrap}.day{font-size:.66rem;padding:5px 7px;border-radius:7px;background:#20262c;color:var(--muted)}.day.on{color:#fff;background:#303943}.tag{font-size:.68rem;padding:6px 8px;border-radius:999px;background:#222931;color:var(--muted)}.tag.on{color:var(--green);background:rgba(90,211,122,.12)}
.events{display:flex;flex-direction:column;gap:8px}.event{display:grid;grid-template-columns:90px 1fr;gap:12px;border:1px solid var(--line);background:var(--panel);border-radius:13px;padding:12px}.event time{color:var(--muted);font-size:.75rem}
.settings-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.setting h3{margin:0 0 5px;font-size:.95rem}.setting p{margin:0 0 12px;color:var(--muted);font-size:.78rem}.danger{border-color:rgba(239,107,107,.35)}.danger button{background:rgba(239,107,107,.12);color:#ff9b9b}
dialog{width:min(92vw,520px);border:1px solid var(--line);border-radius:18px;background:#12171c;color:#fff;padding:18px}dialog::backdrop{background:rgba(0,0,0,.72);backdrop-filter:blur(5px)}dialog h2{margin:0 0 14px;font-size:1.05rem}.form{display:grid;grid-template-columns:1fr 1fr;gap:10px}label{display:flex;flex-direction:column;gap:6px;color:var(--muted);font-size:.75rem}input[type=text],input[type=time]{width:100%;background:#0d1114;color:#fff;border:1px solid var(--line);border-radius:11px;padding:11px}.check{display:flex;flex-direction:row;align-items:center;gap:8px;color:#fff;margin:14px 0}.week{display:grid;grid-template-columns:repeat(7,1fr);gap:5px;margin:12px 0}.week button{border:1px solid var(--line);background:#171d23;color:var(--muted);border-radius:9px;padding:8px 3px;font-size:.68rem;cursor:pointer}.week button.on{background:#303943;color:#fff}.actions{display:flex;gap:8px;margin-top:14px}.actions button{flex:1;border:1px solid var(--line);border-radius:11px;padding:10px;background:#20272e;color:#fff;font-weight:750;cursor:pointer}.actions .primary{background:#fff;color:#111;border-color:#fff}.muted{color:var(--muted)}.empty{padding:24px;text-align:center;color:var(--muted);border:1px dashed var(--line);border-radius:14px}
@media(max-width:760px){.hero,.grid,.settings-grid{grid-template-columns:1fr}.metrics{grid-template-columns:repeat(3,1fr)}.schedule-grid{grid-template-columns:1fr}.clock{font-size:2.1rem}.event{grid-template-columns:72px 1fr}.app{padding:12px 12px 36px}}
</style>
</head>
<body>
<div class="app">
  <header class="topbar">
    <div class="identity"><h1>Relay Automation Controller</h1><p>ESP32 • Local control system</p></div>
    <div class="online"><span class="dot"></span><span id="net">Connected</span></div>
  </header>

  <nav class="nav">
    <button class="active" data-page="overview">Overview</button>
    <button data-page="schedules">Schedules</button>
    <button data-page="history">History</button>
    <button data-page="device">Device</button>
  </nav>

  <section id="overview" class="page active">
    <div class="hero">
      <div class="panel"><div class="clock" id="clock">--:--:--</div><div class="date" id="date">Loading RTC…</div></div>
      <div class="metrics">
        <div class="metric"><b id="temp">--.-°</b><span>RTC temperature</span></div>
        <div class="metric"><b id="clients">0</b><span>Connected clients</span></div>
        <div class="metric"><b id="uptime">0m</b><span>Uptime</span></div>
      </div>
    </div>
    <div class="grid" id="channels"></div>
  </section>

  <section id="schedules" class="page">
    <div class="section-head"><div><h2>Schedules</h2><p>Up to six rules per channel, with weekday control.</p></div></div>
    <div id="scheduleList"></div>
  </section>

  <section id="history" class="page">
    <div class="section-head"><div><h2>History</h2><p>Recent runtime events stored in memory.</p></div><button class="smallbtn" style="width:auto;margin:0" onclick="loadEvents()">Refresh</button></div>
    <div id="events" class="events"></div>
  </section>

  <section id="device" class="page">
    <div class="settings-grid">
      <div class="panel setting"><h3>RTC synchronization</h3><p>Set the DS3231 from the current time on this phone or computer.</p><button class="smallbtn" onclick="syncTime()">Sync from this device</button></div>
      <div class="panel setting"><h3>Network</h3><p id="networkInfo">Loading…</p></div>
      <div class="panel setting"><h3>Firmware</h3><p id="firmwareInfo">Loading…</p></div>
      <div class="panel setting danger"><h3>Factory reset</h3><p>Restore channel names, schedules and modes to defaults.</p><button class="smallbtn" onclick="factoryReset()">Restore defaults</button></div>
    </div>
  </section>
</div>

<dialog id="nameDialog">
  <h2>Rename channel</h2>
  <label>Channel name<input id="channelName" type="text" maxlength="23"></label>
  <div class="actions"><button onclick="nameDialog.close()">Cancel</button><button class="primary" onclick="saveName()">Save</button></div>
</dialog>

<dialog id="scheduleDialog">
  <h2 id="scheduleTitle">Edit schedule</h2>
  <div class="form"><label>Start<input id="startTime" type="time"></label><label>End<input id="endTime" type="time"></label></div>
  <label class="check"><input id="enabled" type="checkbox"> Enabled</label>
  <div class="week" id="week"></div>
  <div class="actions"><button onclick="scheduleDialog.close()">Cancel</button><button class="primary" onclick="saveSchedule()">Save</button></div>
</dialog>

<script>
let state=null, editChannel=0, editSlot=0, editDays=127;
const dayNames=['Su','Mo','Tu','We','Th','Fr','Sa'];

document.querySelectorAll('.nav button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('.nav button').forEach(x=>x.classList.remove('active'));
  document.querySelectorAll('.page').forEach(x=>x.classList.remove('active'));
  b.classList.add('active'); document.getElementById(b.dataset.page).classList.add('active');
  if(b.dataset.page==='history') loadEvents();
});

const pad=n=>String(n).padStart(2,'0');
const timeText=m=>pad(Math.floor(m/60))+':'+pad(m%60);
const esc=s=>{const d=document.createElement('div');d.textContent=s;return d.innerHTML};

function render(){
  if(!state)return;
  clock.textContent=state.time; date.textContent=state.date+(state.rtcReady?'':' • RTC fault');
  temp.textContent=Number(state.temperature).toFixed(1)+'°'; clients.textContent=state.clients;
  uptime.textContent=Math.floor(state.uptimeSec/60)+'m';
  networkInfo.textContent=state.ssid+' • '+state.ip+' • '+state.clients+' client(s)';
  firmwareInfo.textContent=state.firmware+' • settings schema '+state.schema;

  channels.innerHTML='';
  state.channels.forEach((c,i)=>{
    const activeSchedules=c.schedules.filter(s=>s.enabled).length;
    const el=document.createElement('article'); el.className='channel';
    el.innerHTML='<div class="row"><div><div class="eyebrow">CHANNEL '+pad(i+1)+'</div><h3>'+esc(c.name)+'</h3></div><span class="state '+(c.state?'on':'')+'">'+(c.state?'RUNNING':'STOPPED')+'</span></div>'+
      '<div class="next"><span>AUTOMATION</span><b>'+activeSchedules+' active schedule'+(activeSchedules===1?'':'s')+' • '+c.modeName+'</b></div>'+
      '<div class="mode">'+modeBtn(i,0,'AUTO',c.mode)+modeBtn(i,1,'ON',c.mode)+modeBtn(i,2,'OFF',c.mode)+'</div>'+
      '<button class="smallbtn" onclick="renameChannel('+i+')">Rename channel</button>';
    channels.appendChild(el);
  });

  renderSchedules();
}

function modeBtn(i,m,label,current){return '<button class="'+(m===current?'active':'')+'" onclick="setMode('+i+','+m+')">'+label+'</button>'}

function renderSchedules(){
  scheduleList.innerHTML='';
  state.channels.forEach((c,ci)=>{
    const wrap=document.createElement('div');
    wrap.innerHTML='<div class="section-head"><div><h2 style="font-size:1rem">'+esc(c.name)+'</h2><p>Channel '+(ci+1)+'</p></div></div>';
    c.schedules.forEach((s,si)=>{
      const row=document.createElement('div');row.className='schedule-card';
      let days=''; dayNames.forEach((d,di)=>days+='<span class="day '+((s.daysMask&(1<<di))?'on':'')+'">'+d+'</span>');
      row.innerHTML='<div class="schedule-grid"><div><div class="eyebrow">RULE '+pad(si+1)+'</div><div class="times">'+s.start+' → '+s.end+'</div></div><div class="days">'+days+'</div><div><span class="tag '+(s.enabled?'on':'')+'">'+(s.enabled?'ENABLED':'DISABLED')+'</span><button class="smallbtn" onclick="editSchedule('+ci+','+si+')">Edit</button></div></div>';
      wrap.appendChild(row);
    });
    scheduleList.appendChild(wrap);
  });
}

async function refresh(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'}); if(!r.ok)throw 0;
    state=await r.json(); net.textContent='Connected'; render();
  }catch(e){net.textContent='Reconnecting';}
}
async function post(url,data){const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});if(!r.ok)throw new Error(await r.text());return r.json()}
async function setMode(channel,mode){await post('/api/channel/mode',{channel,mode});await refresh()}
function renameChannel(i){editChannel=i;channelName.value=state.channels[i].name;nameDialog.showModal()}
async function saveName(){await post('/api/channel/name',{channel:editChannel,name:channelName.value.trim()});nameDialog.close();await refresh()}
function editSchedule(c,s){editChannel=c;editSlot=s;const x=state.channels[c].schedules[s];editDays=x.daysMask;startTime.value=x.start;endTime.value=x.end;enabled.checked=x.enabled;scheduleTitle.textContent=state.channels[c].name+' • rule '+(s+1);renderWeek();scheduleDialog.showModal()}
function renderWeek(){week.innerHTML='';dayNames.forEach((d,i)=>{const b=document.createElement('button');b.textContent=d;b.className=(editDays&(1<<i))?'on':'';b.onclick=()=>{editDays^=(1<<i);renderWeek()};week.appendChild(b)})}
async function saveSchedule(){const a=startTime.value.split(':'),b=endTime.value.split(':');await post('/api/schedule',{channel:editChannel,slot:editSlot,enabled:enabled.checked,daysMask:editDays,startMinute:(+a[0])*60+(+a[1]),endMinute:(+b[0])*60+(+b[1])});scheduleDialog.close();await refresh()}
async function syncTime(){const d=new Date();await post('/api/time',{year:d.getFullYear(),month:d.getMonth()+1,day:d.getDate(),hour:d.getHours(),minute:d.getMinutes(),second:d.getSeconds()});await refresh();alert('RTC synchronized')}
async function loadEvents(){try{const r=await fetch('/api/events',{cache:'no-store'}),d=await r.json();events.innerHTML=d.events.length?d.events.map(e=>'<div class="event"><time>'+esc(e.time)+'</time><div>'+esc(e.message)+'</div></div>').join(''):'<div class="empty">No events yet.</div>'}catch(e){events.innerHTML='<div class="empty">Could not load history.</div>'}}
async function factoryReset(){if(!confirm('Restore all relay settings to defaults?'))return;await post('/api/factory-reset',{confirm:true});await refresh();alert('Factory defaults restored')}

refresh();setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";
