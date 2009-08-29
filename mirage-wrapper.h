/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdbool.h>

const bool miragewrap_init(void);
const char* const miragewrap_get_version(void);
const bool miragewrap_open(const char* const fn, const int session_num);
const int miragewrap_get_track_count(void);
const size_t miragewrap_get_track_size(const int track_num);
const bool miragewrap_output_track(void *out, const int track_num);
void miragewrap_free(void);
