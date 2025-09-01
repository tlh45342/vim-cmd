# vim-cmd - Makefile (client-only; mac-smart per-user install, single file)
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=
INC     ?=

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin

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
	  USER_HOME="$$HOME"; \
	  if [ -n "$$SUDO_USER" ]; then USER_HOME=$$(eval echo ~"$$SUDO_USER"); fi; \
	  if [ -z "$(DESTDIR)" ] && [ "$(BINDIR)" = "/usr/local/bin" ]; then \
	    TARGET_BINDIR="$$USER_HOME/bin"; \
	  else \
	    TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
	  fi; \
	  mkdir -p "$$TARGET_BINDIR"; \
	  install -m 0755 vim-cmd "$$TARGET_BINDIR/vim-cmd"; \
	  if [ -n "$$SUDO_USER" ] && [ -z "$(DESTDIR)" ] && [ "$${TARGET_BINDIR#$$USER_HOME/}" != "$$TARGET_BINDIR" ]; then \
	    CHGRP=$$(id -gn "$$SUDO_USER" 2>/dev/null || echo staff); \
	    chown "$$SUDO_USER:$$CHGRP" "$$TARGET_BINDIR/vim-cmd" 2>/dev/null || true; \
	  fi; \
	  if [ -z "$(DESTDIR)" ] && [ "$${TARGET_BINDIR#$$USER_HOME/}" != "$$TARGET_BINDIR" ]; then \
	    ZP="$$USER_HOME/.zprofile"; \
	    MARK="# Added by vim-cmd installer (PATH guard for $$TARGET_BINDIR)"; \
	    [ -f "$$ZP" ] || : > "$$ZP"; \
	    if ! grep -qF "$$MARK" "$$ZP"; then \
	      printf '\n%s\n' "$$MARK" >> "$$ZP"; \
	      printf 'case ":$$PATH:" in\n' >> "$$ZP"; \
	      printf '  *":%s:"*) ;;\n' "$$TARGET_BINDIR" >> "$$ZP"; \
	      printf '  *) PATH="%s:$$PATH" ;;\n' "$$TARGET_BINDIR" >> "$$ZP"; \
	      printf 'esac\nexport PATH\n' >> "$$ZP"; \
	      echo "Updated $$ZP (PATH guard for $$TARGET_BINDIR)"; \
	    else \
	      echo "PATH guard already present in $$ZP"; \
	    fi; \
	  fi; \
	  echo "Installed to: $$TARGET_BINDIR/vim-cmd"; \
	else \
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
	  if [ -n "$$SUDO_USER" ]; then USER_HOME=$$(eval echo ~"$$SUDO_USER"); fi; \
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
