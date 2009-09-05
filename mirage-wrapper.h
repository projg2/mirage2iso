/* mirage2iso; libmirage interface
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef _MIRAGE_WRAPPER_H
#define _MIRAGE_WRAPPER_H 1

#include <stdbool.h>

bool miragewrap_init(void);
const char* miragewrap_get_version(void);
bool miragewrap_open(const char* const fn, const int session_num);
int miragewrap_get_track_count(void);
size_t miragewrap_get_track_size(const int track_num);
bool miragewrap_output_track(void* const out, const int track_num, FILE* const f);
void miragewrap_free(void);

#endif
