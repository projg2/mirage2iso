/* mirage2iso; getopt wrapper
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#include "mirage-features.h"
#include "mirage-getopt.h"

#ifndef NO_GETOPT_LONG
#	define _GNU_SOURCE 1
#	include <stdlib.h>
#	include <getopt.h>
#else
#	define _ISOC99_SOURCE 1
#	warning "Currently NO_GETOPT_LONG implies *no* argument parsing, sorry."
#endif

#include <stdio.h>
#include <string.h>

const short int mirage_getopt(const int argc, char* const argv[], const struct mirage_opt* const opts) {
#ifndef NO_GETOPT_LONG
	const struct mirage_opt *op;
	int arrlen = 1, buflen = 1;

	for (op = opts; op->name; op++) {
		arrlen++;
		if (op->val) {
			buflen++;
			if (op->arg != mirage_arg_none)
				buflen++;
		}
	}

	char buf[buflen + 1];
	struct option longopts[arrlen + 1];
	char* bufptr = buf;
	struct option *optptr = longopts;

	for (op = opts; op->name; op++, optptr++) {
		optptr->name = op->name;
		optptr->has_arg = op->arg == mirage_arg_none ? no_argument : required_argument;
		optptr->flag = NULL;
		optptr->val = op->val;

		if (op->val) {
			*(bufptr++) = op->val;
			if (op->arg != mirage_arg_none)
				*(bufptr++) = ':';
		}
	}
	*bufptr = 0;
	memset(optptr, 0, sizeof(*optptr));

	const int ret = getopt_long(argc, argv, buf, longopts, NULL);

	if (ret == -1) /* done parsing */
		return -optind;

	return ret;
#else
	return -1;
#endif
}

void mirage_getopt_help(const char* const argv0, const char* const synopsis, const struct mirage_opt* const opts) {
	const char* const msg =
		"Synopsis:\n"
		"\t%s %s\n\n"
		"Options:\n";

	fprintf(stderr, msg, argv0, synopsis);

	const struct mirage_opt* op;
	for (op = opts; op->name; op++) {
		const char* formatspec = "";
		switch (op->arg) {
			case mirage_arg_int: formatspec = " %d"; break;
		}

		const char* const addtab = (strlen(op->name) + 2 * strlen(formatspec)) >= 10 ? "" : "\t";

		fprintf(stderr, "\t--%s%s, -%c%s\t%s%s\n", op->name, formatspec, op->val, formatspec, addtab, op->help);
	}
}
