/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <mirage.h>

extern bool verbose;

static MIRAGE_Mirage *mirage = NULL;
static MIRAGE_Disc *disc = NULL;
static MIRAGE_Session *session = NULL;
static gint tracks;

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

const char* const miragewrap_get_version(void) {
	static char buf[10];
	gchar *tmp;

	if (!mirage_mirage_get_version(mirage, &tmp, &err)) {
		miragewrap_err("Unable to get libmirage version");
		return NULL;
	}

	if (strlen(tmp) > 9)
		fprintf(stderr, "libmirage version string too long: %s", tmp);

	strncpy(buf, tmp, sizeof(buf)-1);
	buf[sizeof(buf)-1] = 0;

	g_free(tmp);
	return buf;
}

const bool miragewrap_open(const char* const fn, const int session_num) {
	gchar *_fn = g_strdup(fn);
	gchar *filenames[] = { _fn, NULL };

	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err)) {
		free(_fn);
		return miragewrap_err("Unable to open input '%s'", fn);
	}
	g_free(_fn);

	gint sessions;

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err))
		return miragewrap_err("Unable to get session count");
	if (sessions == 0) {
		fprintf(stderr, "Input file doesn't contain any session.");
		return false;
	}

	if (!mirage_disc_get_session_by_index(disc, session_num, (GObject**) &session, &err))
		return miragewrap_err(session_num == -1 ? "Unable to get last session" : "Unable to get session %d", session_num);

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err))
		return miragewrap_err("Unable to get track count");

	if (tracks == 0) {
		fprintf(stderr, "Input session doesn't contain any track.");
		return false;
	}

	return true;
}

const int miragewrap_get_track_count(void) {
	return tracks;
}

const size_t miragewrap_get_track_size(const int track_num) {
	MIRAGE_Track *track = NULL;
	gint sstart, len, mode;
	int expssize;

	if (!mirage_session_get_track_by_index(session, track_num, (GObject**) &track, &err)) {
		miragewrap_err("Unable to get track %d", track_num);;
		return 0;
	}

	if (!mirage_track_get_mode(track, &mode, &err)) {
		g_object_unref(track);
		miragewrap_err("Unable to get track mode");
		return 0;
	}

	switch (mode) {
		case MIRAGE_MODE_MODE1:
			expssize = 2048;
			break;
		default:
			g_object_unref(track);
			fprintf(stderr, "mirage2iso supports only Mode1 tracks, sorry.");
			return 0;
	}

	if (!mirage_track_get_track_start(track, &sstart, &err)) {
		g_object_unref(track);
		miragewrap_err("Unable to get track start");
		return 0;
	}

	if (!mirage_track_layout_get_length(track, &len, &err)) {
		g_object_unref(track);
		miragewrap_err("Unable to get track length");
		return 0;
	}

	g_object_unref(track);
	return expssize * (len-sstart);
}

const bool miragewrap_output_track(void *out, const int track_num) {
	MIRAGE_Track *track = NULL;
	gint sstart, len;

	if (!mirage_session_get_track_by_index(session, track_num, (GObject**) &track, &err))
		return miragewrap_err("Unable to get track %d", track_num);

	if (!mirage_track_get_track_start(track, &sstart, &err)) {
		g_object_unref(track);
		return miragewrap_err("Unable to get track start");
	}

	if (!mirage_track_layout_get_length(track, &len, &err)) {
		g_object_unref(track);
		return miragewrap_err("Unable to get track length");
	}

	gint i, olen;
	const int vlen = verbose ? snprintf(NULL, 0, "%d", len) : 0; /* printf() accepts <= 0 */

	len--; /* well, now it's rather 'last' */
	for (i = sstart; i <= len; i++, out += olen) {
		if (verbose && !(i % 64))
			fprintf(stderr, "\rTrack: %2d, sector: %*d of %d (%3d%%)", track_num, vlen, i, len, 100 * i / len);
		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, out, &olen, &err)) {
			g_object_unref(track);
			return miragewrap_err("%sUnable to read sector %d", verbose ? "\n" : "", i);
		}
	}
	if (verbose)
		fprintf(stderr, "\rTrack: %2d, sector: %d of %d (100%%)\n", track_num, len, len);

	g_object_unref(track);
	return true;
}

void miragewrap_free(void) {
	if (session) g_object_unref(session);
	if (disc) g_object_unref(disc);
	if (mirage) g_object_unref(mirage);
}
