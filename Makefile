.DEFAULT_GOAL := all

## Special options
# Things that generally dont need to be overwritten but can be if needed.
ROOTDIR?=.
OBJDIR:=$(ROOTDIR)/obj
EXECUTABLE=$(ROOTDIR)/$(projectname)
COMPILER_C_FLAGS?=

## Setup variables
# These can be modified if desired at project creation, but shouldn't need to
# be updated when compiling.
projectname:=squire
srcdir:=$(ROOTDIR)/src
includedir:=$(ROOTDIR)/include/$(projectname)
override required_compiler_flags:=\
	-I$(dir $(includedir)) \
	-Wall -Wpedantic -Wextra

## Internal variables
# These don't ever need to be edited
recurisve-c-files=$(foreach path,\
	$(wildcard $(1)/*),\
	$(if $(filter %.c,$(path)),$(path),$(call recurisve-c-files,$(path))))
sources:=$(call recurisve-c-files,$(srcdir))
objects:=$(sources:$(srcdir)/%.c=$(OBJDIR)/%.o)

## Custom logic
ifdef optimized
	COMPILER_C_FLAGS+=-flto -O3 -DNDEBUG -DSQ_RELEASE_FAST -DSQ_NDEBUG
	# SQ_USE_ALLOCA
else
	COMPILER_C_FLAGS+=-g
	COMPILER_C_FLAGS+=\
		-Weverything -Wno-covered-switch-default \
		-Wno-switch-enum -Wno-comma -Wno-padded -Wno-poison-system-directories \
		-Wno-shorten-64-to-32 -Wno-cast-qual \
		-Wno-conditional-uninitialized -Wno-sign-conversion \
		-Wno-declaration-after-statement \
		-Wno-strict-prototypes
	ifdef debug
		COMPILER_C_FLAGS+=-fsanitize=address,undefined -DSQ_LOG
		njoke:=1
	endif
endif

ifdef njoke
	COMPILER_C_FLAGS+=-DSQ_NMOON_JOKE
endif

override CFLAGS+=$(ANNOYING_FLAGS)
override cflags:=$(COMPILER_C_FLAGS) $(required_compiler_flags) $(CFLAGS) 
## end custom logic

.PHONY: all
all: $(objects) $(EXECUTABLE)

.PHONY: clean
clean:
	-rm -rf $(OBJDIR) $(EXECUTABLE)

# Compiles the executable
$(EXECUTABLE): $(objects)
	@mkdir -p $(@D)
	$(CC) $(cflags) -o $@ $+

# `token.c` is wonky cause it `#include`s a c file. 
$(OBJDIR)/program/token.o: $(srcdir)/program/token.c $(srcdir)/program/macro.c
	$(CC) $(cflags) -o $@ -c $<

# Create the object files. The `include` can be uncommented if that's what youre doing
$(OBJDIR)/%.o: $(srcdir)/%.c #$(includedir)/%.h
	@mkdir -p $(@D)
	$(CC) $(cflags) -o $@ -c $<
