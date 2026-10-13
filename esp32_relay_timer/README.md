# ESP32 School Bell Controller — V1

Standalone school bell controller built around only:

- ESP32
- DS3231 RTC
- one relay module
- ESP32 Wi-Fi access point

No LCD, rotary encoder, physical menu, external router or internet connection is required.

## Default timetable

Factory defaults are enabled Sunday through Thursday and ring the relay for **3 seconds** at:

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

There are 16 event slots total, so four additional bells can be added from the web dashboard.

## How it works

1. The ESP32 boots with the relay forced OFF.
2. The DS3231 provides the time.
3. The ESP32 creates the Wi-Fi network `SchoolBell-Setup`.
4. A phone or laptop connects directly to the ESP32.
5. The dashboard is opened at `http://192.168.4.1`.
6. Bell events are stored in ESP32 Preferences/NVS.
7. At a scheduled minute the relay receives one short pulse.
8. The relay is released by a non-blocking timeout.
9. The phone can disconnect; the controller continues operating by itself.

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

The DS3231 backup battery keeps its clock running when the ESP32 supply is removed.

## Arduino IDE dependencies

Install:

- **RTClib** by Adafruit
- **ArduinoJson** by Benoit Blanchon
- Espressif ESP32 board support

No LCD library is required.

## Reliability behavior

The scheduler was changed from ON/OFF time intervals to one-shot **bell events**.

Important protections in the firmware:

- relay OFF level is established before the GPIO becomes an output;
- default active-LOW relay logic for common relay modules;
- scheduled ringing is blocked when the DS3231 is missing or reports lost power;
- invalid RTC time must be synchronized before automation runs;
- after boot or a clock change, the current minute is only observed, not replayed, reducing accidental duplicate bells after a power interruption;
- each scheduled minute can trigger only once during normal runtime;
- relay pulses use a non-blocking deadline rather than `delay()`;
- pulse duration is validated and limited to 1–15 seconds;
- malformed or oversized API requests are rejected;
- calendar dates are validated before writing to the RTC;
- schedule edits are range-checked before being saved;
- automation can be disabled without disabling manual web ringing;
- factory reset first releases the relay and restores the original timetable;
- settings schema is versioned so incompatible old settings are replaced safely.

These protections reduce common software failure modes, but they do **not** replace hardware validation.

## Relay logic

The firmware defaults to a common active-LOW relay module:

```cpp
static constexpr bool RELAY_ACTIVE_HIGH = false;
```

For this configuration:

- GPIO HIGH = relay OFF
- GPIO LOW = relay ON

Verify your exact relay module before connecting the real school bell.

## Fail-safe wiring

For the automatic output use the relay's **COM + NO** contacts so an unpowered relay opens the automatic bell circuit.

Keep the existing manual school-bell path independent of the ESP32. The automatic relay contact should be an additional control path, so the original manual control remains available if the controller is unplugged or fails.

If the relay module remains powered while the ESP32 is off, its IN pin must have the correct external bias so it cannot float into the ON state.

For mains or high-current bells, use a properly rated isolated relay/contactor stage, protection and enclosure. Do not assume a small development relay board is suitable for the final load.

## Dashboard

The local dashboard provides:

- live RTC time and date
- next-bell calculation
- relay state
- manual **Ring now** pulse
- enable/disable automation
- editable bell times
- selectable weekdays
- per-event pulse duration
- add/disable bell events
- RTC synchronization from the connected phone/computer
- local runtime history
- factory timetable restore
- captive-portal-style redirect
- persistent configuration

## RTC safety behavior

If the DS3231 is absent or reports that it lost power, automatic ringing is disabled. Connect to the dashboard and use **Sync RTC from this device** before relying on the automatic timetable.

## Status

Firmware version: `1.0.0-beta.1`

This branch is intended for hardware testing. It has not yet been validated on the exact final relay module, bell load, power supply and enclosure, so installation testing is still required before treating it as production-ready.
