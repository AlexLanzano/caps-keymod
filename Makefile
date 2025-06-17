CFLAGS = -Werror -Wall -Wextra
all: caps-keymod caps-keymod-debug

caps-keymod: caps-keymod.c Makefile
	gcc $(CFLAGS) -o $@ $<

caps-keymod-debug: caps-keymod.c Makefile
	gcc $(CFLAGS) -g3 -DDEBUG -o $@ $<

.PHONY: clean
clean:
	rm -rf caps-keymod caps-keymod-debug
