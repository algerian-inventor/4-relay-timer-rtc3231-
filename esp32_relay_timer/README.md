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

The firmware now defaults to the common **active-LOW relay-module** convention:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

At boot, the OFF logic level is written **before** each relay GPIO is configured as an output. This reduces startup/reset glitches.

For a fail-safe product, firmware is only one layer. The hardware should also guarantee:

- use **COM + NO** for the automatic bell contact so an unpowered relay opens the circuit;
- add an external bias resistor on each relay input so the input cannot float when the ESP32 is unpowered;
- for active-LOW modules, bias the input to the module's verified OFF level;
- keep the manual bell path independent of the ESP32 so a controller failure does not remove manual operation;
- use an appropriately rated isolated relay/contactor stage for the real bell load.

Important: some relay modules remain powered from a separate 5 V rail while the ESP32 is off. In that condition, a floating or back-powered IN pin can energize the relay. Verify the exact module before installation.

Test with LEDs or another low-voltage load first. Do not connect mains voltage until relay polarity, isolation, wiring and enclosure safety are verified.

## Status

This branch is an active V1 refactor. The architecture and UI are substantially upgraded, but hardware compilation/testing is still required on the target ESP32 board before calling it production-ready.
