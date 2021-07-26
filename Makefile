CC?=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -std=gnu11
SRCDIR?=src
OBJDIR?=obj
BINDIR?=bin

exe=$(BINDIR)/squire
dyn=$(BINDIR)/libsquire.so
objects=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c))
objects+=$(patsubst $(SRCDIR)/**/%.c,$(OBJDIR)/**/%.o,$(wildcard $(SRCDIR)/**/*.c))
objects+=$(patsubst $(SRCDIR)/**/**/%.c,$(OBJDIR)/**/**/%.o,$(wildcard $(SRCDIR)/**/**/*.c))

CFLAGS+=-F$(SRCDIR) -Iinclude/squire/

CFLAGS+=-DSQ_NUMERAL_TO_ROMAN

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

$(OBJDIR)/token.o: $(SRCDIR)/token.c $(SRCDIR)/macro.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
