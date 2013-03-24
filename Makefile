HEADERS=font.h display.h
CFLAGS=-Wall -g

default: all

all: mock-curses-client mock-pipe-client

font_render.o: display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o display.o

mock-display.o: mock-display.c $(HEADERS)
	cc $(CFLAGS) $< -c -o mock-display.o

mock-curses-client: curses-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o -lncurses -o $@

mock-pipe-client: pipe-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o -lncurses -o $@

clean:
	rm -f mock-curses-client mock-pipe-client
	rm -f curses-client pipe-client
	rm -f *.o

