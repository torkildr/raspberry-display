BINDIR:=bin

PROGRAM_CURSES:=$(BINDIR)/curses-client
PROGRAM_FIFO:=$(BINDIR)/fifo-client
PROGRAM_MOCK_CURSES:=$(BINDIR)/mock-curses-client
PROGRAM_MOCK_FIFO:=$(BINDIR)/mock-fifo-client

WARNINGS:=-Wall -Wextra -Werror
LDFLAGS:=-lstdc++ -lrt -lwiringPi -lncurses -lcrypt -lpthread -lm
CXXFLAGS:=-std=c++17 $(WARNINGS) -DDEBUG_ENABLED -g

CPP_SRCS:=$(wildcard src/*.cpp)
DEPFILES:=${CPP_SRCS:.cpp=.d}
OBJECTS:=${CPP_SRCS:.cpp=.o}

COMMON:=src/timer.o src/display.o src/font.o
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

$(PROGRAM_CURSES): src/curses-client.o $(REAL_DISPLAY) $(COMMON)
	cc -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(PROGRAM_MOCK_CURSES): src/curses-client.o $(MOCK_DISPLAY) $(COMMON)
	cc -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(PROGRAM_FIFO): src/fifo-client.o $(REAL_DISPLAY) $(COMMON)
	cc -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(PROGRAM_MOCK_FIFO): src/fifo-client.o $(MOCK_DISPLAY) $(COMMON)
	cc -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

install:
	@if test -z "$(SUDO_USER)"; then echo "\n\n!!! No sudo detected, installation will probably not work as intended !!!\n\n"; fi
	# fifo bin
	install -m0755 $(PROGRAM_FIFO) "/usr/$(PROGRAM_FIFO)"
	# http server
	install -m0755 -d /usr/local/bin/display-http-server
	install -m0755 -D http-api/*.py /usr/local/bin/display-http-server
	# systemd files
	install -m0644 systemd/raspberry-display-driver.service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/raspberry-display-driver.service
	install -m0644 systemd/raspberry-display-server.service /etc/systemd/system/
	sed -i -e s/PLACEHOLDER_USER/$(SUDO_USER)/ /etc/systemd/system/raspberry-display-server.service
	$(SYSTEMCTL) reenable raspberry-display-driver.service
	$(SYSTEMCTL) reenable raspberry-display-server.service
	@echo "\nAll files installed."
	@echo "\nYou can start the service with"
	@echo "  Driver: \"sudo systemctl restart raspberry-display-driver\""
	@echo "  Server: \"sudo systemctl restart raspberry-display-server\""

uninstall:
	$(SYSTEMCTL) stop raspberry-display-driver.service
	$(SYSTEMCTL) disable raspberry-display-driver.service
	$(SYSTEMCTL) stop raspberry-display-server.service
	$(SYSTEMCTL) disable raspberry-display-server.service
	$(RM) /etc/systemd/system/raspberry-display-driver.service
	$(RM) /etc/systemd/system/raspberry-display-server.service
	$(RM) -R "/usr/local/bin/display-http-server"
	$(RM) "/usr/$(PROGRAM_FIFO)"

-include $(DEPFILES)
