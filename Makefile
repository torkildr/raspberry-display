HEADERS = font.h font_render.h display.h

default: mock-display

font_render.o: font_render.c $(HEADERS)
	cc -g -Wall -o font_render.o -c font_render.c

mock-display: mock-display.c font_render.o
	cc -g -Wall $< font_render.o -lncurses -o $@

clean:
	rm -f mock-display
	rm -f *.o
