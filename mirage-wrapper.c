/* mirage2iso; libmirage interface
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <mirage.h>
#include "mirage-compat.h"
#include "mirage-password.h"

extern bool quiet;
extern bool verbose;

#ifdef MIRAGE_HAS_MIRAGE_OBJ
static MIRAGE_Mirage *mirage = NULL;
#endif
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

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT

gchar* miragewrap_password_callback(gpointer user_data) {
	const char* const pass = mirage_input_password();

	if (!pass)
		return NULL;

	return g_strdup(pass);
}

#endif

const bool miragewrap_init(void) {
	g_type_init();

#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!((mirage = g_object_new(MIRAGE_TYPE_MIRAGE, NULL))))
		return false;
#else
	if (!libmirage_init(&err))
		return miragewrap_err("Unable to init libmirage");
#endif

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT
	if (!libmirage_set_password_function(miragewrap_password_callback, NULL, &err))
		miragewrap_err("Unable to set password callback");
#endif

	return true;
}

#ifdef MIRAGE_HAS_MIRAGE_OBJ

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

#else

const char* const miragewrap_get_version(void) {
	return mirage_version_long;
}

#endif

const bool miragewrap_open(const char* const fn, const int session_num) {
#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!mirage) {
		fprintf(stderr, "miragewrap_open() has to be called after miragewrap_init()\n");
		return false;
	}
#endif /* XXX: add some check for new API */

	gchar *_fn = g_strdup(fn);
	gchar *filenames[] = { _fn, NULL };

#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err)) {
#else
	if (!((disc = (MIRAGE_Disc*) libmirage_create_disc(filenames, NULL, NULL, &err)))) {
#endif
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

		const char* unsupp_desc = NULL;
		switch (mode) {
			/* supported modes, we set *sectsize and leave unsupp_desc NULL */
			case MIRAGE_MODE_MODE1:
				*sectsize = 2048;
				break;
			/* unsupported modes, we leave *sectsize unmodified and set unsupp_desc */
			case MIRAGE_MODE_MODE0:
				unsupp_desc = "a Mode 0";
				break;
			case MIRAGE_MODE_AUDIO:
				unsupp_desc = "an audio";
				break;
			case MIRAGE_MODE_MODE2:
				unsupp_desc = "a Mode 2";
				break;
			case MIRAGE_MODE_MODE2_FORM1:
				unsupp_desc = "a Mode 2 Form 1";
				break;
			case MIRAGE_MODE_MODE2_FORM2:
				unsupp_desc = "a Mode 2 Form 2";
				break;
			case MIRAGE_MODE_MODE2_MIXED:
				unsupp_desc = "a mixed Mode 2";
				break;
			/* unknown mode, report it even if non-verbose and leave now */
			default:
				fprintf(stderr, "Unknown track mode (%d) for track %d (newer libmirage?)\n", mode, track_num);
				return NULL;
		}

		if (unsupp_desc) { /* got unsupported mode */
			if (verbose)
				fprintf(stderr, "Track %d is %s track (unsupported)\n", track_num, unsupp_desc);
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

const bool miragewrap_output_track(void* const out, const int track_num, FILE* const f) {
	const bool use_mmap = !!out;

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
	const int vlen = quiet ? 0 : snprintf(NULL, 0, "%d", len); /* printf() accepts <= 0 */

	guint8 *buf;
	if (!use_mmap) {
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
		if (!quiet && !(i % 64))
			fprintf(stderr, "\rTrack: %2d, sector: %*d of %d (%3d%%)", track_num, vlen, i, len, 100 * i / len);

		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, buf, &olen, &err)) {
			g_object_unref(track);
			if (!use_mmap)
				free(buf);
			return miragewrap_err("%sUnable to read sector %d", quiet ? "" : "\n", i);
		}

		if (olen != bufsize) {
			fprintf(stderr, "%sData read returned %d bytes while %d was expected\n",
					quiet ? "" : "\n", olen, bufsize);
			g_object_unref(track);
			if (!use_mmap)
				free(buf);
			return false;
		}

		if (!use_mmap) {
			if (fwrite(buf, olen, 1, f) != 1) {
				fprintf(stderr, "%sWrite failed on sector %d%s", quiet ? "" : "\n", i,
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
	if (!quiet)
		fprintf(stderr, "\rTrack: %2d, sector: %d of %d (100%%)\n", track_num, len, len);

	g_object_unref(track);
	return true;
}

void miragewrap_free(void) {
#ifdef MIRAGE_HAS_PASSWORD_SUPPORT
	mirage_forget_password();
#endif

	if (session) g_object_unref(session);
	if (disc) g_object_unref(disc);
#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (mirage) g_object_unref(mirage);
#else
	if (!libmirage_shutdown(&err))
		miragewrap_err("Library shutdown failed");
#endif
}
