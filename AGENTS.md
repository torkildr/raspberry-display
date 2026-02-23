# raspberry-display

C++20 project that drives a Sure P4 32x8 LED matrix display (Holtek HT1632) via MQTT, targeting Raspberry Pi hardware. Integrates with Home Assistant and other IoT systems.

## Critical: Docker is the Only Build Environment

**Never run `make` or any compiler commands directly on the host machine.**

All build, test, and tool operations must happen inside the Docker container. The host machine will not have the required libraries (`libmosquitto`, `libwiringPi`, `Catch2`, `nlohmann/json`, etc.).

### Container image

```bash
# Build the image (only needed once, or when Dockerfile changes)
docker build -t raspberry-display .

# Generic run pattern for non-interactive commands
docker run --rm -v "$PWD:/code" raspberry-display <command>
```

The container mounts the project directory as `/code` and sets it as the working directory. All build artifacts land in the host's `build/` or `debug-build/` directories.

To detect docker vs podman (the project supports both):
```bash
{ command -v podman &>/dev/null && _c=podman || _c=docker; }
$_c build -q -t raspberry-display .
$_c run --rm -v "$PWD:/code" raspberry-display make
```

### Available custom commands

Use these slash commands instead of running Docker manually:

- `/build` — build all release targets
- `/build-debug` — build all debug targets
- `/test` — build and run all Catch2 tests
- `/clean` — remove all build artifacts
- `/font-generate` — regenerate `src/display/font_generated.hpp`

## Project Structure

```
src/
  mqtt-client.cpp       # Main entry point: MQTT client for real hardware
  mock-display.cpp      # Mock display implementation (no hardware required)
  curses-client.cpp     # Interactive ncurses client
  ht1632.cpp/.hpp       # HT1632 LED driver (SPI/GPIO, real hardware only)
  ha_discovery.cpp/.hpp # Home Assistant MQTT auto-discovery
  display/              # Core display logic (hardware-agnostic)
    display.cpp/.hpp    # Display abstraction and template interface
    font.cpp/.hpp       # Font rendering
    font_generated.hpp  # Auto-generated from tools/font_definitions.txt (git-ignored)
    sequence.cpp/.hpp   # Display sequence/playlist logic
    transition.cpp/.hpp # Visual transition effects
  util/                 # Utilities: timer, cyclic_list, log_util, utf8_converter
  test/                 # Catch2 unit tests
    test_cyclic_list.cpp
    test_display.cpp
    test_font.cpp
    test_sequence.cpp
    test_transition.cpp
tools/
  font_definitions.txt  # Human-readable ASCII art pixel font source
  font_generator.py     # Python3 script: ASCII art → C++ header
systemd/                # systemd service unit and environment config
```

## Build Targets

Run all commands inside the container: `docker run --rm -v "$PWD:/code" raspberry-display make <target>`

| Target | Description |
|---|---|
| `make` / `make release` | Build all four release binaries to `build/` |
| `make debug` | Build all four debug binaries to `debug-build/` |
| `make test` | Compile and run all Catch2 tests (uses mock display) |
| `make clean` | Remove `build/`, `debug-build/`, and `font_generated.hpp` |
| `make font-generate` | Regenerate `src/display/font_generated.hpp` |
| `make compile_commands` | Regenerate `compile_commands.json` for clangd LSP |
| `make install` | Install binary to `/usr/bin/` (Raspberry Pi only) |
| `make install-service` | Install and enable systemd service (Raspberry Pi only) |

### The four binaries

| Binary | Hardware | Interface | Use case |
|---|---|---|---|
| `raspberry-display-mqtt` | Real Pi (wiringPi) | MQTT | Production deployment |
| `curses-client` | Real Pi (wiringPi) | ncurses TUI | Hardware testing |
| `mock-display-mqtt` | None (software mock) | MQTT | Development / CI |
| `mock-curses-client` | None (software mock) | ncurses TUI | Manual UI testing |

**For development and testing, always use the `mock-*` targets.** The non-mock binaries require WiringPi and SPI hardware (Raspberry Pi GPIO).

## Testing

Tests live in `src/test/` and use the **Catch2** framework (v3). Run with:

```bash
docker run --rm -v "$PWD:/code" raspberry-display make test
```

- All tests link against `MOCK_OBJ` — no Raspberry Pi hardware needed
- Each `src/test/test_*.cpp` file is compiled into its own executable and run sequentially
- Tests fail the build if any assertion fails (`|| exit 1`)

When adding new functionality, add corresponding tests in `src/test/`.

## C++ Conventions

- **Standard**: C++20 (`-std=c++20`)
- **Warnings**: `-Wall -Wextra -Wpedantic -Werror` plus many more — **all warnings are errors**
- **Optimization**: `-O2 -DNDEBUG` for release, `-O0 -g -DDEBUG` for debug
- **Strict flags in use**: `-Wconversion`, `-Wsign-conversion`, `-Wcast-align`, `-Wold-style-cast`, `-Wformat=2`, etc.
- **No implicit conversions**: casts must be explicit; signed/unsigned mismatches are errors

Be conservative with casts. Prefer `static_cast` over C-style casts. The compiler is strict and will reject common implicit narrowing.

## Font Workflow

The display font is defined as ASCII art in `tools/font_definitions.txt`. The file `src/display/font_generated.hpp` is auto-generated from it and is **git-ignored**.

- `make` automatically regenerates `font_generated.hpp` if `font_definitions.txt` or `font_generator.py` is newer
- To manually regenerate: `make font-generate` (inside Docker)
- When editing font definitions, edit `tools/font_definitions.txt`, never `font_generated.hpp` directly

## Dependencies

All dependencies are pre-installed in the Docker image — there is no package manager to run:

| Library | Purpose |
|---|---|
| `libmosquitto` | MQTT client |
| `nlohmann/json` | JSON parsing (header-only) |
| `libwiringPi` | GPIO/SPI on Raspberry Pi |
| `libncurses` | ncurses TUI |
| `Catch2` v3 | Unit test framework |
| `bear` | Generates `compile_commands.json` |
| `clangd` | LSP server for IDE support |
| `valgrind` | Memory error detection |

## LSP / IDE Notes

`compile_commands.json` is committed and kept up to date. If it gets stale after significant refactoring, regenerate it:

```bash
docker run --rm -v "$PWD:/code" raspberry-display make compile_commands
```

Note: the host-side LSP (clangd) will report errors for headers like `mosquitto.h` and `catch2/` because those libraries are only installed inside Docker. These are false positives — the code is correct and compiles cleanly inside the container.

## MQTT Protocol Summary

The display is controlled by publishing JSON to MQTT topics:

- `display/set` — replace the full sequence (array of state objects)
- `display/add` — add a state to the sequence (single object)
- `display/clear` — clear sequence (optionally by `id`)
- `display/quit` — stop the application

State object fields: `text`, `show_time`, `time_format`, `brightness` (0–15), `scroll` (`"enabled"`/`"disabled"`/`"reset"`), `alignment` (`"left"`/`"center"`), `transition` (string or `{type, duration}`)

Example:
```bash
mosquitto_pub -h localhost -t display/set -m '[{"state": {"text": "Hello", "alignment": "center"}, "time": 5.0}]'
```

For testing with the mock MQTT client, start it inside the container and use `mosquitto_pub` / `mosquitto_sub` (also available in the container).
