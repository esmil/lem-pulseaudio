CC         = gcc
CFLAGS    ?= -O2 -pipe -Wall -Wextra -Wno-variadic-macros
CFLAGS    += -std=c99 -D_GNU_SOURCE
PKGCONFIG  = pkg-config
STRIP      = strip
INSTALL    = install
UNAME      = uname

OS         = $(shell $(UNAME))
CFLAGS    += $(shell $(PKGCONFIG) --cflags lem)
LUA_PATH   = $(shell $(PKGCONFIG) --variable=path lem)
LUA_CPATH  = $(shell $(PKGCONFIG) --variable=cpath lem)

ifeq ($(OS),Darwin)
SHARED     = -dynamiclib -Wl,-undefined,dynamic_lookup
STRIP_ARGS = -x
else
SHARED     = -shared
endif

clibs = core.so
libs  = pulseaudio.lua

ifdef NDEBUG
CFLAGS += -DNDEBUG
endif

ifdef V
E=@\#
Q=
else
E=@echo
Q=@
endif

.PHONY: all strip install clean
.PRECIOUS: %.o

all: $(clibs)

core.so: pulseaudio.c mainloop.c query.c set.c stream.c
	$E '  CCLD $@'
	$Q$(CC) $(CFLAGS) -fPIC -nostartfiles $(SHARED) $< -o $@ $(LDFLAGS) -lpulse

%-strip: %
	$E '  STRIP $<'
	$Q$(STRIP) $(STRIP_ARGS) $<

strip: $(clibs:%=%-strip)

path-install:
	$E '  INSTALL -d $(LUA_PATH)/lem'
	$Q$(INSTALL) -d $(DESTDIR)$(LUA_PATH)/lem

%.lua-install: %.lua path-install
	$E '  INSTALL $<'
	$Q$(INSTALL) -m644 $< $(DESTDIR)$(LUA_PATH)/lem/$<

cpath-install:
	$E '  INSTALL -d $(LUA_CPATH)/lem/pulseaudio'
	$Q$(INSTALL) -d $(DESTDIR)$(LUA_CPATH)/lem/pulseaudio

%.so-install: %.so cpath-install
	$E '  INSTALL $<'
	$Q$(INSTALL) $< $(DESTDIR)$(LUA_CPATH)/lem/pulseaudio/$<

install: $(clibs:%=%-install) $(libs:%=%-install)

clean:
	rm -f $(clibs) *.o *.c~ *.h~
