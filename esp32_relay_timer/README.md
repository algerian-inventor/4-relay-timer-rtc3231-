# ESP32 School Bell Controller — V1

Standalone Arduino IDE firmware for a school bell controller built around only:

- ESP32
- DS3231 RTC
- one relay module
- ESP32 Wi-Fi access point

No LCD, rotary encoder, button panel, router or internet connection is required.

## Default timetable

The controller ships with these 12 enabled bell events:

| # | Time |
|---:|:---|
| 1 | 07:45 |
| 2 | 08:00 |
| 3 | 09:00 |
| 4 | 10:00 |
| 5 | 10:10 |
| 6 | 11:00 |
| 7 | 12:00 |
| 8 | 12:45 |
| 9 | 13:00 |
| 10 | 14:00 |
| 11 | 15:00 |
| 12 | 16:00 |

Defaults are enabled **Sunday through Thursday** and each event rings for **3 seconds**. Every time, weekday selection, enabled state and pulse duration can be edited from the web dashboard.

Factory reset restores this timetable.

## How it works

1. The ESP32 powers up with the relay forced to OFF.
2. The DS3231 supplies the date and time.
3. The ESP32 creates the Wi-Fi network `SchoolBell-Setup`.
4. A phone or computer connects directly to the ESP32.
5. The dashboard is opened at `http://192.168.4.1`.
6. Bell events are stored in ESP32 non-volatile Preferences/NVS memory.
7. At a matching event time, the ESP32 produces a short non-blocking relay pulse.
8. The phone can disconnect and the schedule continues autonomously.

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

The DS3231 backup battery maintains time while the ESP32 is unpowered.

## Arduino IDE dependencies

Install:

- **RTClib** by Adafruit
- **ArduinoJson** by Benoit Blanchon
- Espressif ESP32 board support

No LCD library is required.

## Reliability and safety behavior

The firmware is designed so the automatic controller fails toward **bell OFF**:

- relay OFF level is written before GPIO 25 becomes an output;
- the common active-LOW relay convention is the default;
- automatic events are pulses, never permanent FORCE-ON states;
- pulse timing is non-blocking, so the web server and scheduler remain responsive;
- the dashboard can stop a currently active pulse, and Stop always forces the physical relay GPIO OFF;
- automatic scheduling is disabled when the DS3231 is missing or reports lost power;
- time must be synchronized again after RTC power loss;
- automatic events only start during the first few seconds of their scheduled minute, preventing a reboot late in the minute from causing a delayed bell;
- the last automatic event minute is stored in NVS to reduce duplicate ringing after a reset;
- editing a schedule, synchronizing the RTC, restoring defaults or resuming automation suppresses ringing in the current minute;
- duplicate enabled events at the same time on overlapping weekdays are rejected;
- invalid dates, times and pulse durations are rejected;
- oversized JSON API bodies are rejected before parsing;
- RTC year validity is checked again at boot before automation is trusted;
- the I2C bus uses a timeout so a wiring problem is less likely to stall the controller indefinitely;
- Wi-Fi AP status and RTC validity are exposed in the dashboard.

These protections reduce software failure modes, but they do **not** replace hardware fail-safe design or real hardware testing.

## Relay logic

The firmware defaults to:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

For a typical active-LOW module:

- GPIO HIGH = relay OFF
- GPIO LOW = relay ON

Verify the exact relay module before installing the controller.

## Fail-safe bell wiring

Use the relay's **COM + NO** contacts for the automatic bell path so an unpowered relay opens that path.

Keep the school's manual bell control independent from the ESP32. The automatic relay contact should act as an additional control path, so a controller fault or removal does not prevent manual operation.

A production build must also bias the relay input to a known OFF state when the ESP32 is unpowered. The correct pull-up/pull-down arrangement depends on the exact relay module, especially when the module remains powered from a separate 5 V supply.

Do not switch a mains bell directly with a prototype relay module unless the module, isolation, PCB clearances, enclosure, fusing and installation are appropriate for that load. Use an appropriately rated isolated relay/contactor stage where required.

## Dashboard

The local dashboard provides:

- current DS3231 time/date and RTC fault warning
- next scheduled bell
- manual **Ring now** pulse
- manual **Stop bell**
- automatic schedule pause/resume
- all 12 editable bell events
- weekday selection per event
- 1–30 second pulse duration per event
- RTC synchronization from the connected phone/computer
- runtime history
- AP/client status
- factory reset to the default timetable

## Status

Firmware: `1.0.0-alpha.5`

This branch is intended for hardware validation before production use. Test first with an LED or safe low-voltage load, then verify the exact relay module and DS3231 behavior before connecting a real school bell.
