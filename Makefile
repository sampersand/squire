CC?=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -std=c11
SRCDIR?=src
OBJDIR?=obj
BINDIR?=bin

exe=$(BINDIR)/squire
dyn=$(BINDIR)/libsquire.so
objects=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c))

CFLAGS+=-F$(SRCDIR)

ifdef DEBUG
CFLAGS+=-g -fsanitize=address,undefined -DSQ_LOG
else
CFLAGS+=-O2 -flto -march=native -DNDEBUG
endif

ifdef COMPUTED_GOTOS
CFLAGS+=-DKN_COMPUTED_GOTOS -Wno-gnu-label-as-value -Wno-gnu-designator
endif

CFLAGS+=$(CEXTRA)
.PHONY: all optimized clean shared

CFLAGS+= $(EFLAGS)
all: $(exe)
shared: $(dyn)

clean:
	@-rm -r bin obj squire.dSYM

optimized:
	$(CC) $(CFLAGS) -o $(exe) $(wildcard $(SRCDIR)/*.c)

$(exe): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $+

$(dyn): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -shared -o $@ $+

$(BINDIR):
	@mkdir -p $(BINDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(objects): | $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
