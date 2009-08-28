CPPFLAGS += $(shell pkg-config --cflags libmirage)
LDFLAGS += $(shell pkg-config --libs libmirage)

PROG = mirage2iso
OBJS = mirage-wrapper.o

DESTDIR ?= 
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

all: $(PROG)

$(PROG): $(OBJS)

clean:
	rm -f $(PROG) $(OBJS)

install: $(PROG)
	install -m755 $(PROG) $(DESTDIR)$(BINDIR)/

.PHONY: clean install
