/* mirage2iso; getopt wrapper
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#include "mirage-features.h"
#include "mirage-getopt.h"

#ifndef NO_GETOPT_LONG
#	define _GNU_SOURCE 1
#	include <getopt.h>
#else
#	define _ISOC99_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static const bool try_atoi(const char* const val, int* const out) {
	char *end;
	int tmp;

	if (!*val)
		return false;

	tmp = strtol(val, &end, 0);
	if (end && *end)
		return false;

	*out = tmp;
	return true;
}

#ifndef NO_GETOPT_LONG

const short int mirage_getopt(const int argc, char* const argv[], const struct mirage_opt* const opts, union mirage_optarg_val *outval, const char* newargv[]) {
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

	if (ret == -1) { /* done parsing, set up newargv */
		int i, newargc;
		for (i = optind, newargc = 0; i < argc; i++)
			newargv[newargc++] = argv[i];
		newargv[newargc] = NULL;
		return -newargc;
	}

	for (op = opts; op->name; op++) {
		if (op->val == ret) {
			switch (op->arg) {
				case mirage_arg_int:
					if (!try_atoi(optarg, &(outval->as_int))) {
						fprintf(stderr, "--%s requires integer argument which '%s' apparently isn't\n", op->name, optarg);
						return '?';
					}
					break;
				case mirage_arg_str:
					outval->as_str = optarg;
					break;
				case mirage_arg_none:
					break;
			}
		}
	}

	return ret;
}

#else

const short int mirage_getopt(const int argc, char* const argv[], const struct mirage_opt* const opts, union mirage_optarg_val *outval, const char* newargv[]) {
	static int argindex = 1;
	static const char* shortptr = NULL;
	static int newargc = 0;

	const struct mirage_opt *op;

	while (argindex < argc) {
		const int i = argindex++;
		const char *cp = argv[i];

		const struct mirage_opt* opt = NULL;
		const char *val = NULL;

		if (shortptr || (cp[0] == '-' && cp[1] && (cp[1] != '-' || cp[2]))) {
			cp++;
			if (!shortptr && cp[0] == '-') {
				cp++;

				const char* const vp = strchr(cp, '=');
				const int cl = vp ? vp-cp : strlen(cp);

				for (op = opts; op->name; op++) {
					if (!strncmp(op->name, cp, cl) && !op->name[cl]) {
						if (vp) {
							if (op->arg == mirage_arg_none) {
								fprintf(stderr, "Option '--%s' doesn't take an argument\n", op->name);
								return '?';
							} else
								val = vp + 1;
						}
						opt = op;
						break;
					}
				}

				if (!opt) {
					fprintf(stderr, "Incorrect option: --%s\n", cp);
					return '?';
				}
			} else { /* short option */
				if (!shortptr)
					shortptr = cp;
				cp = shortptr++;

				for (op = opts; op->name; op++) {
					if (op->val == *cp) {
						if (op->arg != mirage_arg_none && *shortptr) /* value coming */
							val = shortptr;
						opt = op;
						break;
					}
				}

				if (*shortptr && !val) { /* next short arg coming? */
					argindex--;
				} else
					shortptr = NULL;

				if (!opt) {
					fprintf(stderr, "Incorrect option: -%c\n", *cp);
					return '?';
				}
			}

			if (opt->arg != mirage_arg_none) {
				if (!val) { /* need to fetch argument from next arg */
					if (!argv[argindex]) {
						fprintf(stderr, "Option '--%s' requires an argument\n", opt->name);
						return '?';
					}

					val = argv[argindex++];
				}

				switch (opt->arg) {
					case mirage_arg_int:
						if (!try_atoi(val, &(outval->as_int))) {
							fprintf(stderr, "'--%s' requires integer argument which '%s' apparently isn't\n", opt->name, val);
							return '?';
						}
						break;
					case mirage_arg_str:
						outval->as_str = val;
				}
			}

			return opt->val;
		} else if (cp[0] == '-' && cp[1] == '-') {
			int i;
			for (i = argindex; argv[i]; i++)
				newargv[newargc++] = argv[i];
		} else { /* non-option */
			newargv[newargc++] = cp;
			continue;
		}
	}

	newargv[newargc] = NULL;
	return -newargc;
}

#endif

void mirage_getopt_help(const char* const argv0, const char* const synopsis, const struct mirage_opt* const opts) {
	const char* const msg =
		"Synopsis:\n"
		"\t%s %s\n\n"
		"Options:\n";

	fprintf(stderr, msg, argv0, synopsis);

	const struct mirage_opt* op;
	for (op = opts; op->name; op++) {
		const char* formatspec;
		switch (op->arg) {
			case mirage_arg_int: formatspec = " %d"; break;
			case mirage_arg_str: formatspec = " %s"; break;
			case mirage_arg_none: formatspec = "";
		}

		const char* const addtab = (strlen(op->name) + 2 * strlen(formatspec)) >= 10 ? "" : "\t";

		fprintf(stderr, "\t--%s%s, -%c%s\t%s%s\n", op->name, formatspec, op->val, formatspec, addtab, op->help);
	}
}
