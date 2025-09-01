# vim-cmd - smart macOS per-user install
CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
LDFLAGS ?=
INC     ?=

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin

UNAME_S := $(shell uname -s)

.PHONY: all clean install uninstall win

all: vim-cmd

vim-cmd: src/vim-cmd.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LDFLAGS)

clean:
	rm -f vim-cmd vim-cmd.exe
	rm -f *.o src/*.o

install: vim-cmd
	@UNAME=$$(uname -s); \
	if [ "$$UNAME" = "Darwin" ]; then \
	  # Figure out whose home to use (handles sudo)
	  USER_HOME="$$HOME"; \
	  if [ -n "$$SUDO_USER" ]; then USER_HOME=$$(eval echo ~$$SUDO_USER); fi; \
	  # Intelligent default: if user didn't override BINDIR/DESTDIR and BINDIR is /usr/local/bin,
	  # install per-user into ~/bin instead.
	  if [ -z "$(DESTDIR)" ] && [ "$(BINDIR)" = "/usr/local/bin" ]; then \
	    TARGET_BINDIR="$$USER_HOME/bin"; \
	  else \
	    TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
	  fi; \
	  install -d "$$TARGET_BINDIR"; \
	  install -m 0755 vim-cmd "$$TARGET_BINDIR/vim-cmd"; \
	  # Fix ownership for per-user installs done via sudo (no-op otherwise
