# ESP32 Relay Automation Controller — V1

Arduino IDE firmware for a standalone 4-channel automation controller using ESP32 + DS3231.

## V1 architecture

The branch is no longer a direct port of the original Arduino timer. It is being rebuilt as a product-style controller with:

- ESP32 SoftAP + captive portal behavior
- responsive local dashboard
- four independently named channels
- AUTO / forced ON / forced OFF modes
- six schedules per channel
- weekday masks
- correct cross-midnight scheduling
- DS3231 RTC
- sync RTC from a connected phone/computer
- ESP32 Preferences/NVS persistence
- versioned settings schema
- runtime event history
- safe relay-off boot sequence
- physical LCD/button fallback controls
- factory reset API/UI
- no internet requirement

## Arduino IDE dependencies

Install:

- **RTClib** by Adafruit
- **LiquidCrystal I2C**
- **ArduinoJson** by Benoit Blanchon

Install Espressif ESP32 board support from Boards Manager.

## Default access point

- SSID: `Relay-Control`
- Password: `relay1234`
- Address: `http://192.168.4.1`

Change production credentials in `config.h`.

## GPIO map

| Function | GPIO |
|---|---:|
| SDA | 21 |
| SCL | 22 |
| Relay 1 | 25 |
| Relay 2 | 26 |
| Relay 3 | 27 |
| Relay 4 | 33 |
| Clock / AUTO | 13 |
| Up / force ON | 14 |
| Down / force OFF | 16 |
| Timer / next channel | 17 |
| Back / AUTO | 18 |
| Buzzer | 19 |

The LCD and DS3231 share the I2C bus.

## Physical controls

The LCD always shows the currently selected channel.

- **Timer**: select next channel
- **Up**: force selected channel ON
- **Down**: force selected channel OFF
- **Clock**: return selected channel to AUTO
- **Back**: return selected channel to AUTO

## Safety

The firmware drives all relay outputs to the OFF state before loading persisted configuration.

Relay boards vary. If yours is active-low, set:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

in `config.h`.

Test with LEDs or another low-voltage load first. Do not connect mains voltage until relay polarity, isolation, wiring and enclosure safety are verified.

## Status

This branch is an active V1 refactor. The architecture and UI are substantially upgraded, but hardware compilation/testing is still required on the target ESP32 board before calling it production-ready.
