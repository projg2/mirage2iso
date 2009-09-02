# mirage2iso
# (c) 2009 Michał Górny
# released under 3-clause BSD license

MIRAGE_CPPFLAGS = $$(pkg-config --cflags libmirage)
MIRAGE_LDFLAGS = $$(pkg-config --libs-only-L --libs-only-other libmirage)
MIRAGE_LIBS = $$(pkg-config --libs-only-l libmirage)

PROG = mirage2iso
OBJS = mirage-getopt.o mirage-wrapper.o

CONFIGOUT = mirage-config.h
CONFIGTESTS = check-getopt.o check-sysexits.o check-mmapio.o
CONFIGIN = check-getopt.c check-sysexits.c check-mmapio.c

CPPFLAGS = $$([ -f $(CONFIGOUT) ] && echo -DUSE_CONFIG=1)

DESTDIR = 
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.SILENT: configure $(CONFIGIN)
.SUFFIXES: .o .c

all: $(PROG)

$(PROG): $(PROG).c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(MIRAGE_LDFLAGS) -o $@ $< $(OBJS) $(MIRAGE_LIBS)

mirage-wrapper.o: mirage-wrapper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MIRAGE_CPPFLAGS) -c -o $@ $<

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

configure:
	make $(MAKEOPTS) -k CPPFLAGS='' $(CONFIGTESTS) || true
	if [ -f check-getopt.o ]; then echo 'getopt_long() found.'; \
		else echo 'getopt_long() unavailable.'; echo '#define NO_GETOPT_LONG 1' >> $(CONFIGOUT); fi
	if [ -f check-sysexits.o ]; then echo '<sysexits.h> found.'; \
		else echo '<sysexits.h> unavailable.'; echo '#define NO_SYSEXITS 1' >> $(CONFIGOUT); fi
	if [ -f check-mmapio.o ]; then echo 'mmap() & ftruncate() found.'; \
		else echo 'mmap() & ftruncate() unavailable.'; echo '#define NO_MMAPIO 1' >> $(CONFIGOUT); fi
	touch $(CONFIGOUT)
	rm -f $(CONFIGTESTS) $(CONFIGIN)

check-getopt.c:
	printf '#define _GNU_SOURCE 1\n#include <getopt.h>\nint main(int argc, char *argv[]) { getopt_long(argc, argv, "", 0, 0); return 0; }\n' > $@

check-sysexits.c:
	printf '#define _BSD_SOURCE 1\n#include <sysexits.h>\nint main(int argc, char *argv[]) { return EX_OK + EX_IOERR; }\n' > $@

check-mmapio.c:
	printf '#define _POSIX_C_SOURCE 200112L\n#include <unistd.h>\n#include <sys/types.h>\n#include <sys/mman.h>\nint main(void) { return (ftruncate(0, 0) || mmap(0, 0, 0, 0, 0, 0) == MAP_FAILED); }\n' > $@

clean:
	rm -f $(PROG) $(OBJS)

distclean: clean
	rm -f $(CONFIGOUT)

install: $(PROG)
	umask a+rx; mkdir -p "$(DESTDIR)$(BINDIR)"
	cp $(PROG) "$(DESTDIR)$(BINDIR)/"
	chmod a+rx "$(DESTDIR)$(BINDIR)/$(PROG)"

.PHONY: all clean configure distclean install
