# Makefile for clox

# tools
CC      ?= clang

# flags
CFLAGS  ?= -g -O0 -std=c99 -Wall -Wextra -Wpedantic -Wno-strict-prototypes
LDFLAGS ?=
LDLIBS  ?=

# sources/objects
SRC     := $(wildcard *.c)
OBJDIR  := build
OBJ     := $(patsubst %.c,$(OBJDIR)/%.o,$(SRC))
DEP     := $(OBJ:.o=.d)

# default
all: clox

# link
clox: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

# compile (with header deps)
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# obj dir
$(OBJDIR):
	mkdir -p $@

# housekeeping
.PHONY: clean run
clean:
	rm -rf $(OBJDIR) clox

run: clox
	./clox

# include auto-generated header deps
-include $(DEP)
