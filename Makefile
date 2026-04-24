# ============================================================
# BZip2 — Phase 1 Makefile  (Linux / macOS / Windows MinGW)
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -Iinclude
LDFLAGS = -lm

TARGET  = bzip2_impl
SRCDIR  = src
OBJDIR  = obj

SOURCES = $(SRCDIR)/main.c   \
          $(SRCDIR)/config.c \
          $(SRCDIR)/block.c  \
          $(SRCDIR)/rle.c    \
          $(SRCDIR)/bwt.c    \
          $(SRCDIR)/utils.c

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# ---- Platform detection ----
UNAME := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME), Windows)
    EXE = .exe
else
    EXE =
endif

# ---- Targets ----
all: dirs $(TARGET)$(EXE)
	@echo ""
	@echo "  Build complete -> ./$(TARGET)$(EXE)"
	@echo "  Quick test     -> make test"
	@echo "  Demo           -> make demo"

$(TARGET)$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

dirs:
	@mkdir -p $(OBJDIR) benchmarks results

# ---- Tests ----
test: all
	./$(TARGET)$(EXE) test

# ---- Demo: compress and verify a sample file ----
demo: all
	@echo "ABCDDDDDDDDDDEEEEEEEEEEEEEE" > benchmarks/demo.txt
	@echo "banana"                      >> benchmarks/demo.txt
	@echo "abracadabra"                 >> benchmarks/demo.txt
	@echo "the quick brown fox jumps over the lazy dog" >> benchmarks/demo.txt
	@echo ""
	./$(TARGET)$(EXE) compress   benchmarks/demo.txt   results/demo.bz2p1
	./$(TARGET)$(EXE) decompress results/demo.bz2p1    results/demo_recovered.txt
	@diff benchmarks/demo.txt results/demo_recovered.txt \
	    && echo "SUCCESS: Recovered file matches original!" \
	    || echo "FAIL: Files differ!"

# ---- Windows cross-compile (requires mingw-w64) ----
windows:
	$(MAKE) CC=x86_64-w64-mingw32-gcc EXE=.exe

# ---- Clean ----
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET).exe
	@echo "Cleaned."

.PHONY: all dirs test demo windows clean
