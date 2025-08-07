BUILD_DIR:=build/
DEBUG_DIR:=debug/

SYSTEMCTL:=$(shell which systemctl)
BINARY_NAME:=raspberry-display-mqtt
SERVICE_NAME:=raspberry-display

.PHONY: all release debug clean install uninstall test

# Default target
all: release

# Build targets
release: $(BUILD_DIR)
	meson compile -C $(BUILD_DIR)

debug: $(DEBUG_DIR)
	meson compile -C $(DEBUG_DIR)

$(BUILD_DIR):
	meson setup -Dbuildtype=release --prefix /usr $(BUILD_DIR)

$(DEBUG_DIR):
	meson setup -Dbuildtype=debug $(DEBUG_DIR)

# Test target
test: debug
	@echo "Running mock MQTT client for testing..."
	@echo "Make sure you have an MQTT broker running on localhost:1883"
	./$(DEBUG_DIR)/mock-display-mqtt localhost 1883

clean:
	$(RM) -R $(BUILD_DIR)
	$(RM) -R $(DEBUG_DIR)

install: release
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	# Install binaries using meson
	cd $(BUILD_DIR) && meson install
	# Install systemd service file
	install -m0644 systemd/$(SERVICE_NAME).service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/$(SERVICE_NAME).service
	# Enable and reload systemd
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) daemon-reload; fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) enable $(SERVICE_NAME).service; fi
	@echo "\nInstallation complete!"
	@echo "\nThe MQTT display client has been installed as: /usr/bin/$(BINARY_NAME)"
	@echo "\nYou can start the service with:"
	@echo "  sudo systemctl start $(SERVICE_NAME)"
	@echo "\nView logs with:"
	@echo "  sudo journalctl -f -u $(SERVICE_NAME)"

uninstall:
	# Stop and disable service
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) stop $(SERVICE_NAME).service; fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) disable $(SERVICE_NAME).service; fi
	# Remove files
	$(RM) /etc/systemd/system/$(SERVICE_NAME).service
	$(RM) "/usr/bin/$(BINARY_NAME)"
	$(RM) "/usr/bin/curses-client"
	# Reload systemd
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) daemon-reload; fi
	@echo "\nUninstallation complete!"
