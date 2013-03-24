HEADERS=font.h display.h
CFLAGS=-Wall

default: mock-display

font_render.o: display.c $(HEADERS)
	cc -g $(CFLAGS) -o display.o -c display.c

mock-display: mock-display.c display.o $(HEADERS)
	cc -g $(CFLAGS) $< display.o -lncurses -o $@

clean:
	rm -f mock-display
	rm -f *.o
