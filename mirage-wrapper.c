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

	fprintf(stderr, ": %s\n", err ? err->message : "(err undefined?!)");
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

	if (!mirage) {
		fprintf(stderr, "miragewrap_get_version() has to be called after miragewrap_init()\n");
		return NULL;
	}

	if (!mirage_mirage_get_version(mirage, &tmp, &err)) {
		miragewrap_err("Unable to get libmirage version");
		return NULL;
	}

	if (strlen(tmp) > 9)
		fprintf(stderr, "libmirage version string too long: %s\n", tmp);

	strncpy(buf, tmp, sizeof(buf)-1);
	buf[sizeof(buf)-1] = 0;

	g_free(tmp);
	return buf;
}

const bool miragewrap_open(const char* const fn, const int session_num) {
	if (!mirage) {
		fprintf(stderr, "miragewrap_open() has to be called after miragewrap_init()\n");
		return false;
	}

	gchar *_fn = g_strdup(fn);
	gchar *filenames[] = { _fn, NULL };

	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err)) {
		disc = NULL;
		free(_fn);
		return miragewrap_err("Unable to open input '%s'", fn);
	}
	g_free(_fn);

	gint sessions;

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err))
		return miragewrap_err("Unable to get session count");
	if (sessions == 0) {
		fprintf(stderr, "Input file doesn't contain any session\n");
		return false;
	}

	if (!mirage_disc_get_session_by_index(disc, session_num, (GObject**) &session, &err)) {
		session = NULL;
		return miragewrap_err(session_num == -1 ? "Unable to get last session" : "Unable to get session %d", session_num);
	}

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err))
		return miragewrap_err("Unable to get track count");

	if (tracks == 0) {
		fprintf(stderr, "Input session doesn't contain any track\n");
		return false;
	}

	return true;
}

const int miragewrap_get_track_count(void) {
	if (!session) {
		fprintf(stderr, "miragewrap_get_track_count() has to be called after miragewrap_open()\n");
		return 0;
	}

	return tracks;
}

static MIRAGE_Track *miragewrap_get_track_common(const int track_num, gint *sstart, gint *len, int *sectsize) {
	MIRAGE_Track *track = NULL;

	if (!mirage_session_get_track_by_index(session, track_num, (GObject**) &track, &err)) {
		track = NULL;
		miragewrap_err("Unable to get track %d", track_num);;
		return NULL;
	}

	if (sstart) {
		if (!mirage_track_get_track_start(track, sstart, &err)) {
			g_object_unref(track);
			miragewrap_err("Unable to get track start for track %d", track_num);
			return NULL;
		}
	}

	if (len) {
		if (!mirage_track_layout_get_length(track, len, &err)) {
			g_object_unref(track);
			miragewrap_err("Unable to get track length for track %d", track_num);
			return NULL;
		}
	}

	if (sectsize) {
		gint mode;

		if (!mirage_track_get_mode(track, &mode, &err)) {
			g_object_unref(track);
			miragewrap_err("Unable to get track mode for track %d", track_num);
			return 0;
		}

		switch (mode) {
			case MIRAGE_MODE_MODE1:
				*sectsize = 2048;
				break;
			case MIRAGE_MODE_MODE0:
			case MIRAGE_MODE_AUDIO:
			case MIRAGE_MODE_MODE2:
			case MIRAGE_MODE_MODE2_FORM1:
			case MIRAGE_MODE_MODE2_FORM2:
			case MIRAGE_MODE_MODE2_MIXED:
				/* formats unsupported but correct */
				return NULL;
			default:
				fprintf(stderr, "Unknown track mode (%d) for track %d (newer libmirage?)\n", mode, track_num);
				return NULL;
		}
	}

	return track;
}

const size_t miragewrap_get_track_size(const int track_num) {
	if (!session) {
		fprintf(stderr, "miragewrap_get_track_size() has to be called after miragewrap_open()\n");
		return 0;
	}

	gint sstart, len;
	int expssize;

	MIRAGE_Track *track = miragewrap_get_track_common(track_num, &sstart, &len, &expssize);
	if (!track)
		return 0;

	g_object_unref(track);

	return expssize * (len-sstart);
}

const bool miragewrap_output_track(void *out, const int track_num, const bool use_mmap) {
	if (!session) {
		fprintf(stderr, "miragewrap_output_track() has to be called after miragewrap_open()\n");
		return 0;
	}

	gint sstart, len;
	int bufsize;
	MIRAGE_Track *track = miragewrap_get_track_common(track_num, &sstart, &len, &bufsize);

	if (!track)
		return false;

	gint i, olen;
	const int vlen = verbose ? snprintf(NULL, 0, "%d", len) : 0; /* printf() accepts <= 0 */

	FILE *f;
	guint8 *buf;
	if (!use_mmap) {
		/* if not using mmap, out is FILE*
		 * and we need to alloc ourselves a buffer */
		f = out;
		buf = malloc(bufsize);

		if (!buf) {
			fprintf(stderr, "malloc(%d) for buffer failed\n", bufsize);
			g_object_unref(track);
			return false;
		}
	} else
		buf = out;

	len--; /* well, now it's rather 'last' */
	for (i = sstart; i <= len; i++) {
		if (verbose && !(i % 64))
			fprintf(stderr, "\rTrack: %2d, sector: %*d of %d (%3d%%)", track_num, vlen, i, len, 100 * i / len);

		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, buf, &olen, &err)) {
			g_object_unref(track);
			if (!use_mmap)
				free(buf);
			return miragewrap_err("%sUnable to read sector %d", verbose ? "\n" : "", i);
		}

		if (olen != bufsize) {
			fprintf(stderr, "%sData read returned %d bytes while %d was expected\n",
					verbose ? "\n" : "", olen, bufsize);
			g_object_unref(track);
			if (!use_mmap)
				free(buf);
			return false;
		}

		if (!use_mmap) {
			if (fwrite(buf, olen, 1, f) != 1) {
				fprintf(stderr, "%sWrite failed on sector %d%s", verbose ? "\n" : "", i,
						ferror(f) ? ": " : " but error flag not set\n");
				if (ferror(f))
					perror(NULL);
				g_object_unref(track);
				free(buf);
				return false;
			}
		} else
			buf += olen;
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
