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
allcfiles=$(wildcard $(SRCDIR)/*.c) \
		$(wildcard $(SRCDIR)/program/*.c) \
		$(wildcard $(SRCDIR)/value/*.c) \
		$(wildcard $(SRCDIR)/other/*.c) \
		$(wildcard $(SRCDIR)/other/io/*.c)

CFLAGS+=-F$(SRCDIR) -Iinclude

ifeq ($(MAKECMDGOALS),debug)
	CFLAGS+=-g -fsanitize=address,undefined -DSQ_LOG
	NJOKE=1
else ifeq ($(MAKECMDGOALS),optimized)
	CFLAGS+=-flto -DNDEBUG -O3 --DSQ_RELEASE_FAST
endif

ifdef NJOKE
CFLAGS+=-DSQ_NMOON_JOKE
endif

ifdef COMPUTED_GOTOS
CFLAGS+=-DKN_COMPUTED_GOTOS -Wno-gnu-label-as-value -Wno-gnu-designator
endif

CFLAGS+=$(CEXTRA)
CFLAGS+=$(EFLAGS)
.PHONY: all optimized clean shared

debug: $(exe)
all: $(exe)
shared: $(dyn)

clean:
	@-rm -r $(BINDIR) $(BINDIR)

optimized: $(allcfiles)
	$(CC) $(CFLAGS) -o $(exe) $+

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
