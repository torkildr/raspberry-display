# Raspberry Display - Single Comprehensive Makefile
# Features: Proper dependency tracking, aggressive warnings, warnings as errors

# Compiler settings
CXX = g++
CC = gcc

# Build type (can be overridden: make BUILD_TYPE=debug)
BUILD_TYPE ?= release

# System variables
SYSTEMCTL := $(shell which systemctl)
BINARY_NAME := raspberry-display-mqtt
SERVICE_NAME := raspberry-display

# Directories
BUILD_DIR := build
DEBUG_BUILD_DIR := debug-build
OBJ_DIR := $(if $(filter debug,$(BUILD_TYPE)),$(DEBUG_BUILD_DIR),$(BUILD_DIR))

# Optimization and architecture flags
ifeq ($(BUILD_TYPE),debug)
    OPT_FLAGS = -O0 -g -DDEBUG
else
    OPT_FLAGS = -O2 -DNDEBUG
endif

# Architecture detection and optimization
ARCH := $(shell uname -m)
ifeq ($(ARCH),armv6l)
    # Pi Zero specific optimizations
    ARCH_FLAGS = -march=armv6 -mfpu=vfp -mfloat-abi=hard
else ifeq ($(ARCH),armv7l)
    # Pi 2/3/4 32-bit optimizations
    ARCH_FLAGS = -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard
else ifeq ($(ARCH),aarch64)
    # Pi 3/4 64-bit optimizations
    ARCH_FLAGS = -march=armv8-a
else
    # Generic flags for other architectures
    ARCH_FLAGS = -march=native
endif

# Comprehensive warning flags (matching meson configuration)
WARNING_FLAGS = -Wall -Wextra -Wpedantic -Werror \
    -Wconversion -Wsign-conversion -Wunused -Wuninitialized \
    -Wundef -Wcast-align -Wcast-qual -Wold-style-cast \
    -Woverloaded-virtual -Wmissing-declarations -Wredundant-decls \
    -Wdouble-promotion -Wformat=2 -Wformat-security \
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict \
    -Wno-psabi

# Compiler flags
CXXFLAGS = -std=c++17 $(OPT_FLAGS) $(ARCH_FLAGS) $(WARNING_FLAGS)
CFLAGS = $(OPT_FLAGS) $(ARCH_FLAGS) $(WARNING_FLAGS)

# Include directories
INCLUDES = -Isrc

# Library flags
THREAD_LIBS = -lpthread
NCURSES_LIBS = -lncurses
WIRINGPI_LIBS = -lwiringPi
MOSQUITTO_LIBS = -lmosquitto
MQTT_LIBS = $(MOSQUITTO_LIBS) $(THREAD_LIBS)
BASIC_LIBS = $(NCURSES_LIBS) $(WIRINGPI_LIBS) $(THREAD_LIBS)

# Source files
COMMON_SRC = src/timer.cpp src/display.cpp src/font.cpp
MQTT_SRC = src/mqtt-client.cpp
CURSES_SRC = src/curses-client.cpp
HT1632_SRC = src/ht1632.cpp
MOCK_SRC = src/mock-display.cpp

# Object files with proper directory structure
COMMON_OBJ = $(COMMON_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
MQTT_OBJ = $(MQTT_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
CURSES_OBJ = $(CURSES_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
HT1632_OBJ = $(HT1632_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
MOCK_OBJ = $(MOCK_SRC:src/%.cpp=$(OBJ_DIR)/%.o)

# Targets
TARGETS = $(OBJ_DIR)/raspberry-display-mqtt $(OBJ_DIR)/curses-client $(OBJ_DIR)/mock-display-mqtt $(OBJ_DIR)/mock-curses-client

# Phony targets
.PHONY: all release debug clean install uninstall test font-extract font-generate help

# Default target
all: release

# Build type targets
release:
	@$(MAKE) BUILD_TYPE=release $(BUILD_DIR)/raspberry-display-mqtt $(BUILD_DIR)/curses-client $(BUILD_DIR)/mock-display-mqtt $(BUILD_DIR)/mock-curses-client

debug:
	@$(MAKE) BUILD_TYPE=debug $(DEBUG_BUILD_DIR)/raspberry-display-mqtt $(DEBUG_BUILD_DIR)/curses-client $(DEBUG_BUILD_DIR)/mock-display-mqtt $(DEBUG_BUILD_DIR)/mock-curses-client

# Create build directories
$(BUILD_DIR):
	mkdir -p $@

$(DEBUG_BUILD_DIR):
	mkdir -p $@

# Font generation (required before building)
src/font_generated.h: tools/font_definitions.txt tools/font_generator.py
	@echo "Generating font header..."
	python3 tools/font_generator.py tools/font_definitions.txt src/font_generated.h

# Executable targets with proper dependencies
$(OBJ_DIR)/raspberry-display-mqtt: $(OBJ_DIR) src/font_generated.h $(COMMON_OBJ) $(MQTT_OBJ) $(HT1632_OBJ)
	@echo "Linking raspberry-display-mqtt..."
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ) $(MQTT_OBJ) $(HT1632_OBJ) $(BASIC_LIBS) $(MQTT_LIBS)

$(OBJ_DIR)/curses-client: $(OBJ_DIR) src/font_generated.h $(COMMON_OBJ) $(CURSES_OBJ) $(HT1632_OBJ)
	@echo "Linking curses-client..."
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ) $(CURSES_OBJ) $(HT1632_OBJ) $(BASIC_LIBS)

$(OBJ_DIR)/mock-display-mqtt: $(OBJ_DIR) src/font_generated.h $(COMMON_OBJ) $(MQTT_OBJ) $(MOCK_OBJ)
	@echo "Linking mock-display-mqtt..."
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ) $(MQTT_OBJ) $(MOCK_OBJ) $(BASIC_LIBS) $(MQTT_LIBS)

$(OBJ_DIR)/mock-curses-client: $(OBJ_DIR) src/font_generated.h $(COMMON_OBJ) $(CURSES_OBJ) $(MOCK_OBJ)
	@echo "Linking mock-curses-client..."
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ) $(CURSES_OBJ) $(MOCK_OBJ) $(BASIC_LIBS)

# Convenience targets that point to the actual binaries
raspberry-display-mqtt: $(OBJ_DIR)/raspberry-display-mqtt
curses-client: $(OBJ_DIR)/curses-client
mock-display-mqtt: $(OBJ_DIR)/mock-display-mqtt
mock-curses-client: $(OBJ_DIR)/mock-curses-client

# Object file compilation with automatic header dependency tracking
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files for header tracking
-include $(OBJ_DIR)/*.d

# Test target
test: $(OBJ_DIR)/mock-display-mqtt
	@echo "Running mock MQTT client for testing..."
	@echo "Make sure you have an MQTT broker running on localhost:1883"
	./$(OBJ_DIR)/mock-display-mqtt localhost 1883

clean:
	$(RM) -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR)
	$(RM) -f src/font_generated.h

install: $(OBJ_DIR)/raspberry-display-mqtt
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	# Install binary
	sudo cp $(OBJ_DIR)/raspberry-display-mqtt /usr/bin/$(BINARY_NAME)
	sudo chmod +x /usr/bin/$(BINARY_NAME)
	# Install systemd service file
	sudo install -m0644 systemd/$(SERVICE_NAME).service /etc/systemd/system/
	sudo sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/$(SERVICE_NAME).service
	# Enable and reload systemd
	@if test -f "$(SYSTEMCTL)"; then sudo $(SYSTEMCTL) daemon-reload; fi
	@if test -f "$(SYSTEMCTL)"; then sudo $(SYSTEMCTL) enable $(SERVICE_NAME).service; fi
	@echo "\nInstallation complete!"
	@echo "\nThe MQTT display client has been installed as: /usr/bin/$(BINARY_NAME)"
	@echo "\nYou can start the service with:"
	@echo "  sudo systemctl start $(SERVICE_NAME)"
	@echo "\nView logs with:"
	@echo "  sudo journalctl -f -u $(SERVICE_NAME)"

uninstall:
	# Stop and disable service
	@if test -f "$(SYSTEMCTL)"; then sudo $(SYSTEMCTL) stop $(SERVICE_NAME).service; fi
	@if test -f "$(SYSTEMCTL)"; then sudo $(SYSTEMCTL) disable $(SERVICE_NAME).service; fi
	# Remove files
	sudo rm -f /etc/systemd/system/$(SERVICE_NAME).service
	sudo rm -f /usr/bin/$(BINARY_NAME)
	# Reload systemd
	@if test -f "$(SYSTEMCTL)"; then sudo $(SYSTEMCTL) daemon-reload; fi
	@echo "\nUninstallation complete!"
	@echo "Service $(SERVICE_NAME) has been stopped, disabled, and removed."

# Font management targets
font-extract:
	@echo "Extracting current font to ASCII art format..."
	python3 tools/extract_font.py src/font.h tools/font_definitions.txt
	@echo "Font extracted to tools/font_definitions.txt"
	@echo "You can now edit the ASCII art and regenerate with 'make font-generate'"

font-generate:
	@echo "Generating font header from ASCII art definitions..."
	python3 tools/font_generator.py tools/font_definitions.txt src/font_generated.h
	@echo "Generated src/font_generated.h"
	@echo "Font ready for build - no additional steps needed"

# Help target
help:
	@echo "Raspberry Display - Single Comprehensive Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all                    - Build all targets (default: release)"
	@echo "  release                - Build optimized release binaries"
	@echo "  debug                  - Build debug binaries with symbols"
	@echo "  raspberry-display-mqtt - MQTT client for real hardware"
	@echo "  curses-client          - Interactive client for real hardware"
	@echo "  mock-display-mqtt      - MQTT client with mock display"
	@echo "  mock-curses-client     - Interactive client with mock display"
	@echo "  test                   - Run mock MQTT client for testing"
	@echo "  install                - Install MQTT client and systemd service"
	@echo "  uninstall              - Remove installed files and service"
	@echo "  clean                  - Remove build artifacts"
	@echo "  font-extract           - Extract font to ASCII art format"
	@echo "  font-generate          - Generate font header from ASCII art"
	@echo "  help                   - Show this help message"
	@echo ""
	@echo "Build options:"
	@echo "  BUILD_TYPE=release     - Optimized build (default)"
	@echo "  BUILD_TYPE=debug       - Debug build with symbols"
	@echo ""
	@echo "Examples:"
	@echo "  make                   - Build release version"
	@echo "  make BUILD_TYPE=debug  - Build debug version"
	@echo "  make test              - Test with mock MQTT client"
