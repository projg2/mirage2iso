/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sysexits.h>

#include <getopt.h>

#include "mirage-wrapper.h"

static const struct option opts[] = {
	{ "session", required_argument, 0, 's' },
	{ "help", no_argument, 0, '?' },
	{ 0, 0, 0, 0 }
};

static const int help(const char* argv0) {
	const char* const msg = "Synopsis: %s <options> <infile> <outfile.iso>\n"
		"\nOptions:\n"
		"\t--session %%d, -s %%d\tSession to use (default: last one)\n";

	fprintf(stderr, msg, argv0);
	return EX_USAGE;
}

static const bool try_atoi(const char* const val, int* const out) {
	char *end;
	int tmp;

	tmp = strtol(val, &end, 0);
	if (end && *end)
		return false;

	*out = tmp;
	return true;
}

int main(int argc, char* const argv[]) {
	int session_num = -1;
	int arg, val;

	while ((arg = getopt_long(argc, argv, "s:", opts, NULL)) != -1) {
		switch (arg) {
			case 's':
				if (!try_atoi(optarg, &session_num))
					fprintf(stderr, "--session requires integer argument which '%s' isn't\n", optarg);
				break;
			case '?':
				return help(argv[0]);
		}
	}

	if (!argv[optind]) {
		fprintf(stderr, "No input file specified\n");
		return help(argv[0]);
	}
	
	if (!argv[optind+1]) {
		fprintf(stderr, "No output file specified\n");
		return help(argv[0]);
	}

	if (!miragewrap_init())
		return EX_SOFTWARE;

	if (!miragewrap_open(argv[optind], session_num)) {
		miragewrap_free();
		return EX_DATAERR;
	}

	FILE* const out = fopen(argv[optind+1], "w+");
	if (!out) {
		perror("Unable to open output file");
		miragewrap_free();
		return EX_CANTCREAT;
	}

	if (!miragewrap_output(fileno(out))) {
		miragewrap_free();
		return EX_IOERR;
	}

	miragewrap_free();
	return EX_OK;
}
