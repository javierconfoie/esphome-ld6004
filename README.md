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

### Binary sensors

| Key | Description |
|-----|-------------|
| `presence` | True if anyone is present in any zone |
| `zone0_presence` … `zone3_presence` | Per-zone presence |

### Sensors

| Key | Description |
|-----|-------------|
| `target_count` | Number of currently tracked targets (0–3) |
| `target0_x` … `target2_x` | Target X position (m) |
| `target0_y` … `target2_y` | Target Y / depth (m) |
| `target0_z` … `target2_z` | Target Z / height (m) |
| `target0_doppler` … `target2_doppler` | Doppler velocity index |
| `target0_id` … `target2_id` | Cluster ID |

### Text sensors

| Key | Description |
|-----|-------------|
| `firmware_version` | Radar firmware version string |

### Selects (configuration)

| Key | Options |
|-----|---------|
| `sensitivity` | `Low`, `Medium`, `High` |
| `trigger_speed` | `Slow`, `Medium`, `Fast` |
| `install_method` | `Top`, `Side` |
| `operating_mode` | `Low Power`, `Normal`, `Off P20 High`, `Off P20 Low`, `High Reflectivity` |
| `p20_mode` | `High Present`, `Low Present`, `Always Low`, `Always High`, `Pulse Low`, `Pulse High` |

### Numbers (configuration)

| Key | Unit | Description |
|-----|------|-------------|
| `delay_time` | s | Hold-on delay after last detection |
| `sleep_time` | ms | Low-power sleep period when unoccupied |
| `output_interval` | — | Serial output interval multiplier |
| `dwell_lifecycle` | — | Target lifecycle in dwell zones |
| `z_range_min` | m | Minimum Z-axis detection height |
| `z_range_max` | m | Maximum Z-axis detection height |

### Switches

| Key | Description |
|-----|-------------|
| `point_cloud` | Enable/disable point cloud output |
| `target_display` | Enable/disable target location output |

### Buttons

| Key | Description |
|-----|-------------|
| `refresh_config` | Re-read all configuration from the radar |
| `reset_unmanned` | Force sensor to "unoccupied" state |
| `auto_interference` | Auto-generate interference zones |
| `clear_interference` | Clear all interference zones |
| `reset_detection` | Reset detection area to default |
| `clear_dwell` | Clear all dwell zones |

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
