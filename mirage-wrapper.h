/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdbool.h>

const bool miragewrap_init(void);
const bool miragewrap_open(const char* const fn, const int session_num);
const int miragewrap_get_track_count(void);
const bool miragewrap_output(const int fd, const int track_num);
void miragewrap_free(void);
