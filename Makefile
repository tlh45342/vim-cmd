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
	  # Fix ownership for per-user installs done via sudo (no-op otherwise)
	  if [ -z "$(DESTDIR)" ] && [ -n "$$SUDO_USER" ] && case "$$TARGET_BINDIR" in "$$USER_HOME"/*) true;; *) false;; esac; then \
	    CHGRP=$$(id -gn "$$SUDO_USER" 2>/dev/null || echo staff); \
	    chown "$$SUDO_USER":"$$CHGRP" "$$TARGET_BINDIR/vim-cmd" 2>/dev/null || true; \
	  fi; \
	  # PATH guard (idempotent) only when installing into the real user's home
	  if [ -z "$(DESTDIR)" ] && case "$$TARGET_BINDIR" in "$$USER_HOME"/*) true;; *) false;; esac; then \
	    ZP="$$USER_HOME/.zprofile"; \
	    MARK="# Added by vim-cmd installer (PATH guard for $$TARGET_BINDIR)"; \
	    if [ ! -f "$$ZP" ] || ! grep -qF "$$MARK" "$$ZP"; then \
	      printf "\n%s\n" "$$MARK" >> "$$ZP"; \
	      printf 'case ":$$PATH:" in\n  *":%s:"*) ;;\n  *) PATH="%s:$$PATH" ;;\n esac\nexport PATH\n' "$$TARGET_BINDIR" "$$TARGET_BINDIR" >> "$$ZP"; \
	      echo "Updated $$ZP (PATH guard for $$TARGET_BINDIR)"; \
	    else \
	      echo "PATH guard already present in $$ZP"; \
	    fi; \
	  fi; \
	  echo "Installed to: $$TARGET_BINDIR/vim-cmd"; \
	else \
	  # Non-macOS: standard install to $(DESTDIR)$(BINDIR)
	  TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
	  install -d "$$TARGET_BINDIR"; \
	  install -m 0755 vim-cmd "$$TARGET_BINDIR/vim-cmd"; \
	  echo "Installed to: $$TARGET_BINDIR/vim-cmd"; \
	fi
	@echo "Install complete."

uninstall:
	@UNAME=$$(uname -s); \
	if [ "$$UNAME" = "Darwin" ]; then \
	  USER_HOME="$$HOME"; \
	  if [ -n "$$SUDO_USER" ]; then USER_HOME=$$(eval echo ~$$SUDO_USER); fi; \
	  if [ -z "$(DESTDIR)" ] && [ "$(BINDIR)" = "/usr/local/bin" ]; then \
	    TARGET_BINDIR="$$USER_HOME/bin"; \
	  else \
	    TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
	  fi; \
	else \
	  TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
	fi; \
	- rm -f "$$TARGET_BINDIR/vim-cmd"; \
	echo "Uninstall complete (checked $$TARGET_BINDIR)."

# Windows cross-compile helper (MinGW)
win:
	x86_64-w64-mingw32-gcc -O2 -o vim-cmd.exe src/vim-cmd.c -lws2_32
