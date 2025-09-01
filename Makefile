# vim-cmd - Makefile (client-only)
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
	# Install binary
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 vim-cmd $(DESTDIR)$(BINDIR)/vim-cmd

	# macOS PATH guard (idempotent; updates the invoking user's ~/.zprofile)
	@UNAME=$$(uname -s); \
	if [ "$$UNAME" = "Darwin" ]; then \
	  USER_HOME="$$HOME"; \
	  if [ -n "$$SUDO_USER" ]; then USER_HOME=$$(eval echo ~$$SUDO_USER); fi; \
	  ZP="$$USER_HOME/.zprofile"; \
	  MARK="# Added by vim-cmd installer (PATH guard for $(BINDIR))"; \
	  if [ ! -f "$$ZP" ] || ! grep -qF "$$MARK" "$$ZP"; then \
	    printf "\n%s\n" "$$MARK" >> "$$ZP"; \
	    printf 'case ":$$PATH:" in\n  *":%s:"*) ;;\n  *) PATH="%s:$$PATH" ;;\n esac\nexport PATH\n' "$(BINDIR)" "$(BINDIR)" >> "$$ZP"; \
	    echo "Updated $$ZP (PATH guard for $(BINDIR))"; \
	  else \
	    echo "PATH guard already present in $$ZP"; \
	  fi; \
	fi

	@echo "Install complete."

uninstall:
	- rm -f $(DESTDIR)$(BINDIR)/vim-cmd
	@echo "Uninstall complete."

# Windows cross-compile helper (MinGW)
win:
	x86_64-w64-mingw32-gcc -O2 -o vim-cmd.exe src/vim-cmd.c -lws2_32
