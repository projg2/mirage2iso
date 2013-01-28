bin_PROGRAMS = mirage2iso

SUBDIRS = tests

mirage2iso_SOURCES = mirage2iso.c \
	mirage-password.c mirage-password.h mirage-wrapper.c mirage-wrapper.h
mirage2iso_LDADD = $(GLIB_LIBS) $(LIBMIRAGE_LIBS) $(LIBASSUAN_LIBS)
mirage2iso_CPPFLAGS = $(GLIB_CFLAGS) $(LIBMIRAGE_CFLAGS) $(LIBASSUAN_CFLAGS)

check-recursive: mirage2iso
