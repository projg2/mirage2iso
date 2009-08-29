/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdbool.h>

const bool miragewrap_init(void);
const bool miragewrap_open(const char* const fn, const int session_num);
const bool miragewrap_output(const int fd);
void miragewrap_free(void);
