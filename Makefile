CC?=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -std=gnu11
SRCDIR?=src
OBJDIR?=obj
BINDIR?=bin

exe=$(BINDIR)/squire
dyn=$(BINDIR)/libsquire.so
objects=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c))
objects+=$(patsubst $(SRCDIR)/value/%.c,$(OBJDIR)/value/%.o,$(wildcard $(SRCDIR)/value/*.c))
objects+=$(patsubst $(SRCDIR)/program/%.c,$(OBJDIR)/program/%.o,$(wildcard $(SRCDIR)/program/*.c))
objects+=$(patsubst $(SRCDIR)/other/%.c,$(OBJDIR)/other/%.o,$(wildcard $(SRCDIR)/other/*.c))
objects+=$(patsubst $(SRCDIR)/other/io/%.c,$(OBJDIR)/other/io/%.o,$(wildcard $(SRCDIR)/other/io/*.c))

CFLAGS+=-F$(SRCDIR) -Iinclude
CFLAGS+=-DSQ_NUMERAL_TO_ROMAN

ifndef JOKE
CFLAGS+=-DSQ_NMOON_JOKE
endif

ifdef DEBUG
CFLAGS+=-g -fsanitize=address,undefined -DSQ_LOG
else
	CFLAGS+=-O2
	ifdef OPTIMIZED
		CFLAGS+= -flto -march=native -DNDEBUG
	endif
endif

ifdef COMPUTED_GOTOS
CFLAGS+=-DKN_COMPUTED_GOTOS -Wno-gnu-label-as-value -Wno-gnu-designator
endif

CFLAGS+=$(CEXTRA) $(EFLAGS)
.PHONY: all optimized clean shared

all: $(exe)
shared: $(dyn)

clean:
	@-rm -r $(BINDIR) $(BINDIR)

optimized:
	$(CC) $(CFLAGS) -o $(exe) $(wildcard $(SRCDIR)/*.c)

$(exe): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $+

$(dyn): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -shared -o $@ $+

$(BINDIR):
	@mkdir -p $(BINDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)/value $(OBJDIR)/program $(OBJDIR)/other/io

$(objects): | $(OBJDIR)

$(OBJDIR)/program/token.o: $(SRCDIR)/program/token.c $(SRCDIR)/program/macro.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
