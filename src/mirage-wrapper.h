/* mirage2iso; libmirage interface
 * (c) 2009 Michał Górny
 * Released under the terms of the 3-clause BSD license.
 */

#ifndef _MIRAGE_WRAPPER_H
#define _MIRAGE_WRAPPER_H 1

#include <glib.h>

gboolean miragewrap_init(void);
const gchar* miragewrap_get_version(void);
gboolean miragewrap_open(const gchar* const fn, const gint session_num);
gint miragewrap_get_track_count(void);
gsize miragewrap_get_track_size(const gint track_num);
gboolean miragewrap_output_track(const gint track_num, FILE* const f,
		void (*report_progress)(gint, gint, gint));
void miragewrap_free(void);

#endif
