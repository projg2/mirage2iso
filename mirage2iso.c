/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>

#include <getopt.h>

#include "mirage-wrapper.h"

const struct option opts[] = {
	{ 0, 0, 0, 0 }
};

static const int help(const char *argv0) {
	fprintf(stderr, "Synopsis: %s <infile> <outfile.iso>\n", argv0);
	return EX_USAGE;
}

int main(int argc, char* const argv[]) {
	int arg;

	while ((arg = getopt_long(argc, argv, "", opts, NULL)) != -1) {
	}

	if (!argv[optind] || !argv[optind+1])
		return help(argv[0]);

	if (!miragewrap_init())
		return EX_SOFTWARE;

	if (!miragewrap_open(argv[optind])) {
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
