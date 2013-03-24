HEADERS=font.h display.h
CFLAGS=-Wall -g

default: all

all: mock-curses-client mock-fifo-client

font_render.o: display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o display.o

mock-display.o: mock-display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o mock-display.o

mock-curses-client: curses-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o -lncurses -o $@

mock-fifo-client: fifo-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o -lncurses -o $@

clean:
	rm -f mock-curses-client mock-fifo-client
	rm -f curses-client fifo-client
	rm -f *.o

