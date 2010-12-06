bin_PROGRAMS = mirage2iso

mirage2iso_SOURCES = mirage2iso.c mirage-compat.h mirage-getopt.c mirage-getopt.h \
	mirage-password.c mirage-password.h mirage-wrapper.c mirage-wrapper.h
mirage2iso_LDADD = $(LIBMIRAGE_LIBS) $(LIBASSUAN_LIBS)
mirage2iso_CPPFLAGS = $(LIBMIRAGE_CFLAGS) $(LIBASSUAN_CFLAGS)
