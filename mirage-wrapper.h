/* mirage2iso; libmirage interface
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef _MIRAGE_WRAPPER_H
#define _MIRAGE_WRAPPER_H 1

#include <stdbool.h>

const bool miragewrap_init(void);
const char* const miragewrap_get_version(void);
const bool miragewrap_open(const char* const fn, const int session_num);
const int miragewrap_get_track_count(void);
const size_t miragewrap_get_track_size(const int track_num);
const bool miragewrap_output_track(void *out, const int track_num, const bool use_mmap);
void miragewrap_free(void);

#endif
