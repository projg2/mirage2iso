/* mirage2iso; password input support
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const int password_bufsize = 256;

static char *buf = NULL;

const char* const mirage_input_password(void) {
	if (!buf) {
		buf = malloc(password_bufsize);
		if (!buf) {
			fprintf(stderr, "malloc() for password buffer failed\n");
			return NULL;
		}

		fprintf(stderr, "Please input password to the encrypted image: ");

		if (!fgets(buf, password_bufsize, stdin)) {
			fprintf(stderr, "Password input failed\n");
			free(buf);
			return NULL;
		}

		/* remove trailing newline */
		const int len = strlen(buf);
		char *last = &buf[len - 1];
		char *plast = &buf[len - 2];

		/* support single LF, single CR, CR/LF, LF/CR */
		if (len >= 1 && (*last == '\n' || *last == '\r')) {
			if (len >= 2 && *plast != *last && (*plast == '\r' || *plast == '\n'))
				*plast = 0;
			*last = 0;
		}

		/* buf got wiped? */
		if (!*buf) {
			fprintf(stderr, "No password supplied\n");
			free(buf);
			return NULL;
		}
	}

	return buf;
}

void mirage_forget_password(void) {
	if (buf) {
		/* wipe out the buf */
		memset(buf, 0, password_bufsize);
		free(buf);
	}
}
