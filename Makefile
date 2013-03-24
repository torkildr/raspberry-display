HEADERS = font.h display.h

default: mock-display

font_render.o: display.c $(HEADERS)
	cc -g -Wall -o display.o -c display.c

mock-display: mock-display.c display.o
	cc -g -Wall $< display.o -lncurses -o $@

clean:
	rm -f mock-display
	rm -f *.o
