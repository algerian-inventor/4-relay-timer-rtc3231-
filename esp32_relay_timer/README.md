# ESP32 School Bell Controller — V1

Arduino IDE firmware for a standalone school bell timer built around only:

- ESP32
- DS3231 RTC
- one relay module
- ESP32 Wi-Fi access point

There is no LCD, rotary encoder, button panel or external router requirement.

## How it works

1. The ESP32 boots and immediately puts the relay in its OFF state.
2. The DS3231 provides the current time.
3. The ESP32 creates its own Wi-Fi network.
4. A phone or laptop connects directly to the ESP32.
5. The local web dashboard is opened at `http://192.168.4.1`.
6. Schedules are stored in ESP32 non-volatile memory.
7. The ESP32 continuously compares the RTC time with the stored schedule and controls the relay.
8. The phone can disconnect; the controller continues operating by itself.

Internet is not required.

## Default access point

- SSID: `SchoolBell-Setup`
- Password: `schoolbell`
- Dashboard: `http://192.168.4.1`

Change production credentials in `config.h`.

## Hardware

| Function | ESP32 GPIO |
|---|---:|
| DS3231 SDA | 21 |
| DS3231 SCL | 22 |
| Relay IN | 25 |

The DS3231 backup battery keeps time when the ESP32 loses power.

## Arduino IDE dependencies

Install:

- **RTClib** by Adafruit
- **ArduinoJson** by Benoit Blanchon

Install Espressif ESP32 board support from Boards Manager.

No LCD library is required.

## Relay logic

The firmware defaults to the common active-LOW relay-module convention:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

For active-LOW modules:

- GPIO HIGH = relay OFF
- GPIO LOW = relay ON

Verify the exact module before connecting the real bell.

## Fail-safe wiring

For the automatic bell output, use the relay's **COM + NO** contacts so an unpowered relay opens the automatic bell circuit.

The existing manual bell control should remain electrically independent of the ESP32. The automatic relay contact can be wired as an additional control path so the school can still ring the bell manually if the controller is disconnected or fails.

A production installation must also prevent the relay input from floating when the ESP32 is unpowered. Use the correct external bias resistor for the specific relay module and verify the module's behavior with the ESP32 powered off.

Do not use a prototype relay board to switch a mains bell directly unless its ratings, isolation, wiring, enclosure and protection are appropriate. Use an isolated relay/contactor stage where required.

## Current software

The local dashboard currently provides:

- RTC time and date
- relay state
- AUTO / ON / OFF control
- weekday schedules
- RTC synchronization from the connected phone/computer
- local event history
- factory reset
- captive-portal style redirect
- persistent settings in ESP32 Preferences/NVS

## Status

This is the minimal V1 hardware architecture for the school bell product. Hardware testing on the exact ESP32, DS3231 and relay module is still required before installation.
