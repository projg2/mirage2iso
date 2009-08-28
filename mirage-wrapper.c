/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <mirage.h>

static MIRAGE_Mirage *mirage = NULL;
static MIRAGE_Disc *disc = NULL;
static MIRAGE_Session *session = NULL;
static MIRAGE_Track *track = NULL;

const bool miragewrap_init(void) {
	g_type_init();

	mirage = g_object_new(MIRAGE_TYPE_MIRAGE, NULL);

	return !!mirage;
}

const bool miragewrap_setinput(const char* const fn) {
	gchar *_fn = strdup(fn);
	gchar *filenames[] = { _fn, NULL };
	GError *err = NULL;

	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err) || err) {
		fprintf(stderr, "Unable to open input '%s': %s\n", fn, err->message);
		g_error_free(err);
		free(_fn);
		return FALSE;
	}
	free(_fn);

	gint sessions;

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err) || err) {
		fprintf(stderr, "Unable to get session count: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	if (sessions > 1)
		fprintf(stderr, "NOTE: input file contains %d sessions; mirage2iso will read only the last one.", sessions);
	else if (sessions == 0) {
		fprintf(stderr, "Input file doesn't contain any session.");
		return FALSE;
	}

	if (!mirage_disc_get_session_by_index(disc, -1, (GObject**) &session, &err) || err) {
		fprintf(stderr, "Unable to get last session: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	gint tracks;

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err) || err) {
		fprintf(stderr, "Unable to get track count: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	if (tracks > 1)
		fprintf(stderr, "NOTE: input session contains %d tracks; mirage2iso will read only the first one.", tracks);
	else if (tracks == 0) {
		fprintf(stderr, "Input session doesn't contain any track.");
		return FALSE;
	}

	if (!mirage_session_get_track_by_index(session, 0, (GObject**) &track, &err) || err) {
		fprintf(stderr, "Unable to get track: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	return TRUE;
}

const bool miragewrap_output(FILE* const f) {
	GError *err = NULL;
	gint sstart, len, mode, expsize;

	if (!mirage_track_get_mode(track, &mode, &err) || err) {
		fprintf(stderr, "Unable to get track mode: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	switch (mode) {
		case MIRAGE_MODE_MODE1:
			expsize = 2048;
			break;
		default:
			fprintf(stderr, "mirage2iso supports only Mode1 tracks, sorry.");
			return FALSE;
	}

	if (!mirage_track_get_track_start(track, &sstart, &err) || err) {
		fprintf(stderr, "Unable to get track start: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	if (!mirage_track_layout_get_length(track, &len, &err) || err) {
		fprintf(stderr, "Unable to get track length: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	guint8 buf[4096];
	gint i, olen;

	for (i = sstart; i < len; i++) {
		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, buf, &olen, &err) || err) {
			fprintf(stderr, "Unable to read sector %d: %s\n", i, err->message);
			g_error_free(err);
			return FALSE;
		}

		if (olen != expsize) { /* Mode1 */
			fprintf(stderr, "Sector %d has incorrect size: %d (instead of %d)\n", i, olen, expsize);
			return FALSE;
		}

		if (fwrite(buf, olen, 1, f) != 1) {
			perror("Unable to write output");
			return FALSE;
		}
	}

	return TRUE;
}

void miragewrap_free(void) {
	if (track) g_object_unref(track);
	if (session) g_object_unref(session);
	if (disc) g_object_unref(disc);
	if (mirage) g_object_unref(mirage);
}
