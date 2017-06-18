bin_PROGRAMS = mirage2iso

SUBDIRS = tests

mirage2iso_SOURCES = src/mirage2iso.c \
	src/mirage-password.c src/mirage-password.h \
	src/mirage-wrapper.c src/mirage-wrapper.h
mirage2iso_LDADD = $(GLIB_LIBS) $(LIBMIRAGE_LIBS) $(LIBASSUAN_LIBS)
mirage2iso_CPPFLAGS = $(GLIB_CFLAGS) $(LIBMIRAGE_CFLAGS) $(LIBASSUAN_CFLAGS)

check-recursive: mirage2iso
