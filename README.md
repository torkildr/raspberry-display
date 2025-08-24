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

The display uses a state-based MQTT protocol where you send JSON payloads describing the desired display state to the `display/set` topic.

### Connection

The MQTT client connects to your MQTT broker and subscribes to:
- `display/set` - Set the full display sequence state
- `display/add` - Add state to the display sequence
- `display/clear` - Clear the display sequence
- `display/quit` - Quit the application

### JSON State Schema

All commands are JSON objects that can contain multiple state properties:

```
{
  "text": "string",           // Text to display (optional)
  "show_time": boolean,       // Show time (with or without text)
  "time_format": "string",    // Time format, strftime(3) compatible
  "brightness": number,       // 0-15
  "scroll": "string",         // "enabled"/"disabled"/"reset"
  "alignment": "string",      // "left"/"center" - text alignment (default: "left")
  "transition": "string" | {  // Transition effect (optional)
    "type": "string",         // Transition type
    "duration": number        // Duration in seconds (optional)
  }
}
```

#### Transition Types

Available transition effects:
- `"none"` - No transition (default)
- `"wipe_left"` - Wipe from right to left
- `"wipe_right"` - Wipe from left to right
- `"dissolve"` - Fade effect
- `"scroll_up"` - Scroll content upward
- `"scroll_down"` - Scroll content downward
- `"split_center"` - Split from center outward
- `"split_sides"` - Split from sides inward
- `"random"` - Random transition selection

Transitions can be specified as:
- Simple string: `"transition": "dissolve"`
- Object with duration: `"transition": {"type": "wipe_left", "duration": 1.5}`

### Display Modes

The display supports three main modes based on the JSON fields:

1. **Text only**: `{"text": "Hello World!"}`
2. **Text with time**: `{"text": "Meeting soon", "show_time": true}`
3. **Time only**: `{"show_time": true}` (no text field)

### Usage Examples

Here are practical examples using `mosquitto_pub` to control the display:

#### Basic Display Commands

**Show simple text:**
```bash
mosquitto_pub -h localhost -t display/set -m '[{"state": {"text": "Hello World!"}, "time": 5.0}]'
```

**Show current time:**
```bash
mosquitto_pub -h localhost -t display/set -m '[{"state": {"show_time": true}, "time": 10.0}]'
```

**Show text with custom time format and dissolve transition:**
```bash
mosquitto_pub -h localhost -t display/set -m '[{"state": {"text": "Meeting at", "show_time": true, "time_format": "%H:%M", "transition": "dissolve"}, "time": 8.0}]'
```

**Centered text with custom brightness and wipe transition:**
```bash
mosquitto_pub -h localhost -t display/set -m '[{"state": {"text": "WELCOME", "alignment": "center", "brightness": 10, "transition": {"type": "wipe_left", "duration": 1.5}}, "time": 3.0}]'
```

#### Adding to Sequence (display/add)

**Add a single message to the sequence:**
```bash
mosquitto_pub -h localhost -t display/add -m '{"state": {"text": "New Message"}, "time": 4.0}'
```

**Add with TTL (auto-expire after 30 seconds):**
```bash
mosquitto_pub -h localhost -t display/add -m '{"state": {"text": "Temporary Alert"}, "time": 2.0, "ttl": 30.0}'
```

**Add with sequence ID and split transition:**
```bash
mosquitto_pub -h localhost -t display/add -m '{"state": {"text": "Meeting Room A", "transition": "split_center"}, "time": 5.0, "id": "meeting_alerts"}'
```

#### Setting Complete Sequences (display/set)

**Multi-step sequence with transitions:**
```bash
mosquitto_pub -h localhost -t display/set -m '[
  {"state": {"text": "Step 1", "alignment": "center", "transition": "scroll_down"}, "time": 3.0, "id": "meeting_alerts1"},
  {"state": {"text": "Step 2", "brightness": 8, "transition": "wipe_right"}, "time": 3.0, "id": "meeting_alerts2"},
  {"state": {"show_time": true, "transition": "dissolve"}, "time": 5.0, "id": "meeting_alerts3"}
]'
```

**Scrolling text sequence:**
```bash
mosquitto_pub -h localhost -t display/set -m '[
  {"state": {"text": "This is a very long message that will scroll across the display", "scroll": "enabled"}, "time": 10.0, "id": "meeting_alerts1"},
  {"state": {"text": "Normal text", "scroll": "disabled"}, "time": 3.0, "id": "meeting_alerts2"}
]'
```

#### Clearing Display (display/clear)

**Clear all sequences:**
```bash
mosquitto_pub -h localhost -t display/clear -m '{}'
```

**Clear specific sequence by ID:**
```bash
mosquitto_pub -h localhost -t display/clear -m '{"id": "meeting_alerts"}'
```

#### Quit Application (display/quit)

**Gracefully stop the MQTT client:**
```bash
mosquitto_pub -h localhost -t display/quit -m '{}'
```

#### Advanced Examples

**Weather display with smooth transitions:**
```bash
mosquitto_pub -h localhost -t display/set -m '[
  {"state": {"text": "Weather", "alignment": "center", "brightness": 12, "transition": {"type": "dissolve", "duration": 2.0}}, "time": 2.0, "id": "weather1"},
  {"state": {"text": "22°C Sunny", "show_time": true, "time_format": "%H:%M", "transition": "scroll_up"}, "time": 8.0, "id": "weather2"}
]'
```

**Status board rotation with varied transitions:**
```bash
mosquitto_pub -h localhost -t display/set -m '[
  {"state": {"text": "Server: OK", "alignment": "left", "transition": "wipe_left"}, "time": 4.0, "id": "status1"},
  {"state": {"text": "DB: OK", "alignment": "left", "transition": "wipe_right"}, "time": 4.0, "id": "status2"},
  {"state": {"text": "API: OK", "alignment": "left", "transition": "split_sides"}, "time": 4.0, "id": "status3"},
  {"state": {"show_time": true, "transition": "random"}, "time": 6.0, "id": "status4"}
]'
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

### TODO
- Home assistant MQTT
