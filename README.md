# Studer232-to-MQTT

This program connects to a Studer XCom-232i device, retrieves data, and publishes them to an MQTT broker with Home Assistant auto-discovery support.

## Features

- Reads parameters from Studer Xtender inverters via XCom-232i serial interface
- Publishes data to MQTT broker
- **Home Assistant MQTT Discovery** - automatic sensor configuration
- Availability tracking based on serial port communication status
- Automatic reconnection on network failures
- Debug mode for troubleshooting serial communication

## Prerequisites

Install required dependencies:

```bash
# Debian/Ubuntu
sudo apt-get install libmosquitto-dev libjson-c-dev

# Or on other systems
# libmosquitto (MQTT client library)
# libjson-c (JSON library for discovery configs)
```

## Installation

1. Clone the repository:
```bash
git clone https://github.com/kolins-cz/studer232-to-mqtt.git
cd studer232-to-mqtt
```

2. Configure your setup in `src/main.h`:
   - Set your MQTT broker address
   - Add or remove parameters you want to monitor

3. Build the project:
```bash
make
```

4. (Optional) Install as a systemd service:
```bash
./install-service.sh
```

## Home Assistant Integration

This program supports **MQTT Discovery** for automatic sensor configuration in Home Assistant.

### Setup

1. Ensure MQTT integration is configured in Home Assistant
2. Start the program - sensors will automatically appear in Home Assistant
3. All sensors will be grouped under one device in MQTT integration
4. Entities will use clean names like "Studer 1 Input Active Power"

### Entity IDs

Sensors use the format `sensor.xtender_*`:
- `sensor.xtender_xt1_output_active_power` - Inverter 1 output power
- `sensor.xtender_l1_battery_current` - Phase L1 battery current
- `sensor.xtender_batt_voltage` - Battery voltage
- etc.

## Usage

### Running the Program

```bash
# Run directly
sudo bin/studer232-to-mqtt

# Or via systemd service
sudo systemctl start studer232-to-mqtt
sudo systemctl status studer232-to-mqtt
```

### Configuration

Edit `src/main.h` to customize:
- MQTT broker address and port
- Parameters to monitor (add/remove from `requested_parameters` array)
- Friendly names for sensors

### Debugging Serial Communication

To enable verbose serial communication debugging:

```bash
# Build with debug output
make debug

# Or directly with DEBUG flag
make DEBUG=1

# Normal build without debug output
make
```

When compiled with debug enabled, the program will output detailed information about:
- Serial port initialization and configuration
- All bytes written to the serial port (TX)
- All bytes read from the serial port (RX)
- Frame encoding/decoding steps
- Read/write operation status and errors

Debug output is sent to stderr and prefixed with `[SERIAL DEBUG]`, `[SERIAL ERROR]`, or `[SCOM DEBUG]`.

## MQTT Topics

### Discovery Topics
- `homeassistant/sensor/xtender_*/config` - Auto-discovery configuration (retained)

### State Topics
- `studer/XT/xt1_input_active_power` - Inverter power readings
- `studer/DC/batt_voltage` - DC measurements
- `studer/AC/l1_output_active_power` - AC phase measurements
- `studer/commstatus` - Availability status (`online`/`offline`)

### Availability

The program publishes its status to `studer/commstatus`:
- `online` - Serial communication is working and receiving valid data
- `offline` - Serial communication failed or program shut down gracefully

Home Assistant uses this topic to mark sensors as available/unavailable.

## Disclaimer

This program is vibe-coded and likely contains numerous bugs.

### Contributing

If you would like to contribute to the code, feel free to submit pull requests to improve it.

### Reference

This program is based on the [k3a/studer](https://github.com/k3a/studer) library.