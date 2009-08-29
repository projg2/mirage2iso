/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <mirage.h>

static MIRAGE_Mirage *mirage = NULL;
static MIRAGE_Disc *disc = NULL;
static MIRAGE_Session *session = NULL;
static MIRAGE_Track *track = NULL;

static GError *err = NULL;

static const bool miragewrap_err(const char* const format, ...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", err->message);
	g_error_free(err);
	return false;
}

const bool miragewrap_init(void) {
	g_type_init();

	mirage = g_object_new(MIRAGE_TYPE_MIRAGE, NULL);

	return !!mirage;
}

const bool miragewrap_open(const char* const fn, const int session_num) {
	gchar *_fn = strdup(fn);
	gchar *filenames[] = { _fn, NULL };

	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err)) {
		free(_fn);
		return miragewrap_err("Unable to open input '%s'", fn);
	}
	free(_fn);

	gint sessions;

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err))
		return miragewrap_err("Unable to get session count");
	if (sessions == 0) {
		fprintf(stderr, "Input file doesn't contain any session.");
		return false;
	}

	if (!mirage_disc_get_session_by_index(disc, session_num, (GObject**) &session, &err)) {
		return miragewrap_err(session_num == -1 ? "Unable to get last session" : "Unable to get session %d", session_num);
	}

	gint tracks;

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err))
		return miragewrap_err("Unable to get track count");
	if (tracks > 1)
		fprintf(stderr, "NOTE: input session contains %d tracks; mirage2iso will read only the first one.", tracks);
	else if (tracks == 0) {
		fprintf(stderr, "Input session doesn't contain any track.");
		return false;
	}

	if (!mirage_session_get_track_by_index(session, 0, (GObject**) &track, &err))
		return miragewrap_err("Unable to get track");

	return true;
}

const bool miragewrap_output(const int fd) {
	gint sstart, len, mode;
	int expssize;
	size_t expsize;

	if (!mirage_track_get_mode(track, &mode, &err))
		return miragewrap_err("Unable to get track mode");
	switch (mode) {
		case MIRAGE_MODE_MODE1:
			expssize = 2048;
			break;
		default:
			fprintf(stderr, "mirage2iso supports only Mode1 tracks, sorry.");
			return false;
	}

	if (!mirage_track_get_track_start(track, &sstart, &err))
		return miragewrap_err("Unable to get track start");

	if (!mirage_track_layout_get_length(track, &len, &err))
		return miragewrap_err("Unable to get track length");

	expsize = expssize * (len-sstart);

	guint8 *buf = mmap(NULL, expsize, PROT_WRITE, MAP_SHARED, fd, 0);
	guint8 *bufptr = buf;
	gint i, olen;

	if (buf == MAP_FAILED) {
		perror("mmap() failed");
		return false;
	}
	ftruncate(fd, expsize);

	for (i = sstart; i < len; i++, bufptr += olen) {
		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, bufptr, &olen, &err))
			return miragewrap_err("Unable to read sector %d", i);

		if (olen != expssize) {
			fprintf(stderr, "Sector %d has incorrect size: %d (instead of %d)\n", i, olen, expssize);
			return false;
		}
	}

	if (munmap(buf, expsize)) {
		perror("munmap() failed");
		return false;
	}

	return true;
}

void miragewrap_free(void) {
	if (track) g_object_unref(track);
	if (session) g_object_unref(session);
	if (disc) g_object_unref(disc);
	if (mirage) g_object_unref(mirage);
}
