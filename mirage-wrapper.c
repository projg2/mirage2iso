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
#include "mirage-wrapper.h"

extern gboolean quiet;
extern gboolean verbose;

#ifdef MIRAGE_HAS_MIRAGE_OBJ
static MIRAGE_Mirage *mirage = NULL;
#endif
static MIRAGE_Disc *disc = NULL;
static MIRAGE_Session *session = NULL;
static gint tracks;

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT

gchar* miragewrap_password_callback(gpointer user_data) {
	const gchar* const pass = mirage_input_password();

	if (!pass)
		return NULL;

	return g_strdup(pass);
}

#endif

gboolean miragewrap_init(void) {
	GError *err = NULL;

	g_type_init();

#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!((mirage = g_object_new(MIRAGE_TYPE_MIRAGE, NULL))))
		return FALSE;
#else
	if (!libmirage_init(&err)) {
		g_printerr("Unable to init libmirage: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
#endif

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT
	if (!libmirage_set_password_function(miragewrap_password_callback, NULL, &err)) {
		g_printerr("Unable to set password callback: %s\n", err->message);
		g_error_free(err);
	}
#endif

	return TRUE;
}

#ifdef MIRAGE_HAS_MIRAGE_OBJ

const gchar* miragewrap_get_version(void) {
	static gchar buf[10];
	gchar *tmp;
	GError *err = NULL;

	if (!mirage) {
		g_printerr("miragewrap_get_version() has to be called after miragewrap_init()\n");
		return NULL;
	}

	if (!mirage_mirage_get_version(mirage, &tmp, &err)) {
		g_printerr("Unable to get libmirage version: %s\n", err->message);
		g_error_free(err);
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
	GError *err = NULL;
	gchar *filenames[] = { NULL, NULL };
	gint sessions;
	gchar *_fn;

#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!mirage) {
		g_printerr("miragewrap_open() has to be called after miragewrap_init()\n");
		return FALSE;
	}
#endif /* XXX: add some check for new API */

	_fn = g_strdup(fn);
	filenames[0] = _fn;

#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (!mirage_mirage_create_disc(mirage, filenames, (GObject**) &disc, NULL, &err)) {
#else
	if (!((disc = (MIRAGE_Disc*) libmirage_create_disc(filenames, NULL, NULL, &err)))) {
#endif
		g_printerr("Unable to open input '%s': %s\n", fn, err->message);
		disc = NULL;
		g_free(_fn);
		g_error_free(err);
		return FALSE;
	}
	g_free(_fn);

	if (!mirage_disc_get_number_of_sessions(disc, &sessions, &err)) {
		g_printerr("Unable to get session count: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	if (sessions == 0) {
		g_printerr("Input file doesn't contain any session\n");
		return FALSE;
	}

	if (!mirage_disc_get_session_by_index(disc, session_num, (GObject**) &session, &err)) {
		session = NULL;
		if (session_num == -1)
			g_printerr("Unable to get the last session: %s\n", err->message);
		else
			g_printerr("Unable to get session %d: %s\n", session_num, err->message);
		g_error_free(err);
		return FALSE;
	}

	if (!mirage_session_get_number_of_tracks(session, &tracks, &err)) {
		g_printerr("Unable to get track count: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

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
	GError *err = NULL;

	if (!mirage_session_get_track_by_index(session, track_num, (GObject**) &track, &err)) {
		g_printerr("Unable to get track %d: %s\n", track_num, err->message);
		track = NULL;
		g_error_free(err);
		return NULL;
	}

	if (sstart) {
		if (!mirage_track_get_track_start(track, sstart, &err)) {
			g_printerr("Unable to get track start for track %d: %s\n", track_num, err->message);
			g_object_unref(track);
			g_error_free(err);
			return NULL;
		}
	}

	if (len) {
		if (!mirage_track_layout_get_length(track, len, &err)) {
			g_printerr("Unable to get track length for track %d: %s\n", track_num, err->message);
			g_object_unref(track);
			g_error_free(err);
			return NULL;
		}
	}

	if (sectsize) {
		gint mode;
		const gchar *unsupp_desc;

		if (!mirage_track_get_mode(track, &mode, &err)) {
			g_printerr("Unable to get track mode for track %d: %s\n", track_num, err->message);
			g_object_unref(track);
			g_error_free(err);
			return 0;
		}

		unsupp_desc = NULL;
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
	gint sstart, len, expssize;
	MIRAGE_Track *track;

	if (!session) {
		g_printerr("miragewrap_get_track_size() has to be called after miragewrap_open()\n");
		return 0;
	}

	track = miragewrap_get_track_common(track_num, &sstart, &len, &expssize);
	if (!track)
		return 0;

	g_object_unref(track);

	return expssize * (len-sstart);
}

gboolean miragewrap_output_track(const gint track_num, FILE* const f,
		void (*report_progress)(gint, gint, gint)) {
	GError *err = NULL;
	gint sstart, len, bufsize;
	MIRAGE_Track *track;

	if (!session) {
		g_printerr("miragewrap_output_track() has to be called after miragewrap_open()\n");
		return 0;
	}

	track = miragewrap_get_track_common(track_num, &sstart, &len, &bufsize);
	if (!track)
		return FALSE;

	{
		gint i, olen;
		guint8* buf = g_malloc(bufsize);

		len--; /* well, now it's rather 'last' */
		if (!quiet)
			report_progress(-1, 0, len);
		for (i = sstart; i <= len; i++) {
			if (!quiet && !(i % 64))
				report_progress(track_num, i, len);

			if (!mirage_track_read_sector(track, i, FALSE, MIRAGE_MCSB_DATA, 0, buf, &olen, &err)) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Unable to read sector %d: %s\n", i, err->message);
				g_object_unref(track);
				g_free(buf);
				g_error_free(err);
				return FALSE;
			}

			if (olen != bufsize) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Data read returned %d bytes while %d was expected\n",
						olen, bufsize);
				g_object_unref(track);
				g_free(buf);
				return FALSE;
			}

			if (fwrite(buf, olen, 1, f) != 1) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Write failed on sector %d%s%s", i,
						ferror(f) ? ": " : " but error flag not set\n",
						ferror(f) ? g_strerror(errno) : "");
				g_object_unref(track);
				g_free(buf);
				return FALSE;
			}
		}

		if (!quiet) {
			report_progress(track_num, len, len);
			report_progress(-1, 0, 0);
		}
		g_free(buf);
	}

	g_object_unref(track);
	return TRUE;
}

void miragewrap_free(void) {
	GError *err;

#ifdef MIRAGE_HAS_PASSWORD_SUPPORT
	mirage_forget_password();
#endif

	if (session) g_object_unref(session);
	if (disc) g_object_unref(disc);
#ifdef MIRAGE_HAS_MIRAGE_OBJ
	if (mirage) g_object_unref(mirage);
#else
	if (!libmirage_shutdown(&err)) {
		g_printerr("Library shutdown failed: %s\n", err->message);
		g_error_free(err);
	}
#endif
}
