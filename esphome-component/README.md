# ESPHome Studer Component

An ESPHome external component for monitoring Studer Innotec devices (Xtender, VarioTrack, VarioString, BSP) via the SCOM serial protocol.

## Features

- Native ESPHome integration
- Easy YAML configuration
- Automatic Home Assistant discovery
- No MQTT configuration needed
- Poll multiple parameters from multiple devices
- Support for all Studer device addresses and parameters

## Hardware Requirements

- ESP32 (recommended) or ESP8266
- RS-232 to TTL converter (e.g., MAX3232)
- Connection to Studer XCom-232i

## Wiring

```
XCom-232i RS-232  <-->  MAX3232  <-->  ESP32
TX (pin 2)              RX              GPIO16 (RX)
RX (pin 3)              TX              GPIO17 (TX)
GND (pin 5)             GND             GND
```

## Installation

### Option 1: Use as External Component (from GitHub)

In your ESPHome YAML:

```yaml
external_components:
  - source: github://kolins-cz/studer232-to-mqtt/esphome-component
    components: [studer]
```

### Option 2: Local Development

Copy the `components/studer` directory to your ESPHome configuration folder:

```bash
cp -r esphome-component/components/studer ~/.esphome/custom_components/
```

## Configuration

### Basic Setup

```yaml
uart:
  id: uart_bus
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200  # or 38400 depending on your XCom-232i
  parity: EVEN
  stop_bits: 1

studer:
  id: studer_hub
  uart_id: uart_bus

sensor:
  - platform: studer
    studer_id: studer_hub
    address: 101              # Xtender address
    parameter: 3137           # Parameter number
    name: "XT1 Input Power"
    unit_of_measurement: "kW"
    multiply: -1              # Sign correction if needed
```

### Sensor Configuration Options

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `studer_id` | ID | Yes | Reference to the studer component |
| `address` | int | Yes | Device address (101-109 for Xtenders, etc.) |
| `parameter` | int | Yes | Parameter number from protocol documentation |
| `name` | string | Yes | Sensor name in Home Assistant |
| `unit_of_measurement` | string | No | Unit (kW, V, A, °C, etc.) |
| `multiply` | float | No | Multiplier for value (default: 1.0) |
| `device_class` | string | No | HA device class (power, voltage, current, etc.) |
| `state_class` | string | No | HA state class (measurement, total, etc.) |
| `accuracy_decimals` | int | No | Number of decimal places |

## Device Addresses

| Device Type | Address Range | Notes |
|-------------|---------------|-------|
| Xtender | 101-109 | Individual inverters |
| All Xtenders | 100 | Multicast (write only) |
| Phase L1/L2/L3 | 191-193 | Virtual addresses for phases |
| VarioTrack | 301-315 | Solar charge controllers |
| All VarioTrack | 300 | Multicast (write only) |
| BSP | 601 | Battery State Processor |
| VarioString | 701-715 | String inverters |
| Xcom-232i | 501 | Communication gateway |

## Common Parameters

See the [protocol documentation](../../doc/) for complete parameter lists.

### Xtender Info Parameters (read via parameter object)

| Parameter | Description | Unit |
|-----------|-------------|------|
| 3000 | Battery voltage | V |
| 3005 | Battery current | A |
| 3104 | Temperature | °C |
| 3136 | Output active power | kW |
| 3137 | Input active power | kW |
| 3138 | Apparent power | kVA |
| 3085 | Output frequency | Hz |

## Example Configurations

See [`example.yaml`](example.yaml) for a complete example with multiple Xtenders and phases.

## Troubleshooting

### No data received

1. Check UART wiring and baud rate
2. Verify XCom-232i is powered and responding
3. Enable DEBUG logging to see serial communication:
   ```yaml
   logger:
     level: DEBUG
   ```

### Wrong values

- Check the `multiply` parameter (some values need sign correction)
- Verify parameter number from documentation
- Check device address is correct

### Compile errors

- Ensure you're using ESP32 (ESP8266 might have memory constraints)
- Check ESPHome version (minimum 2023.x recommended)

## Development

The component structure:

```
components/studer/
├── __init__.py          # Component registration
├── sensor.py            # Sensor platform
├── studer.h/cpp         # Main component (UART communication)
├── studer_sensor.h      # Sensor class
└── scomlib/             # SCOM protocol library
    ├── scom_data_link.*
    ├── scom_property.*
    └── scomlib_extra.*
```

## Credits

Based on the [studer232-to-mqtt](https://github.com/kolins-cz/studer232-to-mqtt) project and [scomlib](https://github.com/k3a/studer) library.

## License

MIT License
