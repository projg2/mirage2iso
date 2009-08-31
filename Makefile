# mirage2iso
# (c) 2009 Michał Górny
# released under 3-clause BSD license

MIRAGE_CPPFLAGS = $$(pkg-config --cflags libmirage)
MIRAGE_LDFLAGS = $$(pkg-config --libs-only-L --libs-only-other libmirage)
MIRAGE_LIBS = $$(pkg-config --libs-only-l libmirage)

PROG = mirage2iso
OBJS = mirage-wrapper.o

DESTDIR = 
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.SUFFIXES = .o .c

all: $(PROG)

$(PROG): $(PROG).c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(MIRAGE_LDFLAGS) -o $@ $< $(OBJS) $(MIRAGE_LIBS)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MIRAGE_CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)

install: $(PROG)
	umask a+rx; mkdir -p "$(DESTDIR)$(BINDIR)"
	cp $(PROG) "$(DESTDIR)$(BINDIR)/"
	chmod a+rx "$(DESTDIR)$(BINDIR)/$(PROG)"

.PHONY: all clean install
