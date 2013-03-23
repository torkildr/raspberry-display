mock-display: mock-display.c
	cc -g -Wall $< -lncurses -o $@

clean:
	rm -f mock-display
