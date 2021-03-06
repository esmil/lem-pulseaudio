CC         = gcc -std=gnu99
CFLAGS    ?= -O2 -pipe -Wall -Wextra
PKG_CONFIG = pkg-config
STRIP      = strip
INSTALL    = install
UNAME      = uname

OS         = $(shell $(UNAME))
CFLAGS    += $(shell $(PKG_CONFIG) --cflags lem)
lmoddir    = $(shell $(PKG_CONFIG) --variable=INSTALL_LMOD lem)
cmoddir    = $(shell $(PKG_CONFIG) --variable=INSTALL_CMOD lem)

ifeq ($(OS),Darwin)
SHARED     = -dynamiclib -Wl,-undefined,dynamic_lookup
STRIP     += -x
else
SHARED     = -shared
endif

llibs = lem/pulseaudio.lua
clibs = lem/pulseaudio/core.so

ifdef V
E=@\#
Q=
else
E=@echo
Q=@
endif

.PHONY: all debug strip install clean

all: CFLAGS += -DNDEBUG
all: $(clibs)

debug: $(clibs)

lem/pulseaudio/core.so: LIBS += -lpulse
lem/pulseaudio/core.so: lem/pulseaudio/core.c \
		lem/pulseaudio/mainloop.c \
		lem/pulseaudio/query.c \
		lem/pulseaudio/set.c
	$E '  CCLD  $@'
	$Q$(CC) $(CFLAGS) -fPIC -nostartfiles $(SHARED) $< -o $@ $(LDFLAGS) $(LIBS)

%-strip: %
	$E '  STRIP $<'
	$Q$(STRIP) $<

strip: $(clibs:%=%-strip)

$(DESTDIR)$(lmoddir)/% $(DESTDIR)$(cmoddir)/%: %
	$E '  INSTALL $@'
	$Q$(INSTALL) -d $(dir $@)
	$Q$(INSTALL) -m 644 $< $@

install: \
	$(llibs:%=$(DESTDIR)$(lmoddir)/%) \
	$(clibs:%=$(DESTDIR)$(cmoddir)/%)

clean:
	rm -f $(clibs)
