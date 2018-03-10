HEADERS=font.h display.h ht1632.h
CFLAGS=-Wall -g
LIBS=-lrt -lncurses

default: all
all: mock real

real: curses-client fifo-client
mock: mock-curses-client mock-fifo-client

display.o: display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o $@

ht1632.o: ht1632.c $(HEADERS)
	cc $(CFLAGS) $< -c -o $@

mock-display.o: mock-display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o $@

curses-client: curses-client.c display.o ht1632.o $(HEADERS)
	cc $(CFLAGS) $< display.o ht1632.o $(LIBS) -o $@

fifo-client: fifo-client.c display.o ht1632.o $(HEADERS)
	cc $(CFLAGS) $< display.o ht1632.o $(LIBS) -o $@

mock-curses-client: curses-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o $(LIBS) -o $@

mock-fifo-client: fifo-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o $(LIBS) -o $@

clean:
	rm -f mock-curses-client mock-fifo-client
	rm -f curses-client fifo-client
	rm -f *.o

