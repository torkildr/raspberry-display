# Raspberry Display - Transition System

The transition system provides smooth animated transitions between display states, supporting both MQTT and interactive curses control.

## Overview

Transitions animate the change from one display buffer state to another over a specified duration. The system is modular and self-contained, with each transition type implementing its own animation logic.

## Transition Types

### 1. **Wipe Transitions**
- **WIPE_LEFT**: Wipes from left to right with pattern "| || |||"
- **WIPE_RIGHT**: Wipes from right to left with pattern "| || |||"
- **Default Duration**: 1.0 seconds

### 2. **Dissolve Transition**  
- **DISSOLVE**: Random pixel-by-pixel reveal effect
- **Default Duration**: 1.5 seconds
- Uses randomized thresholds for organic dissolve appearance

### 3. **Scroll Transitions**
- **SCROLL_UP**: Entire screen scrolls up to reveal new content
- **SCROLL_DOWN**: Entire screen scrolls down to reveal new content  
- **Default Duration**: 0.8 seconds

### 4. **Split Transitions**
- **SPLIT_CENTER**: Reveals from center outward
- **SPLIT_SIDES**: Reveals from sides inward
- **Default Duration**: 1.0 seconds

## API Usage

### C++ Display Methods

```cpp
#include "display.hpp"
#include "transition.hpp"

// Basic usage with transition
disp.show("Hello World!", transition::Type::WIPE_LEFT, 1.0);
disp.showTime("", transition::Type::DISSOLVE, 2.0);
disp.showTime("%H:%M", "Status text", transition::Type::SCROLL_UP, 1.5);

// Set default transition for automatic use
disp.setTransition(transition::Type::DISSOLVE, 1.5);

// Check if transition is active
bool transitioning = disp.isTransitioning();
```

### MQTT Control

#### Simple String Format
```json
{
  "text": "Hello World!",
  "transition": "wipe_left"
}
```

#### Object Format with Custom Duration
```json
{
  "text": "Hello World!",
  "transition": {
    "type": "dissolve",
    "duration": 2.5
  }
}
```

#### Time Display with Transition
```json
{
  "show_time": true,
  "time_format": "%H:%M:%S",
  "text": "Status message",
  "transition": "scroll_up"
}
```

### Curses Client Controls

The curses client includes interactive transition testing:

```
Transitions:
1: wipe left    2: wipe right
3: dissolve     4: scroll up
5: scroll down  6: split center
7: split sides
```

## Transition String Names

| Type | String Names |
|------|-------------|
| NONE | "none" |
| WIPE_LEFT | "wipe_left", "wipe-left" |  
| WIPE_RIGHT | "wipe_right", "wipe-right" |
| DISSOLVE | "dissolve" |
| SCROLL_UP | "scroll_up", "scroll-up" |
| SCROLL_DOWN | "scroll_down", "scroll-down" |
| SPLIT_CENTER | "split_center", "split-center" |
| SPLIT_SIDES | "split_sides", "split-sides" |

## Technical Details

### Architecture
- **TransitionBase**: Abstract base class for all transitions
- **TransitionFactory**: Creates transition instances by type
- **TransitionManager**: Handles transition lifecycle and display updates
- **Frame-based Animation**: Uses delta time for smooth, consistent animation

### Performance
- Optimized for 15 Hz refresh rate (matches REFRESH_RATE constant)
- Memory efficient - transitions operate on existing display buffers
- No dynamic allocation during animation

### Integration
- Works seamlessly with existing scrolling, alignment, and brightness features
- Compatible with all display modes (TIME, TEXT, TIME_AND_TEXT)
- Thread-safe display buffer updates

## Examples

### MQTT Examples

```bash
# Basic wipe transition
mosquitto_pub -t "display/set" -m '{"text": "New Message!", "transition": "wipe_left"}'

# Dissolve with custom duration
mosquitto_pub -t "display/set" -m '{"text": "Fade In", "transition": {"type": "dissolve", "duration": 3.0}}'

# Time display with scroll
mosquitto_pub -t "display/set" -m '{"show_time": true, "transition": "scroll_up"}'

# Complex example with all features
mosquitto_pub -t "display/set" -m '{
  "text": "Centered Text",
  "alignment": "center",
  "scroll": "disabled", 
  "brightness": 10,
  "transition": {
    "type": "split_center",
    "duration": 1.5
  }
}'


# Sequence of states
  mosquitto_pub -t "display/setSequence" -m '[
    {
      "time": 5,
      "ttl": 120,
      "state": {"show_time": true, "text": "Some temporary state", "alignment": "left", "transition": "random"}
    },
    {
      "time": 5,
      "state": {"show_time": true, "text": "Some permanent state", "alignment": "left", "transition": "random"}
    },
    {
      "time": 5,
      "ttl": 1800,
      "state": {"text": "Some longer living state", "transition": "random", "alignment": "center"}
    },
    {
      "time": 5,
      "state": {"show_time": true, "transition": "random", "alignment": "center"}
    }
  ]'
```

### Programmatic Usage

```cpp
// Set up display with transition callback
auto disp = display::DisplayImpl(preUpdate, postUpdate);

// Configure default transition for all updates
disp.setTransition(transition::Type::DISSOLVE, 2.0);

// Explicit transition overrides default
disp.show("Important!", transition::Type::WIPE_LEFT, 0.5);

// Time display with transition
disp.showTime("%H:%M:%S", "System Ready", transition::Type::SCROLL_DOWN);

// Check transition status
while (disp.isTransitioning()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

## Building with Transitions

The transition system is automatically included in the build:

```bash
make clean && make          # Build all executables
make debug                  # Debug build with transitions
make curses-client         # Interactive transition testing
make mock-display-mqtt     # MQTT testing with transitions
```

## Tips

1. **Duration Guidelines**: 0.5-2.0 seconds work best for most transitions
2. **Performance**: Dissolve and wipe transitions are most CPU intensive
3. **UX**: Scroll transitions work well for status updates
4. **Testing**: Use curses client (keys 1-7) for interactive testing
5. **MQTT**: Object format allows per-message duration customization
