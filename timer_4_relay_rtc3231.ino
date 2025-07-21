#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define bt_clock  2
#define bt_up     3
#define bt_down   4
#define bt_timer  5
#define bt_back   6

#define relay1 8
#define relay2 9
#define relay3 10
#define relay4 11
#define buzzer 13

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
  Wire.begin();
  rtc.begin();
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
  digitalWrite(relay1, LOW);
   digitalWrite(relay2, LOW);
   digitalWrite(relay3, LOW);    
   digitalWrite(relay4, LOW);
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

  digitalWrite(relay1, (timer1 && alarmMode && inRange(hh, mm, Start1HH, Start1MM, Finish1HH, Finish1MM)) ? HIGH : LOW);
  digitalWrite(relay2, (timer2 && alarmMode && inRange(hh, mm, Start2HH, Start2MM, Finish2HH, Finish2MM)) ? HIGH : LOW);
  digitalWrite(relay3, (timer3 && alarmMode && inRange(hh, mm, Start3HH, Start3MM, Finish3HH, Finish3MM)) ? HIGH : LOW);
  digitalWrite(relay4, (timer4 && alarmMode && inRange(hh, mm, Start4HH, Start4MM, Finish4HH, Finish4MM)) ? HIGH : LOW);
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
  EEPROM.update(0, timer1);
  EEPROM.update(1, timer2);
  EEPROM.update(2, timer3);
  EEPROM.update(3, timer4);

  EEPROM.update(4, Start1HH); EEPROM.update(5, Start1MM);
  EEPROM.update(6, Finish1HH); EEPROM.update(7, Finish1MM);

  EEPROM.update(8, Start2HH); EEPROM.update(9, Start2MM);
  EEPROM.update(10, Finish2HH); EEPROM.update(11, Finish2MM);

  EEPROM.update(12, Start3HH); EEPROM.update(13, Start3MM);
  EEPROM.update(14, Finish3HH); EEPROM.update(15, Finish3MM);

  EEPROM.update(16, Start4HH); EEPROM.update(17, Start4MM);
  EEPROM.update(18, Finish4HH); EEPROM.update(19, Finish4MM);
}

void loadRelaySettings() {
  timer1 = EEPROM.read(0);
  timer2 = EEPROM.read(1);
  timer3 = EEPROM.read(2);
  timer4 = EEPROM.read(3);

  Start1HH = EEPROM.read(4); Start1MM = EEPROM.read(5);
  Finish1HH = EEPROM.read(6); Finish1MM = EEPROM.read(7);

  Start2HH = EEPROM.read(8); Start2MM = EEPROM.read(9);
  Finish2HH = EEPROM.read(10); Finish2MM = EEPROM.read(11);

  Start3HH = EEPROM.read(12); Start3MM = EEPROM.read(13);
  Finish3HH = EEPROM.read(14); Finish3MM = EEPROM.read(15);

  Start4HH = EEPROM.read(16); Start4MM = EEPROM.read(17);
  Finish4HH = EEPROM.read(18); Finish4MM = EEPROM.read(19);
}