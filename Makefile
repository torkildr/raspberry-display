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
CXXFLAGS = -std=c++20 $(OPT_FLAGS) $(ARCH_FLAGS) $(WARNING_FLAGS)
CFLAGS = $(OPT_FLAGS) $(ARCH_FLAGS) $(WARNING_FLAGS)

# Include directories
INCLUDES = -Isrc -Isrc/util -Isrc/display

# Library flags
THREAD_LIBS = -lpthread
NCURSES_LIBS = -lncurses
WIRINGPI_LIBS = -lwiringPi
MOSQUITTO_LIBS = -lmosquitto
MQTT_LIBS = $(MOSQUITTO_LIBS) $(THREAD_LIBS)
BASIC_LIBS = $(NCURSES_LIBS) $(WIRINGPI_LIBS) $(THREAD_LIBS)

# Source files organized by dependency layers - auto-discover with wildcards
UTIL_SRC = $(wildcard src/util/*.cpp)
DISPLAY_SRC = $(wildcard src/display/*.cpp)
MQTT_SRC = src/mqtt-client.cpp src/ha_discovery.cpp
CURSES_SRC = src/curses-client.cpp
HT1632_SRC = src/ht1632.cpp
MOCK_SRC = src/mock-display.cpp

# Object files with proper directory structure
UTIL_OBJ = $(UTIL_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
DISPLAY_OBJ = $(DISPLAY_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
MQTT_OBJ = $(MQTT_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
CURSES_OBJ = $(CURSES_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
HT1632_OBJ = $(HT1632_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
MOCK_OBJ = $(MOCK_SRC:src/%.cpp=$(OBJ_DIR)/%.o)

# Targets
TARGETS = $(OBJ_DIR)/raspberry-display-mqtt $(OBJ_DIR)/curses-client $(OBJ_DIR)/mock-display-mqtt $(OBJ_DIR)/mock-curses-client

# Phony targets
.PHONY: all release debug clean install install-service uninstall test font-generate help compile_commands

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
src/display/font_generated.hpp: tools/font_definitions.txt tools/font_generator.py
	@echo "Generating font header..."
	python3 tools/font_generator.py tools/font_definitions.txt src/display/font_generated.hpp

# Executable targets with proper dependencies
$(OBJ_DIR)/raspberry-display-mqtt: $(OBJ_DIR) src/display/font_generated.hpp $(UTIL_OBJ) $(DISPLAY_OBJ) $(MQTT_OBJ) $(HT1632_OBJ)
	@echo "Linking raspberry-display-mqtt..."
	$(CXX) $(CXXFLAGS) -o $@ $(UTIL_OBJ) $(DISPLAY_OBJ) $(MQTT_OBJ) $(HT1632_OBJ) $(BASIC_LIBS) $(MQTT_LIBS)

$(OBJ_DIR)/curses-client: $(OBJ_DIR) src/display/font_generated.hpp $(UTIL_OBJ) $(DISPLAY_OBJ) $(CURSES_OBJ) $(HT1632_OBJ)
	@echo "Linking curses-client..."
	$(CXX) $(CXXFLAGS) -o $@ $(UTIL_OBJ) $(DISPLAY_OBJ) $(CURSES_OBJ) $(HT1632_OBJ) $(BASIC_LIBS)

$(OBJ_DIR)/mock-display-mqtt: $(OBJ_DIR) src/display/font_generated.hpp $(UTIL_OBJ) $(DISPLAY_OBJ) $(MQTT_OBJ) $(MOCK_OBJ)
	@echo "Linking mock-display-mqtt..."
	$(CXX) $(CXXFLAGS) -o $@ $(UTIL_OBJ) $(DISPLAY_OBJ) $(MQTT_OBJ) $(MOCK_OBJ) $(BASIC_LIBS) $(MQTT_LIBS)

$(OBJ_DIR)/mock-curses-client: $(OBJ_DIR) src/display/font_generated.hpp $(UTIL_OBJ) $(DISPLAY_OBJ) $(CURSES_OBJ) $(MOCK_OBJ)
	@echo "Linking mock-curses-client..."
	$(CXX) $(CXXFLAGS) -o $@ $(UTIL_OBJ) $(DISPLAY_OBJ) $(CURSES_OBJ) $(MOCK_OBJ) $(BASIC_LIBS)

# Convenience targets that point to the actual binaries
raspberry-display-mqtt: $(OBJ_DIR)/raspberry-display-mqtt
curses-client: $(OBJ_DIR)/curses-client
mock-display-mqtt: $(OBJ_DIR)/mock-display-mqtt
mock-curses-client: $(OBJ_DIR)/mock-curses-client

# Object file compilation with automatic header dependency tracking
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Handle subdirectory compilation
$(OBJ_DIR)/util/%.o: src/util/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(OBJ_DIR)/util
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJ_DIR)/display/%.o: src/display/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(OBJ_DIR)/display
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files for header tracking - include subdirectories
-include $(OBJ_DIR)/*.d
-include $(OBJ_DIR)/*/*.d

clean:
	$(RM) -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR)
	$(RM) -f src/display/font_generated.hpp

# Testing - auto-discover all test files
TEST_SRC = $(wildcard src/test/*.cpp)
TEST_OBJ = $(TEST_SRC:src/%.cpp=$(OBJ_DIR)/%.o)
TEST_LIBS = $(MQTT_LIBS) $(BASIC_LIBS) -lCatch2Main -lCatch2
TEST_EXECUTABLES = $(TEST_SRC:src/test/%.cpp=$(OBJ_DIR)/test_%)

# Test target - build and run all test executables
test: $(TEST_EXECUTABLES)
	@echo "Running all tests..."
	@for test_exe in $(TEST_EXECUTABLES); do \
		echo "Running $$test_exe..."; \
		$$test_exe || exit 1; \
	done

# Generic test executable rule
$(OBJ_DIR)/test_%: $(OBJ_DIR) src/display/font_generated.hpp $(OBJ_DIR)/test/%.o $(UTIL_OBJ) $(DISPLAY_OBJ) $(MOCK_OBJ)
	@echo "Linking test executable $@..."
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/test/$*.o $(UTIL_OBJ) $(DISPLAY_OBJ) $(MOCK_OBJ) $(TEST_LIBS)

# Handle test subdirectory compilation
$(OBJ_DIR)/test/%.o: src/test/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(OBJ_DIR)/test
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@


# Generate compile_commands.json for LSP support
compile_commands:
	@echo "Generating compile_commands.json..."
	bear -- make -j 16 -B
	bear --append -- make test -j 16 -B

install: $(OBJ_DIR)/raspberry-display-mqtt
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	cp $(OBJ_DIR)/raspberry-display-mqtt /usr/bin/$(BINARY_NAME)
	chmod +x /usr/bin/$(BINARY_NAME)
	@echo "\nThe MQTT display client has been installed as: /usr/bin/$(BINARY_NAME)"

install-service:
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, service installation will probably not work as intended !!!\n\n"; fi
	@if test ! -f "/usr/bin/$(BINARY_NAME)"; then echo "\n\n!!! Binary not found. Run 'make install' first !!!\n\n"; exit 1; fi
	install -m0644 systemd/$(SERVICE_NAME).service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/$(SERVICE_NAME).service
	mkdir -p /etc/systemd/system/$(SERVICE_NAME).service.d/
	@if test ! -f "/etc/systemd/system/$(SERVICE_NAME).service.d/environment.conf"; then \
		echo "Installing default environment configuration..."; \
		install -m0644 systemd/environment.conf /etc/systemd/system/$(SERVICE_NAME).service.d/; \
		echo "Generating unique Home Assistant device ID..."; \
		DEVICE_UUID=$$(python3 -c "import uuid; print(str(uuid.uuid4())[:8])"); \
		echo "Environment=\"HA_DEVICE_ID=$$DEVICE_UUID\"" >> /etc/systemd/system/$(SERVICE_NAME).service.d/environment.conf; \
		echo "Generated HA_DEVICE_ID: $$DEVICE_UUID"; \
	else \
		echo "Environment configuration already exists, skipping to preserve user settings."; \
	fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) daemon-reload; fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) enable $(SERVICE_NAME).service; fi
	@echo "\nTo customize MQTT settings, edit the environment variables in:"
	@echo "  /etc/systemd/system/$(SERVICE_NAME).service.d/environment.conf"

uninstall:
	# Stop and disable service
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) stop $(SERVICE_NAME).service; fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) disable $(SERVICE_NAME).service; fi
	# Remove files
	rm -f /etc/systemd/system/$(SERVICE_NAME).service
	rm -rf /etc/systemd/system/$(SERVICE_NAME).service.d/
	rm -f /usr/bin/$(BINARY_NAME)
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) daemon-reload; fi
	@echo "\nUninstallation complete!"
	@echo "Service $(SERVICE_NAME) has been stopped, disabled, and removed."

font-generate:
	@echo "Generating font header from ASCII art definitions..."
	python3 tools/font_generator.py tools/font_definitions.txt src/display/font_generated.hpp
	@echo "Generated src/display/font_generated.hpp"
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
	@echo "  install                - Install MQTT client binary only"
	@echo "  install-service        - Install and configure systemd service"
	@echo "  uninstall              - Remove installed files and service"
	@echo "  clean                  - Remove build artifacts"
	@echo "  font-generate          - Generate font header from ASCII art"
	@echo "  compile_commands       - Generate compile_commands.json for LSPs"
	@echo "  help                   - Show this help message"
	@echo ""
	@echo "Build options:"
	@echo "  BUILD_TYPE=release     - Optimized build (default)"
	@echo "  BUILD_TYPE=debug       - Debug build with symbols"
	@echo ""
	@echo "Examples:"
	@echo "  make                   - Build release version"
	@echo "  make BUILD_TYPE=debug  - Build debug version"
	@echo "  make test              - Build and run tests"
