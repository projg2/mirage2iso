# mirage2iso
# (c) 2009 Michał Górny
# released under 3-clause BSD license

MIRAGE_CPPFLAGS = $$(pkg-config --cflags libmirage)
MIRAGE_LDFLAGS = $$(pkg-config --libs-only-L --libs-only-other libmirage)
MIRAGE_LIBS = $$(pkg-config --libs-only-l libmirage)
CPPFLAGS = $$([ -f mirage-config.h ] && echo -DUSE_CONFIG)

PROG = mirage2iso
OBJS = mirage-getopt.o mirage-wrapper.o

CONFIGOUT = mirage-config.h
CONFIGTESTS = check-getopt.o check-sysexits.o check-mmapio.o
CONFIGIN = check-getopt.c check-sysexits.c check-mmapio.c

DESTDIR = 
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.SUFFIXES: .o .c

all: $(PROG)
configure: $(CONFIGOUT)

$(PROG): $(PROG).c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(MIRAGE_LDFLAGS) -o $@ $< $(OBJS) $(MIRAGE_LIBS)

mirage-wrapper.o: mirage-wrapper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MIRAGE_CPPFLAGS) -c -o $@ $<

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(CONFIGOUT):
	make $(MAKEOPTS) -k $(CONFIGTESTS) || true
	[ -f check-getopt.o ] || echo '#define NO_GETOPT_LONG 1' >> $@
	[ -f check-sysexits.o ] || echo '#define NO_SYSEXITS 1' >> $@
	[ -f check-mmapio.o ] || echo '#define NO_MMAPIO 1' >> $@
	touch $@
	rm -f $(CONFIGTESTS) $(CONFIGIN)

check-getopt.c:
	printf '#define _GNU_SOURCE 1\n#include <getopt.h>\nint main(int argc, char *argv[]) { getopt_long(argc, argv, "", 0, 0); return 0; }\n' > $@

check-sysexits.c:
	printf '#define _BSD_SOURCE 1\n#include <sysexits.h>\nint main(int argc, char *argv[]) { int test = EX_IOERR; return EX_OK; }\n' > $@

check-mmapio.c:
	printf '#define _POSIX_C_SOURCE 200112L\n#include <unistd.h>\n#include <sys/types.h>\n#include <sys/mman.h>\nint main(void) { ftruncate(0, 0); mmap(0, 0, 0, 0, 0, 0); }\n' > $@

clean:
	rm -f $(PROG) $(OBJS) $(CONFIGOUT)

install: $(PROG)
	umask a+rx; mkdir -p "$(DESTDIR)$(BINDIR)"
	cp $(PROG) "$(DESTDIR)$(BINDIR)/"
	chmod a+rx "$(DESTDIR)$(BINDIR)/$(PROG)"

.PHONY: all clean configure install
