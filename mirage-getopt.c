/* mirage2iso; getopt wrapper
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#include "mirage-features.h"

#ifndef NO_GETOPT_LONG

#define _GNU_SOURCE 1
#include <stdlib.h>
#include <getopt.h>

const int mirage_getopt(int argc, char* const argv[], const struct option *longopts) {
	const struct option *op;
	int len = 1; /* trailing \0 */

	for (op = longopts; op->name; op++) {
		if (op->val) {
			len++;
			if (op->has_arg != no_argument)
				len++;
		}
	}

	char buf[len];
	char* bufptr = buf;

	for (op = longopts; op->name; op++) {
		if (op->val) {
			*(bufptr++) = op->val;
			if (op->has_arg == required_argument)
				*(bufptr++) = ':';
		}
	}
	*bufptr = 0;

	return getopt_long(argc, argv, buf, longopts, NULL);
}

#else

#define _ISOC99_SOURCE 1
#include "mirage-getopt.h"

#warning "Currently NO_GETOPT_LONG implies *no* argument parsing, sorry."

const int mirage_getopt(int argc, char* const argv[], const struct option *longopts) {
	optind = 1;

	return -1;
}

#endif
