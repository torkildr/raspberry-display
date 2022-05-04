BUILD_DIR:=build/
DEBUG_DIR:=debug/

SYSTEMCTL:=$(shell which systemctl)

.PHONY: release debug install

release: $(BUILD_DIR)
	meson compile -C $(BUILD_DIR)
debug: $(DEBUG_DIR)
	meson compile -C $(DEBUG_DIR)
$(BUILD_DIR):
	meson setup -Dbuildtype=release --prefix /usr $(BUILD_DIR)
$(DEBUG_DIR):
	meson setup -Dbuildtype=debug $(DEBUG_DIR)

clean:
	$(RM) -R $(BUILD_DIR)
	$(RM) -R $(DEBUG_DIR)

install:
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	# meson will handle install
	cd $(BUILD_DIR) && meson install
	# systemd files
	install -m0644 systemd/raspberry-display.service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/raspberry-display.service
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) reenable raspberry-display.service; fi
	@echo "\nAll files installed."
	@echo "\nYou can start the service with"
	@echo "  sudo systemctl restart raspberry-display"

uninstall:
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) stop raspberry-display.service; fi
	@if test -f "$(SYSTEMCTL)"; then $(SYSTEMCTL) disable raspberry-display.service; fi
	$(RM) /etc/systemd/system/raspberry-display.service
	$(RM) "/usr/bin/raspberry-display"
