CFLAGS=-Wall -g

LDFLAGS=-lrt -lwiringPi -lncurses

default: all
all: mock real

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
INCLUDES = $(wildcard $(SRCDIR)/*.h)

DRIVER      = $(OBJDIR)/display.o $(OBJDIR)/ht1632.o
MOCK_DRIVER = $(OBJDIR)/display.o $(OBJDIR)/mock-display.o

real: $(BINDIR)/curses-client $(BINDIR)/fifo-client
mock: $(BINDIR)/mock-curses-client $(BINDIR)/mock-fifo-client

$(OBJDIR)/:
	mkdir -p $@

$(BINDIR)/:
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDES) $(OBJDIR)/
	cc $(CFLAGS) $< $(LDFLAGS) -c -o $@

$(BINDIR)/curses-client: $(SRCDIR)/curses-client.c $(DRIVER) $(INCLUDES) $(BINDIR)/
	cc $(CFLAGS) $< $(DRIVER) $(LDFLAGS) -o $@

$(BINDIR)/fifo-client: $(SRCDIR)/fifo-client.c $(DRIVER) $(INCLUDES) $(BINDIR)/
	cc $(CFLAGS) $< $(DRIVER) $(LDFLAGS) -o $@

$(BINDIR)/mock-curses-client: $(SRCDIR)/curses-client.c $(MOCK_DRIVER) $(INCLUDES) $(BINDIR)/
	cc $(CFLAGS) $< $(MOCK_DRIVER) $(LDFLAGS) -o $@

$(BINDIR)/mock-fifo-client: $(SRCDIR)/fifo-client.c $(MOCK_DRIVER) $(INCLUDES) $(BINDIR)/
	cc $(CFLAGS) $< $(MOCK_DRIVER) $(LDFLAGS) -o $@

clean:
	rm -Rf $(OBJDIR)
	rm -Rf $(BINDIR)

