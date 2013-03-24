HEADERS=font.h display.h
CFLAGS=-Wall -g

default: mock-curses-client

font_render.o: display.c $(HEADERS)
	cc $(CFLAGS) -o display.o -c display.c

mock-display.o: mock-display.c $(HEADERS)
	cc $(CFLAGS) -o mock-display.o -c mock-display.c

mock-curses-client: curses-client.c display.o mock-display.o $(HEADERS)
	cc $(CFLAGS) $< display.o mock-display.o -lncurses -o $@

clean:
	rm -f mock-curses-client
	rm -f curses-client
	rm -f *.o

