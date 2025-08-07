RaspberryPi Wall mounted display
================================

## Overview
This project allows you to control the Sure P4 32x8 displays. Though, it is written for this specific version, it should pretty much work for all Holtek HT1632 based matrix displays.
 
The main goal is to give a generic, pluggable way to interface different kinds of applications with the display, without having to bother with the driver stuff.

This package provides an MQTT client that uses a modern state-based JSON protocol, making it easy to integrate with IoT systems, Home Assistant, and other MQTT-enabled applications.

## Application Design

The main application consists of two basic parts:

`curses-client`, an interactive client, probably best used to test the hardware setup and display capabilities.

`raspberry-display-mqtt` (or `mock-display-mqtt` for testing), the MQTT client that listens for state-based JSON commands and controls the display accordingly.

The mock version works without physical hardware and is perfect for test setups or debugging the API usage.

## MQTT Protocol

The display uses a state-based MQTT protocol where you send JSON payloads describing the desired display state to the `display/state` topic.

### Connection

The MQTT client connects to your MQTT broker and subscribes to:
- `display/state` - Primary topic for state-based commands
- `display/display` - Alternative topic for compatibility

### JSON State Schema

All commands are JSON objects that can contain multiple state properties:

```json
{
  "text": "string",           // Text to display (optional)
  "show_time": boolean,       // Show time (with or without text)
  "time_format": "string",    // Time format (default: "%H:%M" with text, "%H:%M:%S" time-only)
  "brightness": number,       // 0-15
  "scroll": "string",         // "enabled"/"disabled"/"reset"
  "clear": boolean,           // Clear display
  "quit": boolean             // Quit program
}
```

### Display Modes

The display supports three main modes based on the JSON fields:

1. **Text only**: `{"text": "Hello World!"}`
2. **Text with time**: `{"text": "Meeting soon", "show_time": true}`
3. **Time only**: `{"show_time": true}` (no text field)

### Usage Examples

#### Basic Text Display
```bash
mosquitto_pub -h localhost -t "display/state" -m '{"text":"Hello World!"}'
```

#### Text with Current Time
```bash
mosquitto_pub -h localhost -t "display/state" -m '{"text":"Meeting in 5 min", "show_time":true}'
```

#### Time Only Display
```bash
# Default format (%H:%M:%S)
mosquitto_pub -h localhost -t "display/state" -m '{"show_time":true}'

# Custom format
mosquitto_pub -h localhost -t "display/state" -m '{"show_time":true, "time_format":"%A, %b %-d %H:%M:%S"}'
```

#### Set Brightness
```bash
mosquitto_pub -h localhost -t "display/state" -m '{"brightness":10}'
```

#### Enable/Disable Scrolling
```bash
# Enable scrolling
mosquitto_pub -h localhost -t "display/state" -m '{"scroll":"enabled"}'

# Disable scrolling
mosquitto_pub -h localhost -t "display/state" -m '{"scroll":"disabled"}'

# Reset scroll position
mosquitto_pub -h localhost -t "display/state" -m '{"scroll":"reset"}'
```

#### Complex State Changes (Atomic)
```bash
# Set multiple properties at once
mosquitto_pub -h localhost -t "display/state" -m '{
  "text": "System Alert!", 
  "show_time": true, 
  "brightness": 15, 
  "scroll": "enabled"
}'
```

#### Clear Display
```bash
mosquitto_pub -h localhost -t "display/state" -m '{"clear":true}'
```

#### Quit Application
```bash
mosquitto_pub -h localhost -t "display/state" -m '{"quit":true}'
```

### Error Handling

The MQTT client provides helpful error messages:
- Invalid JSON shows parse errors with line/column information
- Unsupported topics show usage examples
- Invalid brightness values show acceptable range (0-15)

### Character Support

Currently, only a limited set of ASCII characters are supported. The display expects `iso-8859-1` encoding, and any unknown characters will be rendered as a single space. 

## Building
First install the system pre-requisites and the WiringPi library.

You might want to change the [display headers](src/ht1632.h) if you have a setup that is different than the reference setup.

If you're on a debian system, like raspbian, this is simply
```bash
./install-prereqs-debian.sh
./install-wiringPi.sh
```

Once this is done, you should be able to build the project with
```bash
make
```

## Installation

Installing the application will place the binary in a system wide location, and create systemd service files to make sure the process
is always running.

**NB:** The service will be installed as the current running user. For this to work, the user needs access to the SPI-device.

After you have built the application, you can install it with
```bash
sudo make install
```

This will install the `raspberry-display-mqtt` to `/usr/bin/raspberry-display-mqtt` and create systemd service files.

### Running the MQTT Client

To run the MQTT client manually:
```bash
# For real hardware
./build/raspberry-display-mqtt <mqtt_host> <mqtt_port> [client_id] [topic_prefix]

# For testing (mock display)
./build/mock-display-mqtt <mqtt_host> <mqtt_port> [client_id] [topic_prefix]

# Example
./build/mock-display-mqtt localhost 1883 raspberry-display display
```

#### Environment Variable Configuration

The MQTT client also supports configuration via environment variables (useful for systemd service):
```bash
# Set environment variables
export MQTT_HOST="localhost"
export MQTT_PORT="1883"
export CLIENT_ID="raspberry-display"
export TOPIC_PREFIX="display"

# Run without arguments
./build/raspberry-display-mqtt
```

To (re)start the systemd service, run
```bash
sudo systemctl restart raspberry-display
```

To view log files, you can use the systemd journal
```bash
sudo journalctl -f -u raspberry-display
```

### Fault Tolerance and Production Deployment

The systemd service is configured for high availability and fault tolerance:

#### Automatic Restart Policies
- **Always restart**: Service automatically restarts on any failure
- **Restart delay**: 10-second delay between restart attempts
- **Rate limiting**: Maximum 5 restarts within 5 minutes to prevent restart loops
- **Network dependency**: Waits for network connectivity before starting

#### Service Monitoring
```bash
# Check service status
sudo systemctl status raspberry-display

# View recent logs
sudo journalctl -u raspberry-display --since "1 hour ago"

# Follow logs in real-time
sudo journalctl -f -u raspberry-display

# Check restart history
sudo systemctl show raspberry-display -p NRestarts
```

#### MQTT Connection Resilience

**Current Behavior**: When MQTT connection is lost:
- ✅ Service automatically restarts via systemd
- ✅ Display continues showing last state during reconnection
- ✅ Automatic resubscription to topics on restart
- ✅ Configurable via environment variables

**Production Recommendations**:
1. **MQTT Broker High Availability**: Use clustered MQTT brokers
2. **Network Monitoring**: Monitor network connectivity
3. **Custom Restart Logic**: Consider implementing application-level reconnection for faster recovery

#### Security Features
- **Privilege dropping**: Runs as specified user, not root
- **Resource limits**: CPU, memory, and file descriptor limits
- **Filesystem isolation**: Private temp directory and system protection

To uninstall
```bash
sudo make uninstall
```

### MQTT Broker Setup

You'll need an MQTT broker running. For testing, you can install Mosquitto:

```bash
# Install Mosquitto MQTT broker
sudo apt update && sudo apt install -y mosquitto mosquitto-clients

# Start the broker
mosquitto -v
```

## Hardware

The application is designed to use `spi0` on the rasperry pi, and four generic GPIO pins. This can be changed, but for a basic
setup, just follow the reference connection.

All pins referenced are [physical pins](https://pinout.xyz/pinout#). The application uses WiringPi-numbering for CS-pins. These
will be different from the physical pins (use [this guide](https://pinout.xyz/pinout/wiringpi) for WiringPi numbering).

Pin | Usage
--- | -----
2   | 5V
6   | Ground
3   | CS1
5   | CS2
8   | CS3
10  | CS4
19  | MOSI/Data
23  | SCLK/WR

### Reference hardware
![Example Wiring](images/raspberry-wiring.png)

## Acknowledgements
This project is written by Torkild Retvedt, as a rewrite of a similar project for the [Arduino UNO](https://github.com/torkildr/display)

Much of the ht1632 driver is inspired by [this repo](https://github.com/DerBer/ht1632clib).

## License
This software is licensed under the MIT license, unless specified.

