# vim-cmd - Makefile (cross-platform install logic)
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin

SRC     := src/vim-cmd.c
TARGET  := vim-cmd

.PHONY: all clean install uninstall win

all: $(TARGET)

$(TARGET): $(SRC)
    $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
    rm -f $(TARGET) $(TARGET).exe
    rm -f *.o src/*.o

install: $(TARGET)
    @UNAME=$$(uname -s); \
    if [ "$$UNAME" = "Darwin" ]; then \
        TARGET_BINDIR="$${HOME}/bin"; \
    elif [ "$$UNAME" = "Linux" ]; then \
        TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
    else \
        echo "Unsupported OS for install"; exit 1; \
    fi; \
    mkdir -p "$$TARGET_BINDIR"; \
    install -m 0755 $(TARGET) "$$TARGET_BINDIR/$(TARGET)"; \
    echo "Installed to: $$TARGET_BINDIR/$(TARGET)"

uninstall:
    @UNAME=$$(uname -s); \
    if [ "$$UNAME" = "Darwin" ]; then \
        TARGET_BINDIR="$${HOME}/bin"; \
    elif [ "$$UNAME" = "Linux" ]; then \
        TARGET_BINDIR="$(DESTDIR)$(BINDIR)"; \
    else \
        echo "Unsupported OS for uninstall"; exit 1; \
    fi; \
    - rm -f "$$TARGET_BINDIR/$(TARGET)"; \
    echo "Uninstall complete (checked $$TARGET_BINDIR)"

# Windows cross-compile helper (MinGW)
win:
    x86_64-w64-mingw32-gcc -O2 -o $(TARGET).exe $(SRC) -lws2_32
