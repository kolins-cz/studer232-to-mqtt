# Studer Reset Tool

A simple command-line tool to reset Studer devices to factory defaults via serial communication.

## Building

First, build the main project dependencies:
```bash
cd /home/kolin/studer232-to-mqtt
make
```

Then build the reset tool:
```bash
cd tools
make
```

## Usage

```bash
./studer_reset [serial_port] [option]
```

### Options

- `--system-reset` - **Reset ALL devices in the system** (RECOMMENDED)
  - Uses parameter 5121 at Xcom-232i address 501
  - Resets all Xtenders, VarioTracks, BSP, etc.

- `--xtender-all` - Reset ALL Xtenders to factory defaults
  - Uses multicast address 100
  - Broadcasts reset to all Xtender inverters

- `--xtender <addr>` - Reset specific Xtender at address 101-109
  - Example: `./studer_reset --xtender 101`

- `--xcom-defaults` - Restore default access levels on Xcom-232i
  - Uses parameter 5044 at address 501

### Examples

```bash
# Reset entire system (all devices)
./studer_reset --system-reset

# Reset all Xtender inverters
./studer_reset --xtender-all

# Reset specific Xtender at address 101
./studer_reset --xtender 101

# Use different serial port
./studer_reset /dev/ttyUSB0 --system-reset
```

### Default Settings

- **Serial port:** `/dev/serial/by-path/platform-xhci-hcd.1.auto-usb-0:1.1.1:1.0-port0`
- **Baud rate:** 115200
- **Parity:** Even
- **Stop bits:** 1

## ⚠️ Warning

**These operations will reset devices to factory defaults!**
All custom settings will be lost. The tool will ask for confirmation before proceeding.

## Parameters Used

| Device | Parameter | Level | Description |
|--------|-----------|-------|-------------|
| Xcom-232i | 5121 | Expert | Reset all devices of the system |
| Xcom-232i | 5044 | Installer | Restore default access level |
| Xtender | 1287 | Installer | Restore factory settings |
| Xtender | 1395 | Basic | Restore default settings |

## Troubleshooting

If the reset command fails:
1. Check serial port connection
2. Verify correct baud rate (115200 for most setups, 38400 for some XCom-232i)
3. Ensure devices are powered on and responding
4. Check cables and RS-232 connections

## Technical Details

The tool uses the SCOM protocol to send WRITE_PROPERTY commands with signal parameters. Signal parameters are triggered by writing any value (typically 1) to the parameter number.

Multicast addresses (like 100 for all Xtenders) only support WRITE operations, not READ.
