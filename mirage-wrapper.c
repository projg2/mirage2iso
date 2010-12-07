/* mirage2iso; libmirage interface
 * (c) 2009/10 Michał Górny
 * Released under the terms of the 3-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "mirage-config.h"
#endif

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <mirage.h>
#include "mirage-compat.h"
#include "mirage-password.h"

extern gboolean quiet;
extern gboolean verbose;

#ifdef MIRAGE_HAS_MIRAGE_OBJ
static MIRAGE_Mirage *mirage = NULL;
#endif
static MIRAGE_Disc *disc = NULL;
static MIRAGE_Session *session = NULL;
static gint tracks;

static GError *err = NULL;

static gboolean miragewrap_err(const gchar* const format, ...) {
	va_list ap;

	va_start(ap, format);
	g_vfprintf(stderr, format, ap);
	va_end(ap);

	g_printerr(": %s\n", err ? err->message : "(err undefined?!)");
	g_error_free(err);

	return FALSE;
}

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT

gchar* miragewrap_password_callback(gpointer user_data) {
	const gchar* const pass = mirage_input_password();

	if (!pass)
		return NULL;

	return g_strdup(pass);
}

#endif

gboolean miragewrap_init(void) {
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

	return TRUE;
}

#ifdef MIRAGE_HAS_MIRAGE_OBJ

const gchar* miragewrap_get_version(void) {
	static gchar buf[10];
	gchar *tmp;

	if (!mirage) {
		g_printerr("miragewrap_get_version() has to be called after miragewrap_init()\n");
		return NULL;
	}

	if (!mirage_mirage_get_version(mirage, &tmp, &err)) {
		miragewrap_err("Unable to get libmirage version");
		return NULL;
	}

	if (strlen(tmp) > 9)
		g_printerr("libmirage version string too long: %s\n", tmp);

	strncpy(buf, tmp, sizeof(buf)-1);
	buf[sizeof(buf)-1] = 0;

	g_free(tmp);
	return buf;
}

#else

const gchar* miragewrap_get_version(void) {
	return mirage_version_long;
}

#endif

gboolean miragewrap_open(const gchar* const fn, const gint session_num) {
#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!mirage) {
		g_printerr("miragewrap_open() has to be called after miragewrap_init()\n");
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
		g_free(_fn);
		return miragewrap_err("Unable to open input '%s'", fn);
	}
	g_free(_fn);

	gint sessions;

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err))
		return miragewrap_err("Unable to get session count");
	if (sessions == 0) {
		g_printerr("Input file doesn't contain any session\n");
		return FALSE;
	}

	if (!mirage_disc_get_session_by_index(disc, session_num, (GObject**) &session, &err)) {
		session = NULL;
		return miragewrap_err(session_num == -1 ? "Unable to get last session" : "Unable to get session %d", session_num);
	}

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err))
		return miragewrap_err("Unable to get track count");

	if (tracks == 0) {
		g_printerr("Input session doesn't contain any track\n");
		return FALSE;
	}

	return TRUE;
}

gint miragewrap_get_track_count(void) {
	if (!session) {
		g_printerr("miragewrap_get_track_count() has to be called after miragewrap_open()\n");
		return 0;
	}

	return tracks;
}

static MIRAGE_Track *miragewrap_get_track_common(const gint track_num, gint *sstart, gint *len, gint *sectsize) {
	MIRAGE_Track *track = NULL;

	if (!mirage_session_get_track_by_index(session, track_num, (GObject**) &track, &err)) {
		track = NULL;
		miragewrap_err("Unable to get track %d", track_num);
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

		const gchar* unsupp_desc = NULL;
		switch (mode) {
			/* supported modes, we set *sectsize and leave unsupp_desc NULL */
			case MIRAGE_MODE_MODE1:
			case MIRAGE_MODE_MODE2_FORM1:
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
			case MIRAGE_MODE_MODE2_FORM2:
				unsupp_desc = "a Mode 2 Form 2";
				break;
			case MIRAGE_MODE_MODE2_MIXED:
				unsupp_desc = "a mixed Mode 2";
				break;
			/* unknown mode, report it even if non-verbose and leave now */
			default:
				g_printerr("Unknown track mode (%d) for track %d (newer libmirage?)\n", mode, track_num);
				return NULL;
		}

		if (unsupp_desc) { /* got unsupported mode */
			if (verbose)
				g_printerr("Track %d is %s track (unsupported)\n", track_num, unsupp_desc);
			return NULL;
		}
	}

	return track;
}

gsize miragewrap_get_track_size(const gint track_num) {
	if (!session) {
		g_printerr("miragewrap_get_track_size() has to be called after miragewrap_open()\n");
		return 0;
	}

	gint sstart, len, expssize;

	MIRAGE_Track *track = miragewrap_get_track_common(track_num, &sstart, &len, &expssize);
	if (!track)
		return 0;

	g_object_unref(track);

	return expssize * (len-sstart);
}

gboolean miragewrap_output_track(gpointer const out, const gint track_num, FILE* const f) {
	const gboolean use_mmap = !!out;

	if (!session) {
		g_printerr("miragewrap_output_track() has to be called after miragewrap_open()\n");
		return 0;
	}

	gint sstart, len, bufsize;
	MIRAGE_Track *track = miragewrap_get_track_common(track_num, &sstart, &len, &bufsize);

	if (!track)
		return FALSE;

	gint i, olen;
	const gint vlen = quiet ? 0 : snprintf(NULL, 0, "%d", len); /* printf() accepts <= 0 */
	guint8* buf = use_mmap ? out : g_malloc(bufsize);

	len--; /* well, now it's rather 'last' */
	for (i = sstart; i <= len; i++) {
		if (!quiet && !(i % 64))
			g_printerr("\rTrack: %2d, sector: %*d of %d (%3d%%)", track_num, vlen, i, len, 100 * i / len);

		if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, buf, &olen, &err)) {
			g_object_unref(track);
			if (!use_mmap)
				g_free(buf);
			return miragewrap_err("%sUnable to read sector %d", quiet ? "" : "\n", i);
		}

		if (olen != bufsize) {
			g_printerr("%sData read returned %d bytes while %d was expected\n",
					quiet ? "" : "\n", olen, bufsize);
			g_object_unref(track);
			if (!use_mmap)
				g_free(buf);
			return FALSE;
		}

		if (!use_mmap) {
			if (fwrite(buf, olen, 1, f) != 1) {
				g_printerr("%sWrite failed on sector %d%s%s", quiet ? "" : "\n", i,
						ferror(f) ? ": " : " but error flag not set\n",
						ferror(f) ? g_strerror(errno) : "");
				g_object_unref(track);
				g_free(buf);
				return FALSE;
			}
		} else
			buf += olen;
	}
	if (!quiet)
		g_printerr("\rTrack: %2d, sector: %d of %d (100%%)\n", track_num, len, len);
	if (!use_mmap)
		g_free(buf);

	g_object_unref(track);
	return TRUE;
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
