# ESP32 Access Point Version

This folder contains the Arduino IDE version of the 4-relay DS3231 timer.

## Current features

- ESP32 Wi-Fi Access Point
- Local web dashboard at `192.168.4.1`
- Captive-portal style redirects
- DS3231 RTC
- 16x2 I2C LCD
- 4 independent relay channels
- AUTO / forced ON / forced OFF modes
- One schedule per relay, including schedules that cross midnight
- Schedule and mode persistence using ESP32 Preferences/NVS
- Mobile-responsive embedded web interface
- No internet or router required

## Arduino IDE libraries

Install these from Library Manager:

- RTClib by Adafruit
- LiquidCrystal I2C
- ArduinoJson by Benoit Blanchon

ESP32 support must also be installed through Arduino IDE Boards Manager.

## Default network

- SSID: `Relay-Control`
- Password: `relay1234`
- Dashboard: `http://192.168.4.1`

Change the defaults in `config.h` before production use.

## Default pin map

| Function | GPIO |
|---|---:|
| I2C SDA | 21 |
| I2C SCL | 22 |
| Relay 1 | 25 |
| Relay 2 | 26 |
| Relay 3 | 27 |
| Relay 4 | 33 |
| Clock button | 13 |
| Up button | 14 |
| Down button | 16 |
| Timer button | 17 |
| Back button | 18 |
| Buzzer | 19 |

## Important

Many 4-channel relay boards are active-low. If yours is active-low, change:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = true;
```

to:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

in `config.h`.

Test relay switching with low voltage first. Do not connect mains voltage until the logic and relay polarity have been verified.
