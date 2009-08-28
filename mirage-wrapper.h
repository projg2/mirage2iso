/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdbool.h>

const bool miragewrap_init(void);
const bool miragewrap_setinput(const char* const fn);
const bool miragewrap_output(FILE* const f);
void miragewrap_free(void);
