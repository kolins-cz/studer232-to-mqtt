# Studer232-to-MQTT

This program connects to a Studer XCom-232i device, retrieves data, and publishes them to an MQTT broker.

### Usage
- Look at `main.h` set up your MQTT server and feel free to add or remove parameters you would like to retrieve.
- Compile with `make` and you can optionally install a systemd service with `./install-service.sh`
- `homeassistant.yaml` and `lovelace.yaml` are provided to simplify adding this to Home Assistant.

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

### Contributing

If you would like to contribute to the code, feel free to submit pull requests to improve it.

### Reference

This program is based on the [k3a/studer](https://github.com/k3a/studer) library.