/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

#ifndef NO_MMAPIO
#	define _POSIX_C_SOURCE 200112L
#else
#	define _ISOC99_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sysexits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "mirage-wrapper.h"

bool verbose = false;

static const char* const VERSION = "0.0.1_pre";

static const struct option opts[] = {
	{ "session", required_argument, 0, 's' },
	{ "help", no_argument, 0, '?' },
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ 0, 0, 0, 0 }
};

static const int help(const char* argv0) {
	const char* const msg = "Synopsis:\n"
		"\t%s [options] <infile> <outfile.iso>\n"
		"\nOptions:\n"
		"\t--help, -?\t\tGuess what\n"
		"\t--session %%d, -s %%d\tSession to use (default: last one)\n"
		"\t--verbose, -v\t\tReport progress verbosely\n"
		"\t--version, -V\t\tPrint version number and quit\n"
		"\n";

	fprintf(stderr, msg, argv0);
	return EX_USAGE;
}

static void version(const bool mirage) {
	const char* const ver = mirage ? miragewrap_get_version() : NULL;
	fprintf(stderr, "mirage2iso %s, using libmirage %s\n", VERSION, ver ? ver : "unknown");
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

static const int output_track(const char* const fn, const int track_num) {
#ifndef NO_MMAPIO
	bool use_mmap = true;
#else
	bool use_mmap = false;
#endif

	size_t size = miragewrap_get_track_size(track_num);
	if (size == 0)
		return EX_DATAERR;

	int fd = open(fn, (use_mmap ? O_RDWR : O_WRONLY | O_TRUNC) | O_CREAT, 0666);
	if (fd == -1) {
		perror("Unable to open output file");
		return EX_CANTCREAT;
	}
	if (verbose)
		fprintf(stderr, "Output file '%s' open for track %d\n", fn, track_num);

#ifndef NO_MMAPIO
	if (ftruncate(fd, size) == -1) {
		perror("ftruncate() failed");

		if (errno == EPERM || errno == EINVAL)
			use_mmap = false; /* we can't expand the file, so will use standard I/O */
		else
			return EX_IOERR;
	}

	void *buf;

	if (use_mmap) {
		buf = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
		if (buf == MAP_FAILED) {
			use_mmap = false;
			perror("mmap() failed (trying mmap-free I/O)");
		}
	}

	if (!use_mmap) { /* we tried and we failed */
		if (close(fd) == -1)
			perror("close() failed (trying to reopen anyway)");
		fd = open(fn, O_WRONLY | O_TRUNC); /* we should have it created already */
		if (fd == -1) {
			perror("Unable to reopen output file");
			return EX_CANTCREAT;
		}
	}
#endif

	if (!use_mmap) {
		fprintf(stderr, "mmap-free I/O not implemented yet\n");
		return EX_SOFTWARE;
	}

#ifndef NO_MMAPIO
	if (!miragewrap_output_track(buf, track_num)) {
		if (munmap(buf, size))
			perror("munmap() failed");
#else
	if (0) {
#endif
		if (close(fd) == -1)
			perror("close() failed");
		return EX_IOERR;
	}

#ifndef NO_MMAPIO
	if (munmap(buf, size))
		perror("munmap() failed");
#endif

	if (close(fd) == -1) {
		perror("close() failed");
		return EX_IOERR;
	}

	return EX_OK;
}

int main(int argc, char* const argv[]) {
	int session_num = -1;

	int arg;

	while ((arg = getopt_long(argc, argv, "s:v?", opts, NULL)) != -1) {
		switch (arg) {
			case 's':
				if (!try_atoi(optarg, &session_num))
					fprintf(stderr, "--session requires integer argument which '%s' isn't\n", optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case 'V':
				version(miragewrap_init());
				return EX_OK;
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

	if (verbose)
		version(true);

	if (!miragewrap_open(argv[optind], session_num)) {
		miragewrap_free();
		return EX_DATAERR;
	}
	if (verbose)
		fprintf(stderr, "Input file '%s' open\n", argv[optind]);

	int ret;
	if (((ret = miragewrap_get_track_count())) > 1)
		fprintf(stderr, "NOTE: input session contains %d tracks; mirage2iso will read only the first one.", ret);

	if (((ret = output_track(argv[optind+1], 0))) != EX_OK) {
		miragewrap_free();
		return ret;
	}

	if (verbose)
		fprintf(stderr, "Done\n");

	miragewrap_free();
	return EX_OK;
}
