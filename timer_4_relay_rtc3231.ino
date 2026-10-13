#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

static constexpr size_t EEPROM_SIZE = 64;
static constexpr int EEPROM_MAGIC_ADDR = 63;
static constexpr uint8_t EEPROM_MAGIC_VALUE = 0xA5;

// ESP32-WROOM safe pin map
static constexpr uint8_t I2C_SDA_PIN = 21;
static constexpr uint8_t I2C_SCL_PIN = 22;

// Set true if your relay module is active-LOW.
static constexpr bool RELAY_ACTIVE_LOW = false;
static constexpr uint8_t RELAY_ON  = RELAY_ACTIVE_LOW ? LOW : HIGH;
static constexpr uint8_t RELAY_OFF = RELAY_ACTIVE_LOW ? HIGH : LOW;

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define bt_clock  13
#define bt_up     14
#define bt_down   16
#define bt_timer  17
#define bt_back   18

#define relay1 25
#define relay2 26
#define relay3 27
#define relay4 33
#define buzzer 19

DateTime now;
int hh, mm, ss, dd, bb, set_day = 1;
int yy;
int StartHH, StartMM, FinishHH, FinishMM;
int Start1HH, Start1MM, Finish1HH, Finish1MM;
int Start2HH, Start2MM, Finish2HH, Finish2MM;
int Start3HH, Start3MM, Finish3HH, Finish3MM;
int Start4HH, Start4MM, Finish4HH, Finish4MM;


uint8_t timer1 = 0, timer2 = 0, timer3 = 0, timer4 = 0;
int mode = 0, setMode = 0, setAlarm = 0, alarmMode = 1;
bool clockPressed = false, timerPressed = false, upPressed = false, downPressed = false;

unsigned long lastBlinkTime = 0;
bool blinkState = true;

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  rtc.begin();

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM init failed");
  }
  lcd.init();
  lcd.backlight();
  pinMode(bt_clock, INPUT_PULLUP);
  pinMode(bt_up, INPUT_PULLUP); 
  pinMode(bt_down, INPUT_PULLUP);
  pinMode(bt_timer, INPUT_PULLUP); 
  pinMode(bt_back, INPUT_PULLUP);
  pinMode(relay1, OUTPUT); 
  pinMode(relay2, OUTPUT); 
  pinMode(relay3, OUTPUT); 
  pinMode(relay4, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(relay1, RELAY_OFF);
   digitalWrite(relay2, RELAY_OFF);
   digitalWrite(relay3, RELAY_OFF);    
   digitalWrite(relay4, RELAY_OFF);
  lcd.setCursor(0, 0); lcd.print(" relay timmer ");
  lcd.setCursor(0, 1); lcd.print(" starting ... ");
  delay(2000); lcd.clear();

  loadRelaySettings();
}

void loop() {
  now = rtc.now();
  if (setMode == 0) { hh = now.hour(); mm = now.minute(); ss = now.second(); dd = now.day(); bb = now.month(); yy = now.year(); }

  if (setAlarm == 0) {
    lcd.setCursor(0, 0); lcd.print((hh / 10) % 10); lcd.print(hh % 10); lcd.print(":");
    lcd.print((mm / 10) % 10); lcd.print(mm % 10); lcd.print(":"); lcd.print((ss / 10) % 10); lcd.print(ss % 10);
    lcd.print(" T:"); lcd.print(rtc.getTemperature()); lcd.print((char)223); lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Date ");
     lcd.setCursor(6, 1);
     if (dd < 10) lcd.print("0");

    lcd.print(dd);
    lcd.setCursor(8, 1);
     lcd.print("/");
     if (bb < 10) lcd.print("0");
     
    lcd.print(bb);
    lcd.setCursor(11, 1);
     lcd.print("/");
     lcd.setCursor(12, 1);
    lcd.print(yy);
  }

  handleButtons();
  setTimer();
  blinking();
  delay(100);

  digitalWrite(relay1, (timer1 && alarmMode && inRange(hh, mm, Start1HH, Start1MM, Finish1HH, Finish1MM)) ? RELAY_ON : RELAY_OFF);
  digitalWrite(relay2, (timer2 && alarmMode && inRange(hh, mm, Start2HH, Start2MM, Finish2HH, Finish2MM)) ? RELAY_ON : RELAY_OFF);
  digitalWrite(relay3, (timer3 && alarmMode && inRange(hh, mm, Start3HH, Start3MM, Finish3HH, Finish3MM)) ? RELAY_ON : RELAY_OFF);
  digitalWrite(relay4, (timer4 && alarmMode && inRange(hh, mm, Start4HH, Start4MM, Finish4HH, Finish4MM)) ? RELAY_ON : RELAY_OFF);
}

bool inRange(int nowH, int nowM, int startH, int startM, int endH, int endM) {
  int nowMin = nowH * 60 + nowM;
  int startMin = startH * 60 + startM;
  int endMin = endH * 60 + endM;

  if (startMin < endMin) return (nowMin >= startMin && nowMin < endMin);
  else if (startMin > endMin) return (nowMin >= startMin || nowMin < endMin);
  else return false;
}
void handleButtons() {
  if (digitalRead(bt_clock) == LOW && !clockPressed) {
    clockPressed = true;

    if (!(setAlarm > 0 && mode > 0)) {
      setMode++;
      if (setMode > 8) setMode = 1;
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
    }
  }
  if (digitalRead(bt_clock) == HIGH) clockPressed = false;

  if (digitalRead(bt_timer) == LOW && !timerPressed) {
    timerPressed = true;

    if (setAlarm >= 1 && setAlarm <= 4 && mode >= 1 && mode <= 4) {
    if (mode == 1) { Start1HH = StartHH; Start1MM = StartMM; Finish1HH = FinishHH; Finish1MM = FinishMM; }
    if (mode == 2) { Start2HH = StartHH; Start2MM = StartMM; Finish2HH = FinishHH; Finish2MM = FinishMM; }
    if (mode == 3) { Start3HH = StartHH; Start3MM = StartMM; Finish3HH = FinishHH; Finish3MM = FinishMM; }
    if (mode == 4) { Start4HH = StartHH; Start4MM = StartMM; Finish4HH = FinishHH; Finish4MM = FinishMM; }
    
    saveRelaySettings();
}

    if (setMode > 0) {
      setMode = 8;
    } else {
      setAlarm++;
      if (setAlarm > 4) {
        setAlarm = 1;
        mode++;
        if (mode > 4) mode = 1;
      }

      if (mode == 1) { StartHH = Start1HH; StartMM = Start1MM; FinishHH = Finish1HH; FinishMM = Finish1MM; }
      if (mode == 2) { StartHH = Start2HH; StartMM = Start2MM; FinishHH = Finish2HH; FinishMM = Finish2MM; }
      if (mode == 3) { StartHH = Start3HH; StartMM = Start3MM; FinishHH = Finish3HH; FinishMM = Finish3MM; }
      if (mode == 4) { StartHH = Start4HH; StartMM = Start4MM; FinishHH = Finish4HH; FinishMM = Finish4MM; }
    }
    lcd.clear();
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
  }
  if (digitalRead(bt_timer) == HIGH) timerPressed = false;

  if (setMode == 8) {
    rtc.adjust(DateTime(yy, bb, dd, hh, mm, ss));
    lcd.clear(); setMode = 0;
  }

  if (digitalRead(bt_up) == LOW && !upPressed) {
    upPressed = true;
    if (setAlarm < 2 && setMode == 1) hh++;
    if (setAlarm < 2 && setMode == 2) mm++;
    if (setAlarm < 2 && setMode == 3) ss++;
    if (setAlarm < 2 && setMode == 4) set_day++;
    if (setAlarm < 2 && setMode == 5) dd++;
    if (setAlarm < 2 && setMode == 6) bb++;
    if (setAlarm < 2 && setMode == 7) yy++;

    if (mode == 0 && setAlarm > 0) {
      if (setAlarm == 1) timer1 = 1;
      if (setAlarm == 2) timer2 = 1;
      if (setAlarm == 3) timer3 = 1;
      if (setAlarm == 4) timer4 = 1;
    }
    if (mode > 0 && setAlarm > 0) {
      if (setAlarm == 1) StartHH++;
      if (setAlarm == 2) StartMM++;
      if (setAlarm == 3) FinishHH++;
      if (setAlarm == 4) FinishMM++;
    }
    boundsCheck(); 
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
  }
  if (digitalRead(bt_up) == HIGH) upPressed = false;

  if (digitalRead(bt_down) == LOW && !downPressed) {
    downPressed = true;
    if (setAlarm < 2 && setMode == 1) hh--;
    if (setAlarm < 2 && setMode == 2) mm--;
    if (setAlarm < 2 && setMode == 3) ss--;
    if (setAlarm < 2 && setMode == 4) set_day--;
    if (setAlarm < 2 && setMode == 5) dd--;
    if (setAlarm < 2 && setMode == 6) bb--;
    if (setAlarm < 2 && setMode == 7) yy--;

    if (mode == 0 && setAlarm > 0) {
      if (setAlarm == 1) timer1 = 0;
      if (setAlarm == 2) timer2 = 0;
      if (setAlarm == 3) timer3 = 0;
      if (setAlarm == 4) timer4 = 0;
    }
    if (mode > 0 && setAlarm > 0) {
      if (setAlarm == 1) StartHH--;
      if (setAlarm == 2) StartMM--;
      if (setAlarm == 3) FinishHH--;
      if (setAlarm == 4) FinishMM--;
    }
    boundsCheck();
     digitalWrite(buzzer, HIGH);
     delay(100);
     digitalWrite(buzzer, LOW);
  }
  if (digitalRead(bt_down) == HIGH) downPressed = false;

  if (digitalRead(bt_back) == LOW) {
    digitalWrite(buzzer, HIGH);
    delay(200); 
    digitalWrite(buzzer, LOW);
    setMode = 0; setAlarm = 0; mode = 0; alarmMode = 1; lcd.clear();
  }
}

void boundsCheck() {
  if (hh > 23) hh = 0;
  if (hh < 0) hh = 23;

  if (mm > 59) mm = 0;
  if (mm < 0) mm = 59;

  if (ss > 59) ss = 0;
  if (ss < 0) ss = 59;

  if (set_day > 7) set_day = 0;
  if (set_day < 0) set_day = 7;

  if (dd > 31) dd = 1;
  if (dd < 1) dd = 31;

  if (bb > 12) bb = 1;
  if (bb < 1) bb = 12;

  if (yy < 2020) yy = 2024;
  if (yy > 2099) yy = 2024;

  if (StartHH > 23) StartHH = 0;
  if (StartHH < 0) StartHH = 23;

  if (StartMM > 59) StartMM = 0;
  if (StartMM < 0) StartMM = 59;

  if (FinishHH > 23) FinishHH = 0;
  if (FinishHH < 0) FinishHH = 23;

  if (FinishMM > 59) FinishMM = 0;
  if (FinishMM < 0) FinishMM = 59;
}



void setTimer() {
  if (setMode == 0 && setAlarm > 0 && mode > 0) {
    lcd.setCursor(0, 0);
    lcd.print("Load "); lcd.print(mode); lcd.print(" On : ");
    lcd.setCursor(11, 0); lcd.print(StartHH / 10); lcd.print(StartHH % 10);
    lcd.print(":"); lcd.print(StartMM / 10); lcd.print(StartMM % 10);

    lcd.setCursor(0, 1);
    lcd.print("Load "); lcd.print(mode); lcd.print(" Off:");
    lcd.setCursor(11, 1); lcd.print(FinishHH / 10); lcd.print(FinishHH % 10);
    lcd.print(":"); lcd.print(FinishMM / 10); lcd.print(FinishMM % 10);
  }
  if (setMode == 0 && setAlarm > 0 && mode == 0) {
    lcd.setCursor(0, 0); lcd.print("L1 L2 L3 L4 ");
    lcd.setCursor(0, 1);
    lcd.print(timer1 ? " A" : " D");
    lcd.print(timer2 ? "  A" : "  D");
    lcd.print(timer3 ? "  A" : "  D");
    lcd.print(timer4 ? "  A" : "  D");
  }
}

void blinking() {
  if (millis() - lastBlinkTime >= 500) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  if (!blinkState) return;

   if (setAlarm < 2) {
    if (setMode == 1) { lcd.setCursor(0, 0); lcd.print("  "); }
    if (setMode == 2) { lcd.setCursor(3, 0); lcd.print("  "); }
    if (setMode == 3) { lcd.setCursor(6, 0); lcd.print("  "); }
    if (setMode == 4) { lcd.setCursor(0, 1); lcd.print("    "); }
    if (setMode == 5) { lcd.setCursor(6, 1); lcd.print("  "); }
    if (setMode == 6) { lcd.setCursor(9, 1); lcd.print("  "); }
    if (setMode == 7) { lcd.setCursor(14, 1); lcd.print("    "); }
  }

  if (setMode == 0 && setAlarm > 0) {
    if (mode == 0) {
      if (setAlarm == 1) lcd.setCursor(1, 1);
      if (setAlarm == 2) lcd.setCursor(4, 1);
      if (setAlarm == 3) lcd.setCursor(7, 1);
      if (setAlarm == 4) lcd.setCursor(10, 1);
      lcd.print(" ");
    } else {
      if (setAlarm == 1) { lcd.setCursor(11, 0); lcd.print("  "); }
      if (setAlarm == 2) { lcd.setCursor(14, 0); lcd.print("  "); }
      if (setAlarm == 3) { lcd.setCursor(11, 1); lcd.print("  "); }
      if (setAlarm == 4) { lcd.setCursor(14, 1); lcd.print("  "); }
    }
  }
}


void saveRelaySettings() {
  EEPROM.write(0, timer1);
  EEPROM.write(1, timer2);
  EEPROM.write(2, timer3);
  EEPROM.write(3, timer4);

  EEPROM.write(4, Start1HH); EEPROM.write(5, Start1MM);
  EEPROM.write(6, Finish1HH); EEPROM.write(7, Finish1MM);

  EEPROM.write(8, Start2HH); EEPROM.write(9, Start2MM);
  EEPROM.write(10, Finish2HH); EEPROM.write(11, Finish2MM);

  EEPROM.write(12, Start3HH); EEPROM.write(13, Start3MM);
  EEPROM.write(14, Finish3HH); EEPROM.write(15, Finish3MM);

  EEPROM.write(16, Start4HH); EEPROM.write(17, Start4MM);
  EEPROM.write(18, Finish4HH); EEPROM.write(19, Finish4MM);

  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
  EEPROM.commit();
}

void loadRelaySettings() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) {
    timer1 = timer2 = timer3 = timer4 = 0;

    Start1HH = Start1MM = Finish1HH = Finish1MM = 0;
    Start2HH = Start2MM = Finish2HH = Finish2MM = 0;
    Start3HH = Start3MM = Finish3HH = Finish3MM = 0;
    Start4HH = Start4MM = Finish4HH = Finish4MM = 0;

    saveRelaySettings();
    return;
  }

  timer1 = EEPROM.read(0) ? 1 : 0;
  timer2 = EEPROM.read(1) ? 1 : 0;
  timer3 = EEPROM.read(2) ? 1 : 0;
  timer4 = EEPROM.read(3) ? 1 : 0;

  Start1HH = EEPROM.read(4); Start1MM = EEPROM.read(5);
  Finish1HH = EEPROM.read(6); Finish1MM = EEPROM.read(7);

  Start2HH = EEPROM.read(8); Start2MM = EEPROM.read(9);
  Finish2HH = EEPROM.read(10); Finish2MM = EEPROM.read(11);

  Start3HH = EEPROM.read(12); Start3MM = EEPROM.read(13);
  Finish3HH = EEPROM.read(14); Finish3MM = EEPROM.read(15);

  Start4HH = EEPROM.read(16); Start4MM = EEPROM.read(17);
  Finish4HH = EEPROM.read(18); Finish4MM = EEPROM.read(19);

  // Reject invalid/corrupt saved values.
  if (Start1HH > 23 || Start1MM > 59 || Finish1HH > 23 || Finish1MM > 59 ||
      Start2HH > 23 || Start2MM > 59 || Finish2HH > 23 || Finish2MM > 59 ||
      Start3HH > 23 || Start3MM > 59 || Finish3HH > 23 || Finish3MM > 59 ||
      Start4HH > 23 || Start4MM > 59 || Finish4HH > 23 || Finish4MM > 59) {
    timer1 = timer2 = timer3 = timer4 = 0;
    Start1HH = Start1MM = Finish1HH = Finish1MM = 0;
    Start2HH = Start2MM = Finish2HH = Finish2MM = 0;
    Start3HH = Start3MM = Finish3HH = Finish3MM = 0;
    Start4HH = Start4MM = Finish4HH = Finish4MM = 0;
    saveRelaySettings();
  }
}