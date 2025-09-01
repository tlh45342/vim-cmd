CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
LDFLAGS ?=
INC     ?=

.PHONY: all clean install uninstall win

all: vim-cmd
vim-cmd: src/vim-cmd.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LDFLAGS)

clean:
	rm -f vim-cmd vim-cmd.exe *.o src/*.o

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
install: vim-cmd
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 vim-cmd $(DESTDIR)$(BINDIR)/vim-cmd
uninstall:
	- rm -f $(DESTDIR)$(BINDIR)/vim-cmd

# Windows cross-compile (MinGW)
win:
	x86_64-w64-mingw32-gcc -O2 -o vim-cmd.exe src/vim-cmd.c -lws2_32
