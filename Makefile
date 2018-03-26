BINDIR:=bin

PROGRAM_CURSES:=$(BINDIR)/curses-client
PROGRAM_FIFO:=$(BINDIR)/fifo-client
PROGRAM_MOCK_CURSES:=$(BINDIR)/mock-curses-client
PROGRAM_MOCK_FIFO:=$(BINDIR)/mock-fifo-client

WARNINGS:=-Wall -Werror
LDFLAGS:=-lrt -lwiringPi -lncurses -lcrypt -lpthread -lm
CFLAGS:=-std=gnu99 $(WARNINGS) $(LDFLAGS)

C_SRCS:=$(wildcard src/*.c)
DEPFILES:=${C_SRCS:.c=.d}
OBJECTS:=${C_SRCS:.c=.o}

REAL_DISPLAY:=src/display.o src/ht1632.o
MOCK_DISPLAY:=src/display.o src/mock-display.o

SYSTEMCTL:=$(shell which systemctl)

.PHONY: build clean

build: $(BINDIR)/ $(PROGRAM_CURSES) $(PROGRAM_FIFO) $(PROGRAM_MOCK_CURSES) $(PROGRAM_MOCK_FIFO)

debug: CFLAGS += -DDEBUG_ENABLED -g
debug: build

clean:
	$(RM) -R $(BINDIR)
	$(RM) $(OBJECTS)

$(BINDIR)/:
	mkdir -p $@

$(PROGRAM_CURSES): src/curses-client.o $(REAL_DISPLAY)
	cc -o $@ $^ $(CFLAGS)

$(PROGRAM_MOCK_CURSES): src/curses-client.o $(MOCK_DISPLAY)
	cc -o $@ $^ $(CFLAGS)

$(PROGRAM_FIFO): src/fifo-client.o $(REAL_DISPLAY)
	cc -o $@ $^ $(CFLAGS)

$(PROGRAM_MOCK_FIFO): src/fifo-client.o $(MOCK_DISPLAY)
	cc -o $@ $^ $(CFLAGS)

install:
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	install -m0755 $(PROGRAM_FIFO) "/usr/$(PROGRAM_FIFO)"
	install -m0644 systemd/raspberry-display.service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/raspberry-display.service
	$(SYSTEMCTL) reenable raspberry-display.service
	@echo "\nAll files installed.\nYou can start the service with \"sudo systemctl start raspberry-display\""

uninstall:
	$(SYSTEMCTL) stop raspberry-display.service
	$(SYSTEMCTL) disable raspberry-display.service
	$(RM) /etc/systemd/system/raspberry-display.service
	$(RM) "/usr/$(PROGRAM_FIFO)"

-include $(DEPFILES)
