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

#if HAVE_LIBMIRAGE3
#   include <mirage/mirage.h>
#else
#   include <mirage.h>
#endif
#include "mirage-password.h"
#include "mirage-wrapper.h"

extern gboolean quiet;
extern gboolean verbose;

static MirageContext *mirage = NULL;
static MirageDisc *disc = NULL;
static MirageSession *session = NULL;
static gint tracks;

gchar* miragewrap_password_callback(gpointer user_data) {
	const gchar* const pass = mirage_input_password();

	if (!pass)
		return NULL;

	return g_strdup(pass);
}

gboolean miragewrap_init(void) {
	GError *err = NULL;

#if !defined(GLIB_VERSION_2_36)
	g_type_init();
#endif

#if !defined(MIRAGE_API12) || defined(MIRAGE_API20)
	if (!((mirage = g_object_new(MIRAGE_TYPE_CONTEXT, NULL))))
		return FALSE;
#endif

	if (!mirage_initialize(&err)) {
		g_printerr("Unable to init libmirage: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	mirage_context_set_password_function(mirage,
			miragewrap_password_callback, NULL);

	return TRUE;
}

const gchar* miragewrap_get_version(void) {
	return mirage_version_long;
}

gboolean miragewrap_open(const gchar* const fn, const gint session_num) {
	GError *err = NULL;
	gchar *filenames[] = { NULL, NULL };
	gint sessions;
	gchar *_fn;

	if (!mirage) {
		g_printerr("miragewrap_open() has to be called after miragewrap_init()\n");
		return FALSE;
	}

	_fn = g_strdup(fn);
	filenames[0] = _fn;

	disc = mirage_context_load_image(mirage, filenames, &err);
	if (!disc) {
		g_printerr("Unable to open input '%s': %s\n", fn, err->message);
		g_free(_fn);
		g_error_free(err);
		return FALSE;
	}
	g_free(_fn);

	sessions = mirage_disc_get_number_of_sessions(disc);
	if (sessions == 0) {
		g_printerr("Input file doesn't contain any session\n");
		return FALSE;
	}

	session = mirage_disc_get_session_by_index(disc, session_num, &err);
	if (!session) {
		if (session_num == -1)
			g_printerr("Unable to get the last session: %s\n", err->message);
		else
			g_printerr("Unable to get session %d: %s\n", session_num, err->message);
		g_error_free(err);
		return FALSE;
	}

	tracks = mirage_session_get_number_of_tracks(session);
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

static MirageTrack *miragewrap_get_track_common(const gint track_num, gint *sstart, gint *len, gint *sectsize) {
	MirageTrack *track = NULL;
	GError *err = NULL;

	track = mirage_session_get_track_by_index(session, track_num, &err);
	if (!track) {
		g_printerr("Unable to get track %d: %s\n", track_num, err->message);
		g_error_free(err);
		return NULL;
	}

	if (sstart)
		*sstart = mirage_track_get_track_start(track);

	if (len)
		*len = mirage_track_layout_get_length(track);

	if (sectsize) {
		gint sector_type;
		const gchar *unsupp_desc;

#if MIRAGE_VERSION_MAJOR >= 3
		sector_type = mirage_track_get_sector_type(track);
#else
		sector_type = mirage_track_get_mode(track);
#endif

		unsupp_desc = NULL;
		switch (sector_type) {
			/* supported sector types, we set *sectsize and leave unsupp_desc NULL */
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_MODE1:
			case MIRAGE_SECTOR_MODE2_FORM1:
#else
			case MIRAGE_MODE_MODE1:
			case MIRAGE_MODE_MODE2_FORM1:
#endif
				*sectsize = 2048;
				break;
			/* unsupported sector types, we leave *sectsize unmodified and set unsupp_desc */
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_MODE0:
#else
			case MIRAGE_MODE_MODE0:
#endif
				unsupp_desc = "a Mode 0";
				break;
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_AUDIO:
#else
			case MIRAGE_MODE_AUDIO:
#endif
				unsupp_desc = "an audio";
				break;
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_MODE2:
#else
			case MIRAGE_MODE_MODE2:
#endif
				unsupp_desc = "a Mode 2";
				break;
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_MODE2_FORM2:
#else
			case MIRAGE_MODE_MODE2_FORM2:
#endif
				unsupp_desc = "a Mode 2 Form 2";
				break;
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_MODE2_MIXED:
#else
			case MIRAGE_MODE_MODE2_MIXED:
#endif
				unsupp_desc = "a mixed Mode 2";
				break;
#if MIRAGE_VERSION_MAJOR >= 3
			case MIRAGE_SECTOR_RAW:
				unsupp_desc = "a raw";
				break;
			case MIRAGE_SECTOR_RAW_SCRAMBLED:
				unsupp_desc = "a scrambled raw";
				break;
#endif
			/* unknown sector type, report it even if non-verbose and leave now */
			default:
				g_printerr("Unknown track sector type / mode (%d) for track %d (newer libmirage?)\n", sector_type, track_num);
				return NULL;
		}

		if (unsupp_desc) { /* got unsupported sector type */
			if (verbose)
				g_printerr("Track %d is %s track (unsupported)\n", track_num, unsupp_desc);
			return NULL;
		}
	}

	return track;
}

gsize miragewrap_get_track_size(const gint track_num) {
	gint sstart, len, expssize;
	MirageTrack *track;

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
	MirageTrack *track;

	if (!session) {
		g_printerr("miragewrap_output_track() has to be called after miragewrap_open()\n");
		return 0;
	}

	track = miragewrap_get_track_common(track_num, &sstart, &len, &bufsize);
	if (!track)
		return FALSE;

	{
		gint i, olen;
		const guint8* buf;

		MirageSector *sect;

		len--; /* well, now it's rather 'last' */
		if (!quiet)
			report_progress(-1, 0, len);
		for (i = sstart; i <= len; i++) {
			if (!quiet && !(i % 64))
				report_progress(track_num, i, len);

			sect = mirage_track_get_sector(track, i, FALSE, &err);
			if (!sect) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Unable to get sector %d: %s\n", i, err->message);
				g_object_unref(track);
				g_error_free(err);
				return FALSE;
			}

			if (!mirage_sector_get_data(sect, &buf, &olen, &err)) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Unable to read sector %d: %s\n", i, err->message);
				g_object_unref(sect);
				g_object_unref(track);
				g_error_free(err);
				return FALSE;
			}

			if (olen != bufsize) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Data read returned %d bytes while %d was expected\n",
						olen, bufsize);
				g_object_unref(sect);
				g_object_unref(track);
				return FALSE;
			}

			if (fwrite(buf, olen, 1, f) != 1) {
				if (!quiet)
					report_progress(-1, 0, 0);
				g_printerr("Write failed on sector %d%s%s", i,
						ferror(f) ? ": " : " but error flag not set\n",
						ferror(f) ? g_strerror(errno) : "");
				g_object_unref(sect);
				g_object_unref(track);
				return FALSE;
			}

			g_object_unref(sect);
		}

		if (!quiet) {
			report_progress(track_num, len, len);
			report_progress(-1, 0, 0);
		}
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
	if (!mirage_shutdown(&err)) {
		g_printerr("Library shutdown failed: %s\n", err->message);
		g_error_free(err);
	}
#endif
}
