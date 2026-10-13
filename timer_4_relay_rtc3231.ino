#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <Preferences.h>

// ============================================================
// School Bell Controller
// Hardware: ESP32 + DS3231 + ONE relay module
// Interface: ESP32 Wi-Fi Access Point + local web dashboard
// No local display or physical control panel.
// ============================================================

// ---------- Hardware ----------
static constexpr uint8_t I2C_SDA_PIN = 21;
static constexpr uint8_t I2C_SCL_PIN = 22;
static constexpr uint8_t RELAY_PIN   = 25;

// Most common relay modules are active LOW.
// If your relay turns ON when GPIO is HIGH, change to false.
static constexpr bool RELAY_ACTIVE_LOW = true;

static constexpr uint8_t RELAY_ON  = RELAY_ACTIVE_LOW ? LOW  : HIGH;
static constexpr uint8_t RELAY_OFF = RELAY_ACTIVE_LOW ? HIGH : LOW;

// ---------- Wi-Fi AP ----------
static const char AP_SSID[]     = "SchoolBell-Setup";
static const char AP_PASSWORD[] = "schoolbell";

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

// ---------- Bell configuration ----------
static constexpr uint8_t EVENT_COUNT = 12;
static constexpr uint8_t DEFAULT_RING_SECONDS = 3;
static constexpr uint8_t MAX_RING_SECONDS = 30;

// Sunday=bit0 ... Saturday=bit6
static constexpr uint8_t DEFAULT_SCHOOL_DAYS = 0x1F; // Sun-Thu

static constexpr uint32_t SETTINGS_MAGIC = 0x53424331; // SBC1
static constexpr uint16_t SETTINGS_VERSION = 1;

struct BellEvent {
  uint16_t minuteOfDay;
  uint8_t daysMask;
  uint8_t durationSeconds;
  bool enabled;
};

struct StoredSettings {
  uint32_t magic;
  uint16_t version;
  bool automationEnabled;
  BellEvent events[EVENT_COUNT];
};

RTC_DS3231 rtc;
Preferences prefs;
WebServer server(80);
DNSServer dnsServer;

StoredSettings settings;

bool rtcDetected = false;
bool rtcValid = false;
bool bellRinging = false;
bool apReady = false;

unsigned long bellOffAt = 0;
uint32_t lastTriggeredStamp = 0;

// ============================================================
// Utility
// ============================================================

String twoDigits(uint8_t value) {
  return value < 10 ? "0" + String(value) : String(value);
}

String minuteText(uint16_t minuteOfDay) {
  return twoDigits(minuteOfDay / 60) + ":" + twoDigits(minuteOfDay % 60);
}

bool isLeapYear(uint16_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t DAYS[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return DAYS[month - 1];
}

bool validRtcDate(const DateTime& now) {
  return now.year() >= 2024 && now.year() <= 2099 &&
         now.month() >= 1 && now.month() <= 12 &&
         now.day() >= 1 && now.day() <= daysInMonth(now.year(), now.month());
}

uint32_t triggerStamp(const DateTime& now) {
  uint32_t dateKey =
      ((uint32_t)now.year() * 13UL + now.month()) * 32UL + now.day();
  return dateKey * 1440UL + (uint32_t)now.hour() * 60UL + now.minute();
}

String nowText() {
  if (!rtcValid) return "RTC not ready";
  DateTime now = rtc.now();
  return twoDigits(now.hour()) + ":" +
         twoDigits(now.minute()) + ":" +
         twoDigits(now.second());
}

bool dayEnabled(uint8_t mask, uint8_t dayOfWeek) {
  return dayOfWeek < 7 && (mask & (1U << dayOfWeek));
}

// ============================================================
// Relay
// ============================================================

void forceRelayOff() {
  digitalWrite(RELAY_PIN, RELAY_OFF);
  bellRinging = false;
}

void startBell(uint8_t durationSeconds, const char* reason) {
  if (durationSeconds < 1) durationSeconds = 1;
  if (durationSeconds > MAX_RING_SECONDS) durationSeconds = MAX_RING_SECONDS;

  digitalWrite(RELAY_PIN, RELAY_ON);
  bellRinging = true;
  bellOffAt = millis() + (unsigned long)durationSeconds * 1000UL;

  Serial.print("Bell ON: ");
  Serial.print(reason);
  Serial.print(" for ");
  Serial.print(durationSeconds);
  Serial.println("s");
}

void stopBell() {
  forceRelayOff();
  Serial.println("Bell OFF");
}

void serviceBellTimeout() {
  if (!bellRinging) return;

  if ((int32_t)(millis() - bellOffAt) >= 0) {
    stopBell();
  }
}

// ============================================================
// Settings
// ============================================================

void setFactoryDefaults() {
  settings.magic = SETTINGS_MAGIC;
  settings.version = SETTINGS_VERSION;
  settings.automationEnabled = true;

  const uint16_t defaultTimes[EVENT_COUNT] = {
    7 * 60 + 45,
    8 * 60,
    9 * 60,
    10 * 60,
    10 * 60 + 10,
    11 * 60,
    12 * 60,
    12 * 60 + 45,
    13 * 60,
    14 * 60,
    15 * 60,
    16 * 60
  };

  for (uint8_t i = 0; i < EVENT_COUNT; ++i) {
    settings.events[i].minuteOfDay = defaultTimes[i];
    settings.events[i].daysMask = DEFAULT_SCHOOL_DAYS;
    settings.events[i].durationSeconds = DEFAULT_RING_SECONDS;
    settings.events[i].enabled = true;
  }
}

bool settingsValid() {
  if (settings.magic != SETTINGS_MAGIC) return false;
  if (settings.version != SETTINGS_VERSION) return false;

  for (uint8_t i = 0; i < EVENT_COUNT; ++i) {
    const BellEvent& e = settings.events[i];
    if (e.minuteOfDay >= 1440) return false;
    if (e.durationSeconds < 1 || e.durationSeconds > MAX_RING_SECONDS) return false;
    if ((e.daysMask & 0x80) != 0) return false;
  }

  return true;
}

void saveSettings() {
  prefs.putBytes("settings", &settings, sizeof(settings));
}

void loadSettings() {
  prefs.begin("schoolbell", false);

  size_t stored = prefs.getBytesLength("settings");

  if (stored != sizeof(settings)) {
    setFactoryDefaults();
    saveSettings();
    return;
  }

  prefs.getBytes("settings", &settings, sizeof(settings));

  if (!settingsValid()) {
    setFactoryDefaults();
    saveSettings();
  }

  lastTriggeredStamp = prefs.getUInt("laststamp", 0);
}

void rememberTriggeredMinute(uint32_t stamp) {
  lastTriggeredStamp = stamp;
  prefs.putUInt("laststamp", stamp);
}

void suppressCurrentMinute() {
  if (!rtcValid) return;
  rememberTriggeredMinute(triggerStamp(rtc.now()));
}

// ============================================================
// Scheduler
// ============================================================

void serviceScheduler() {
  if (!settings.automationEnabled) return;
  if (!rtcValid) return;

  DateTime now = rtc.now();

  // Only trigger during the first 5 seconds of the minute.
  // Prevents late ringing after a reboot at e.g. 10:00:45.
  if (now.second() > 5) return;

  uint16_t currentMinute = (uint16_t)now.hour() * 60U + now.minute();
  uint32_t stamp = triggerStamp(now);

  // Prevent duplicate ringing during the same minute, even after reset.
  if (stamp == lastTriggeredStamp) return;

  for (uint8_t i = 0; i < EVENT_COUNT; ++i) {
    const BellEvent& e = settings.events[i];

    if (!e.enabled) continue;
    if (e.minuteOfDay != currentMinute) continue;
    if (!dayEnabled(e.daysMask, now.dayOfTheWeek())) continue;

    rememberTriggeredMinute(stamp);
    startBell(e.durationSeconds, "scheduled event");
    return;
  }
}

// ============================================================
// JSON helpers
// ============================================================

String boolJson(bool value) {
  return value ? "true" : "false";
}

String statusJson() {
  String out;
  out.reserve(2200);

  out += "{";
  out += "\"rtcDetected\":" + boolJson(rtcDetected) + ",";
  out += "\"rtcValid\":" + boolJson(rtcValid) + ",";
  out += "\"bellRinging\":" + boolJson(bellRinging) + ",";
  out += "\"automationEnabled\":" + boolJson(settings.automationEnabled) + ",";
  out += "\"ssid\":\"" + String(AP_SSID) + "\",";
  out += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  out += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";

  if (rtcValid) {
    DateTime now = rtc.now();
    out += "\"time\":\"" + twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second()) + "\",";
    out += "\"date\":\"" + twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year()) + "\",";
    out += "\"dayOfWeek\":" + String(now.dayOfTheWeek()) + ",";
    out += "\"minuteOfDay\":" + String((uint16_t)now.hour() * 60U + now.minute()) + ",";
  } else {
    out += "\"time\":\"--:--:--\",";
    out += "\"date\":\"RTC needs synchronization\",";
    out += "\"dayOfWeek\":0,";
    out += "\"minuteOfDay\":0,";
  }

  out += "\"events\":[";
  for (uint8_t i = 0; i < EVENT_COUNT; ++i) {
    if (i) out += ",";

    const BellEvent& e = settings.events[i];
    out += "{";
    out += "\"slot\":" + String(i) + ",";
    out += "\"time\":\"" + minuteText(e.minuteOfDay) + "\",";
    out += "\"minuteOfDay\":" + String(e.minuteOfDay) + ",";
    out += "\"daysMask\":" + String(e.daysMask) + ",";
    out += "\"durationSeconds\":" + String(e.durationSeconds) + ",";
    out += "\"enabled\":" + boolJson(e.enabled);
    out += "}";
  }
  out += "]}";

  return out;
}

// ============================================================
// API
// ============================================================

void sendOk() {
  server.send(200, "application/json", "{\"ok\":true}");
}

bool requireArg(const char* name) {
  if (server.hasArg(name)) return true;
  server.send(400, "application/json", "{\"error\":\"missing_parameter\"}");
  return false;
}

void setupApi() {
  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", statusJson());
  });

  server.on("/api/ring", HTTP_POST, []() {
    uint8_t duration = DEFAULT_RING_SECONDS;

    if (server.hasArg("duration")) {
      int value = server.arg("duration").toInt();
      if (value < 1 || value > MAX_RING_SECONDS) {
        server.send(400, "application/json", "{\"error\":\"invalid_duration\"}");
        return;
      }
      duration = (uint8_t)value;
    }

    startBell(duration, "manual web");
    sendOk();
  });

  server.on("/api/stop", HTTP_POST, []() {
    stopBell();
    sendOk();
  });

  server.on("/api/automation", HTTP_POST, []() {
    if (!requireArg("enabled")) return;

    String value = server.arg("enabled");
    if (value != "0" && value != "1") {
      server.send(400, "application/json", "{\"error\":\"invalid_enabled\"}");
      return;
    }

    settings.automationEnabled = (value == "1");
    saveSettings();

    if (settings.automationEnabled) {
      suppressCurrentMinute();
    }

    sendOk();
  });

  server.on("/api/event", HTTP_POST, []() {
    if (!requireArg("slot")) return;
    if (!requireArg("minute")) return;
    if (!requireArg("days")) return;
    if (!requireArg("duration")) return;
    if (!requireArg("enabled")) return;

    int slot = server.arg("slot").toInt();
    int minute = server.arg("minute").toInt();
    int days = server.arg("days").toInt();
    int duration = server.arg("duration").toInt();
    int enabled = server.arg("enabled").toInt();

    if (slot < 0 || slot >= EVENT_COUNT ||
        minute < 0 || minute >= 1440 ||
        days < 0 || days > 127 ||
        duration < 1 || duration > MAX_RING_SECONDS ||
        (enabled != 0 && enabled != 1)) {
      server.send(400, "application/json", "{\"error\":\"invalid_event\"}");
      return;
    }

    // Do not allow two enabled events at the same minute
    // on overlapping weekdays.
    if (enabled) {
      for (uint8_t i = 0; i < EVENT_COUNT; ++i) {
        if (i == slot) continue;

        const BellEvent& other = settings.events[i];
        if (!other.enabled) continue;

        if (other.minuteOfDay == minute &&
            (other.daysMask & (uint8_t)days) != 0) {
          server.send(409, "application/json", "{\"error\":\"duplicate_event\"}");
          return;
        }
      }
    }

    BellEvent& e = settings.events[slot];
    e.minuteOfDay = (uint16_t)minute;
    e.daysMask = (uint8_t)days;
    e.durationSeconds = (uint8_t)duration;
    e.enabled = enabled;

    saveSettings();
    suppressCurrentMinute();
    sendOk();
  });

  server.on("/api/time", HTTP_POST, []() {
    if (!rtcDetected) {
      server.send(503, "application/json", "{\"error\":\"rtc_not_detected\"}");
      return;
    }

    if (!requireArg("year")) return;
    if (!requireArg("month")) return;
    if (!requireArg("day")) return;
    if (!requireArg("hour")) return;
    if (!requireArg("minute")) return;
    if (!requireArg("second")) return;

    int year = server.arg("year").toInt();
    int month = server.arg("month").toInt();
    int day = server.arg("day").toInt();
    int hour = server.arg("hour").toInt();
    int minute = server.arg("minute").toInt();
    int second = server.arg("second").toInt();

    uint8_t maxDay =
      (year >= 2024 && year <= 2099 && month >= 1 && month <= 12)
      ? daysInMonth(year, month)
      : 0;

    if (year < 2024 || year > 2099 ||
        month < 1 || month > 12 ||
        day < 1 || day > maxDay ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59) {
      server.send(400, "application/json", "{\"error\":\"invalid_time\"}");
      return;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    rtcValid = true;
    suppressCurrentMinute();
    sendOk();
  });

  server.on("/api/factory-reset", HTTP_POST, []() {
    forceRelayOff();
    setFactoryDefaults();
    saveSettings();
    suppressCurrentMinute();
    sendOk();
  });
}

// ============================================================
// Web dashboard
// ============================================================

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>School Bell</title>
<style>
:root{font-family:system-ui,-apple-system,Segoe UI,sans-serif;color:#f4f7f8;background:#0b0f13}
*{box-sizing:border-box}body{margin:0;background:#0b0f13}.app{max-width:900px;margin:auto;padding:18px}
h1{font-size:1.15rem;margin:0}.sub{color:#93a0aa;font-size:.82rem;margin-top:4px}
.card{background:#131920;border:1px solid #29343e;border-radius:16px;padding:16px;margin-top:12px}
.hero{display:grid;grid-template-columns:1fr 1fr;gap:12px}.time{font-size:2.5rem;font-weight:800}
.label{color:#93a0aa;font-size:.72rem}.big{font-size:1.5rem;font-weight:800;margin-top:5px}
.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px}
button{min-height:46px;border:1px solid #2b3640;background:#202831;color:white;border-radius:12px;font-weight:750;padding:10px}
button.primary{background:#f5f7f8;color:#111}.warn{color:#ffd889}.bad{color:#ff9a9a}
.grid{display:grid;gap:8px;margin-top:12px}.row{display:grid;grid-template-columns:80px 1fr auto;gap:10px;align-items:center;background:#131920;border:1px solid #29343e;border-radius:14px;padding:12px}
.days{display:flex;gap:4px;flex-wrap:wrap}.day{font-size:.65rem;background:#202831;color:#7f8b95;padding:4px 6px;border-radius:6px}.day.on{color:white;background:#34414c}
.small{color:#93a0aa;font-size:.75rem;margin-top:5px}.pill{font-size:.68rem;color:#63d680}.edit{min-height:38px}.section{margin-top:20px}
dialog{width:min(92vw,480px);background:#131920;color:white;border:1px solid #29343e;border-radius:16px;padding:18px}
dialog::backdrop{background:#000a}label{display:block;color:#93a0aa;font-size:.78rem;margin:10px 0}
input{width:100%;font-size:16px;padding:11px;background:#0d1216;color:white;border:1px solid #29343e;border-radius:10px}
.week{display:grid;grid-template-columns:repeat(7,1fr);gap:5px;margin:12px 0}.week button{padding:7px 2px;min-height:40px}.week button.on{background:#34414c}
@media(max-width:650px){.hero,.actions{grid-template-columns:1fr}.row{grid-template-columns:70px 1fr}.row>button{grid-column:1/-1}}
</style>
</head>
<body>
<div class="app">
  <h1>School Bell Controller</h1>
  <div class="sub">ESP32 access point · no internet required</div>

  <div class="hero">
    <div class="card">
      <div id="time" class="time">--:--:--</div>
      <div id="date" class="sub">Loading RTC…</div>
      <div id="rtcWarn" class="bad small"></div>
    </div>

    <div class="card">
      <div class="label">NEXT BELL</div>
      <div id="next" class="big">--:--</div>
      <div id="nextMeta" class="small">Checking schedule…</div>
      <div class="small">Relay: <b id="relayState">OFF</b> · Devices: <b id="clients">0</b></div>
    </div>
  </div>

  <div class="actions">
    <button id="ring" class="primary">Ring now · 3 s</button>
    <button id="auto">Pause automation</button>
  </div>

  <div class="section">
    <h2>Bell schedule</h2>
    <div id="schedule" class="grid"></div>
  </div>

  <div class="section card">
    <h2>Device</h2>
    <div class="small">Wi-Fi: SchoolBell-Setup · Dashboard: 192.168.4.1</div>
    <div class="actions">
      <button id="sync">Sync RTC from this phone</button>
      <button id="reset" class="bad">Restore default timetable</button>
    </div>
  </div>
</div>

<dialog id="dlg">
  <h3>Edit bell event</h3>
  <label>Time<input id="editTime" type="time"></label>
  <label>Ring duration (1–30 s)<input id="editDuration" type="number" min="1" max="30"></label>
  <label><input id="editEnabled" type="checkbox" style="width:auto"> Enabled</label>
  <div id="week" class="week"></div>
  <div class="actions">
    <button id="cancel">Cancel</button>
    <button id="save" class="primary">Save</button>
  </div>
</dialog>

<script>
let state=null,slot=-1,days=31;
const dayNames=['Su','Mo','Tu','We','Th','Fr','Sa'];
const $=id=>document.getElementById(id);
const enc=o=>new URLSearchParams(o);

async function post(url,obj={}){
  const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:enc(obj)});
  const t=await r.text();
  if(!r.ok) throw new Error(t);
}

function nextBell(){
  if(!state||!state.rtcValid||!state.automationEnabled)return null;
  let best=null;
  for(let d=0;d<7;d++){
    const dow=(state.dayOfWeek+d)%7;
    for(const e of state.events){
      if(!e.enabled||!(e.daysMask&(1<<dow)))continue;
      if(d===0&&e.minuteOfDay<=state.minuteOfDay)continue;
      const delta=d*1440+e.minuteOfDay-(d===0?state.minuteOfDay:0);
      if(!best||delta<best.delta)best={e,d,dow,delta};
    }
  }
  return best;
}

function render(){
  $('time').textContent=state.time;
  $('date').textContent=state.date;
  $('rtcWarn').textContent=state.rtcValid?'':'RTC not ready — sync it before automatic operation.';
  $('relayState').textContent=state.bellRinging?'RINGING':'OFF';
  $('clients').textContent=state.clients;
  $('ring').textContent=state.bellRinging?'Stop bell':'Ring now · 3 s';
  $('auto').textContent=state.automationEnabled?'Pause automation':'Resume automation';

  const n=nextBell();
  $('next').textContent=n?n.e.time:'--:--';
  $('nextMeta').textContent=n?(n.d===0?'Today':n.d===1?'Tomorrow':dayNames[n.dow])+' · '+n.e.durationSeconds+' s':'No upcoming bell';

  $('schedule').innerHTML=state.events.map((e,i)=>{
    const ds=dayNames.map((d,j)=>'<span class="day '+((e.daysMask&(1<<j))?'on':'')+'">'+d+'</span>').join('');
    return '<div class="row"><b>'+e.time+'</b><div><div class="days">'+ds+'</div><div class="small">'+e.durationSeconds+' s · '+(e.enabled?'<span class="pill">ENABLED</span>':'DISABLED')+'</div></div><button class="edit" data-i="'+i+'">Edit</button></div>';
  }).join('');

  document.querySelectorAll('.edit').forEach(b=>b.onclick=()=>openEdit(+b.dataset.i));
}

async function refresh(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    state=await r.json();
    render();
  }catch(e){}
}

function drawWeek(){
  $('week').innerHTML='';
  dayNames.forEach((d,i)=>{
    const b=document.createElement('button');
    b.textContent=d;
    b.className=(days&(1<<i))?'on':'';
    b.onclick=()=>{days^=(1<<i);drawWeek()};
    $('week').appendChild(b);
  });
}

function openEdit(i){
  const e=state.events[i];
  slot=i;days=e.daysMask;
  $('editTime').value=e.time;
  $('editDuration').value=e.durationSeconds;
  $('editEnabled').checked=e.enabled;
  drawWeek();
  $('dlg').showModal();
}

$('cancel').onclick=()=>$('dlg').close();

$('save').onclick=async()=>{
  const p=$('editTime').value.split(':');
  if(p.length!==2)return;
  const minute=(+p[0])*60+(+p[1]);
  const duration=+$('editDuration').value;
  try{
    await post('/api/event',{slot,minute,days,duration,enabled:$('editEnabled').checked?1:0});
    $('dlg').close();await refresh();
  }catch(e){alert('Save failed');}
};

$('ring').onclick=async()=>{
  try{
    if(state&&state.bellRinging)await post('/api/stop');
    else await post('/api/ring',{duration:3});
    await refresh();
  }catch(e){alert('Bell command failed');}
};

$('auto').onclick=async()=>{
  try{
    await post('/api/automation',{enabled:state.automationEnabled?0:1});
    await refresh();
  }catch(e){alert('Automation command failed');}
};

$('sync').onclick=async()=>{
  const d=new Date();
  try{
    await post('/api/time',{
      year:d.getFullYear(),month:d.getMonth()+1,day:d.getDate(),
      hour:d.getHours(),minute:d.getMinutes(),second:d.getSeconds()
    });
    await refresh();
    alert('RTC synchronized');
  }catch(e){alert('RTC sync failed');}
};

$('reset').onclick=async()=>{
  if(!confirm('Restore default timetable?'))return;
  try{await post('/api/factory-reset');await refresh();}
  catch(e){alert('Reset failed');}
};

refresh();
setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";

void redirectToDashboard() {
  server.sendHeader("Location", "http://192.168.4.1", true);
  server.send(302, "text/plain", "");
}

void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", INDEX_HTML);
  });

  setupApi();

  server.on("/generate_204", HTTP_ANY, redirectToDashboard);
  server.on("/gen_204", HTTP_ANY, redirectToDashboard);
  server.on("/hotspot-detect.html", HTTP_ANY, redirectToDashboard);
  server.on("/ncsi.txt", HTTP_ANY, redirectToDashboard);
  server.on("/connecttest.txt", HTTP_ANY, redirectToDashboard);

  server.onNotFound([]() {
    if (server.uri().startsWith("/api/")) {
      server.send(404, "application/json", "{\"error\":\"not_found\"}");
      return;
    }
    redirectToDashboard();
  });

  server.begin();
}

// ============================================================
// Setup / loop
// ============================================================

void setup() {
  Serial.begin(115200);

  // Fail-safe startup: establish OFF before enabling GPIO output.
  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT);
  forceRelayOff();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setTimeOut(50);

  rtcDetected = rtc.begin();

  if (rtcDetected && !rtc.lostPower()) {
    DateTime now = rtc.now();
    rtcValid = validRtcDate(now);
  } else {
    rtcValid = false;
  }

  loadSettings();

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  apReady = WiFi.softAP(AP_SSID, AP_PASSWORD, 6, false, 4);

  if (apReady) {
    dnsServer.start(53, "*", AP_IP);
  }

  setupWeb();

  Serial.println();
  Serial.println("School Bell Controller");
  Serial.print("Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(AP_IP);
  Serial.print("RTC: ");
  Serial.println(rtcValid ? "ready" : "needs sync");
}

void loop() {
  if (apReady) {
    dnsServer.processNextRequest();
  }

  server.handleClient();
  serviceBellTimeout();
  serviceScheduler();

  delay(2);
}
