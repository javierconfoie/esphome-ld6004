# esphome-ld6004

ESPHome custom component for the **HLK-LD6004** 60GHz 3D presence radar sensor.

Supports multi-target 3D tracking, per-zone presence detection, full configuration
read/write, and P20 GPIO control — all via Home Assistant.

## Features

- **Presence detection** — global + 4 configurable detection zones
- **3D target tracking** — up to 3 simultaneous targets with X/Y/Z, Doppler, and cluster ID
- **Full configuration** via HA — sensitivity, trigger speed, install method, operating mode, delay/sleep times, Z-range, dwell lifecycle, output interval, P20 GPIO mode
- **Zone management** — detection zones, interference zones, dwell zones
- **Firmware version** reported as text sensor
- **Refresh Config** button — re-reads all settings from the radar at any time

## Hardware

| Item | Value |
|------|-------|
| Sensor | HLK-LD6004 (60GHz FMCW 3D) |
| Protocol | TinyFrame UART 115200 8N1 |
| Supply voltage | 3.3V, 1A minimum |
| Tested MCU | ESP32-C3 |

### Wiring (ESP32-C3 example)

```
Radar 3.3V  →  External 3.3V regulator (1A minimum)
Radar GND   →  Common GND
Radar TX    →  ESP32 GPIO20 (RX)
Radar RX    →  ESP32 GPIO21 (TX)
ESP32 GND   →  Common GND
```

> **Warning:** The radar requires up to **1A at 3.3V**. Do **not** power it from the ESP32
> dev board's 3.3V pin — the on-board LDO cannot supply enough current and the system
> will be unstable. Use a dedicated 3.3V regulator (e.g. AMS1117-3.3 with adequate
> heat dissipation, or a switching regulator) rated for at least 1A.

## Installation

### Option A — External component from GitHub (recommended)

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/javierconfoie/esphome-ld6004
      ref: main
    components: [hlk_ld6004]
```

### Option B — Local development

Clone this repo and place it next to your YAML file:

```
my-esphome/
  hlk-ld6004.yaml
  components/          ← symlink or copy from this repo
    hlk_ld6004/
```

```yaml
external_components:
  - source:
      type: local
      path: components
```

## Minimal Configuration

```yaml
uart:
  id: uart_ld6004
  tx_pin: GPIO21
  rx_pin: GPIO20
  baud_rate: 115200

hlk_ld6004:
  id: ld6004
  uart_id: uart_ld6004

binary_sensor:
  - platform: hlk_ld6004
    hlk_ld6004_id: ld6004
    presence:
      name: "Presence"
```

## Full Configuration Reference

See [`hlk-ld6004.yaml`](hlk-ld6004.yaml) for a complete example with all sensors,
selects, numbers, switches, and buttons.

---

### Binary sensors

#### `presence`
Global occupancy state. `True` when at least one of the four detection zones reports
a person present. This is the primary sensor for presence-based automations.

#### `zone0_presence` … `zone3_presence`
Per-zone occupancy. Each maps to one of the four configurable detection zones in the
radar. `True` when the radar reports at least one tracked target inside that zone.
Useful for room or area subdivision (e.g. zone 0 = near the door, zone 1 = desk area).

---

### Sensors

#### `target_count`
Number of targets currently being tracked by the radar (0–3). Updates with every
presence/target frame (~10 Hz). Can be used to count people in a room.

#### `target0_x` / `target1_x` / `target2_x`
Horizontal position of each target in meters, measured from the sensor center.
Negative values are to the left, positive to the right (when the sensor faces forward).
Set to `NaN` when fewer than that number of targets are present.

#### `target0_y` / `target1_y` / `target2_y`
Depth (forward distance) of each target in meters from the sensor.
Set to `NaN` when the target is not present.

#### `target0_z` / `target1_z` / `target2_z`
Height of each target in meters relative to the sensor.
Set to `NaN` when the target is not present. Useful to distinguish sitting vs. standing.

#### `target0_doppler` / `target1_doppler` / `target2_doppler`
Signed velocity index from the Doppler channel. Positive = moving toward the sensor,
negative = moving away. The unit is an internal radar index, not m/s. Set to `NaN`
when the target is absent.

#### `target0_id` / `target1_id` / `target2_id`
Cluster ID assigned by the radar's tracking algorithm. Can be used to follow a
specific target across frames. IDs are reassigned each detection session.
Set to `NaN` when the target is absent.

---

### Text sensors

#### `firmware_version`
Firmware version string reported by the radar on boot, in the format `major.sub.modified`
(e.g. `3.18.0`). Useful to confirm the sensor model and firmware compatibility.

---

### Selects (configuration)

All selects read their current value from the radar on boot and write back to the radar
when changed from Home Assistant.

#### `sensitivity`
Detection sensitivity level.
- `Low` — fewer false positives, requires closer or more active presence
- `Medium` — balanced (default)
- `High` — detects faint or very still targets, may increase false positives

#### `trigger_speed`
Minimum movement speed required to trigger presence.
- `Slow` — detects even very slow micro-movements (e.g. sleeping person)
- `Medium` — filters out very slow drift
- `Fast` — only detects clearly moving targets

#### `install_method`
Mounting orientation of the radar module.
- `Top` — sensor faces downward from ceiling
- `Side` — sensor mounted on a wall or vertical surface

This affects the internal coordinate transform used for zone matching.

#### `operating_mode`
Overall operating mode of the radar.
- `Low Power` — reduced TX power, shorter range, lower consumption
- `Normal` — standard full-power detection
- `Off P20 High` — detection off, P20 GPIO driven high
- `Off P20 Low` — detection off, P20 GPIO driven low
- `High Reflectivity` — tuned for environments with many reflective surfaces

#### `p20_mode`
Behavior of the radar's P20 GPIO output pin, which can drive an external indicator
or relay.
- `High Present` — pin goes high when presence is detected
- `Low Present` — pin goes low when presence is detected (active-low)
- `Always Low` — pin always low regardless of detection
- `Always High` — pin always high regardless of detection
- `Pulse Low` — brief low pulse on detection trigger
- `Pulse High` — brief high pulse on detection trigger

---

### Numbers (configuration)

All numbers read their current value from the radar on boot and write back to the radar
when changed from Home Assistant.

#### `delay_time` (unit: seconds)
Hold-on time after the last detected presence event before `presence` switches to
`False`. For example, a value of `30` keeps the presence sensor `True` for 30 seconds
after the last detected movement. Prevents rapid on/off flapping.

#### `sleep_time` (unit: milliseconds)
Low-power sleep interval used in `Low Power` operating mode. The radar enters a sleep
cycle of this duration when no presence is detected. Increase to reduce power
consumption at the cost of slower wake-up response.

#### `output_interval`
Multiplier for the UART output rate. A value of `1` outputs at the maximum rate
(~10 Hz). Higher values reduce the UART output frequency proportionally.
On single-core MCUs (ESP32-C3), increasing this value can significantly improve
WiFi stability by freeing CPU time.

#### `dwell_lifecycle`
Controls how long a target must remain stationary inside a dwell zone before it is
considered "dwelling" (i.e. settled, not just passing through). Higher values require
longer stationary time. Sets the temporal hysteresis for dwell-zone presence.

#### `z_range_min` / `z_range_max` (unit: meters)
Defines the vertical (Z-axis) detection window. Targets outside this height range are
ignored. Useful to exclude the floor or ceiling, or to restrict detection to a
specific height band (e.g. seated occupants only).

---

### Switches

#### `point_cloud`
Enables or disables the raw point cloud data output from the radar over UART.
When enabled, the radar sends an additional frame type with the point cloud alongside
the target location frames. Disable it if you only need the tracked targets and want
to reduce UART traffic.

#### `target_display`
Enables or disables the target location output frames. When disabled, the radar does
not send target X/Y/Z/Doppler/ID data. The `presence` binary sensor continues to work
from the separate presence state frames. Disable only if you only need presence and
want to reduce UART traffic.

---

### Buttons

#### `refresh_config`
Triggers a re-read of all 12 configuration parameters from the radar. The component
queries them automatically on first boot, but this button lets you re-sync if any
setting was changed externally (e.g. via the PC configuration tool).

#### `reset_unmanned`
Sends a command to force the radar into the "unoccupied" state, clearing any stuck
presence detection. Useful if the sensor falsely reports presence after someone has
left (e.g. due to a slowly dissipating reflection).

#### `auto_interference`
Commands the radar to automatically identify and record interference sources in the
current environment (static reflectors such as fans, furniture, or AC units). This
should be run once during setup with the area empty of people.

#### `clear_interference`
Clears all previously recorded interference zones, re-enabling detection in those
areas. Use this if the environment has changed or if `auto_interference` captured
zones it shouldn't have.

#### `reset_detection`
Resets the detection zone configuration to the factory defaults, covering the full
field of view. Use this if you want to undo custom zone settings.

#### `clear_dwell`
Clears all configured dwell zones. After this, all areas are treated uniformly with
no dwell-specific tracking behaviour.

## Protocol Notes

The HLK-LD6004 uses the **TinyFrame** protocol over UART.

**Critical implementation detail:** For frames with `LEN=0` (ACK frames), the radar
does **not** send a `DATA_CKSUM` byte. The frame is exactly 8 bytes
(`SOF + ID + LEN + TYPE + HEAD_CKSUM`). This is not clearly documented in the
official protocol PDF but was confirmed by hardware testing.

Outgoing frames with `LEN=0` also omit the `DATA_CKSUM` byte.

## Troubleshooting

**Config values show "unknown" in HA**
→ The component queries all config parameters automatically on boot. If they
remain unknown, check the logger output for CKSUM errors or increase log level to DEBUG.

**WiFi keeps disconnecting**
→ The radar outputs presence/target frames continuously (~10Hz). On single-core
MCUs (ESP32-C3), this can starve the WiFi stack. Set `output_interval` to a
higher value once connected, or use `power_save_mode: none` in the WiFi config.

**Radar doesn't respond to commands**
→ Verify TX and RX are not swapped. The radar ACKs every command with a
0-length reply of the same TYPE before sending the response payload.

## License

MIT — see [LICENSE](LICENSE).
