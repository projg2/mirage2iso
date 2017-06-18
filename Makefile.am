bin_PROGRAMS = mirage2iso

SUBDIRS = tests

mirage2iso_SOURCES = src/mirage2iso.c \
	src/mirage-password.c src/mirage-password.h \
	src/mirage-wrapper.c src/mirage-wrapper.h
mirage2iso_LDADD = $(GLIB_LIBS) $(LIBMIRAGE_LIBS) $(LIBASSUAN_LIBS)
mirage2iso_CPPFLAGS = $(GLIB_CFLAGS) $(LIBMIRAGE_CFLAGS) $(LIBASSUAN_CFLAGS)

check-recursive: mirage2iso

# force fastest compression possible, we repack it anyway
GZIP_ENV = -1

split-dist: distcheck
	gzip -dc $(distdir).tar.gz | $${TAR-tar} xf -
	rm $(distdir)/tests/Makefile* $(distdir)/tests/perform-test
	$${TAR-tar} chof - $(distdir)/tests | XZ_OPT=$${XZ_OPT--e} xz -c >$(distdir)-tests.tar.xz
	gzip -dc $(distdir).tar.gz | $${TAR-tar} xf -
	rm $(distdir)/tests/[0-9]*
	$${TAR-tar} chof - $(distdir) | XZ_OPT=$${XZ_OPT--e} xz -c >$(distdir).tar.xz
	rm -f $(distdir).tar.gz
	rm -rf $(distdir)
