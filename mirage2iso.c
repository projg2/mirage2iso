/* mirage2iso; libmirage-based .iso converter
 * (c) 2009/10 Michał Górny
 * Released under the terms of the 3-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "mirage-config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <sys/mman.h>
#	include <fcntl.h>
#	include <unistd.h>
#else
#	ifdef HAVE_FALLOCATE
#		include <fcntl.h>
#	endif
#endif

#ifndef NO_SYSEXITS
#	include <sysexits.h>
#else
#	define EX_OK 0
#	define EX_USAGE 64
#	define EX_DATAERR 65
#	define EX_NOINPUT 66
#	define EX_SOFTWARE 70
#	define EX_OSERR 71
#	define EX_CANTCREAT 73
#	define EX_IOERR 74
#endif

#include <glib.h>

#include "mirage-password.h"
#include "mirage-wrapper.h"

gboolean quiet = false;
gboolean verbose = false;

#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
static gboolean force_stdio = false;
#endif

static void version(const bool mirage) {
	const char* const ver = mirage ? miragewrap_get_version() : NULL;
	fprintf(stderr, "mirage2iso %s, using libmirage %s\n", VERSION, ver ? ver : "unknown");
}

static bool common_posix_filesetup(const int fd, const size_t size) {
#ifdef POSIX_FADV_NOREUSE
	if ((errno = posix_fadvise(fd, 0, 0, POSIX_FADV_NOREUSE)))
		perror("posix_fadvise() failed");
#endif

#ifdef HAVE_POSIX_FALLOCATE
	if ((errno = posix_fallocate(fd, 0, size))) {
		perror("posix_fallocate() failed");

		/* If we can't create file large enough, return false.
		 * Otherwise, try to proceed. */
#ifdef EFBIG
		if (errno == EFBIG)
			return false;
#endif
#ifdef ENOSPC
		if (errno == ENOSPC)
			return false;
#endif
	}
#endif

	return true;
}

#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
static int mmapio_open(const char* const fn, const size_t size, FILE** const f, void** const out) {
	*f = fopen(fn, "w+b");
	if (!*f) {
		perror("Unable to open output file");
		return EX_CANTCREAT;
	}

	const int fd = fileno(*f);

	if (!common_posix_filesetup(fd, size))
		return EX_CANTCREAT;

	if (ftruncate(fd, size) == -1) {
		perror("ftruncate() failed");

		if (errno == EPERM || errno == EINVAL)
			return EX_OK; /* we can't expand the file, so will try stdio */
		else
			return EX_IOERR;
	}

	void* const buf = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED)
		perror("mmap() failed");
	else
		*out = buf;

	return EX_OK;
}
#endif

static int stdio_open(const char* const fn, const size_t size, FILE** const f) {
	if (*f)
		*f = freopen(fn, "wb", *f);
	else
		*f = fopen(fn, "wb");

	if (!*f) {
		perror("Unable to open output file");
		return EX_CANTCREAT;
	}

#if (defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)) || defined(HAVE_POSIX_FALLOCATE)
	if (!common_posix_filesetup(fileno(*f), size))
		return EX_CANTCREAT;
#endif

	return EX_OK;
}

static int output_track(const char* const fn, const int track_num) {
	const bool use_stdout = !fn;

	size_t size = miragewrap_get_track_size(track_num);
	if (size == 0)
		return EX_DATAERR;

	FILE *f = NULL;
	void *out = NULL;
	int ret = EX_OK;

	if (use_stdout) {
		f = stdout;

		if (verbose)
			fprintf(stderr, "Using standard output stream for track %d\n", track_num);
	} else {
#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
		if (!force_stdio)
			ret = mmapio_open(fn, size, &f, &out);
#endif

		if (!ret && !out)
			ret = stdio_open(fn, size, &f);

		if (ret) {
			if (f) {
				if (fclose(f))
					perror("fclose() failed");
				/* We probably ate the whole disk space, so unlink the file. */
				if (remove(fn))
					perror("remove() failed");
			}

			return ret;
		}

		if (verbose)
			fprintf(stderr, "Output file '%s' open for track %d\n", fn, track_num);
	}

	if (!miragewrap_output_track(out, track_num, f)) {
#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
		if (out && munmap(out, size))
			perror("munmap() failed");
#endif
		if (!use_stdout && fclose(f))
			perror("fclose() failed");
		return EX_IOERR;
	}

#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
	if (out && munmap(out, size))
		perror("munmap() failed");
#endif

	if (!use_stdout && fclose(f)) {
		perror("fclose() failed");
		return EX_IOERR;
	}

	return EX_OK;
}

int main(int argc, char* argv[]) {
	gint session_num = -1;
	gboolean force = false;
	gboolean use_stdout = false;
	gboolean want_version = false;
	gchar **newargv = NULL;
	gchar *passbuf = NULL;

	const GOptionEntry opts[] = {
		{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force replacing the guessed output file", NULL },
		{ "password", 'p', 0, G_OPTION_ARG_STRING, &passbuf, "Password for the encrypted image", "PASS" },
		{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "Disable progress reporting, output only errors", NULL },
		{ "session", 's', 0, G_OPTION_ARG_INT, &session_num, "Session to use (default: the last one)", "N" },
		{ "stdio", 'S', 0, G_OPTION_ARG_NONE, &force_stdio, "Force using stdio instead of mmap()", NULL },
		{ "stdout", 'c', 0, G_OPTION_ARG_NONE, &use_stdout, "Output the image into stdout instead of a file", NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Increase progress reporting verbosity", NULL },
		{ "version", 'V', 0, G_OPTION_ARG_NONE, &want_version, "Print program version and exit", NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &newargv, NULL, "<in> [<out.iso>]" },
		{ NULL }
	};
	GOptionContext *opt;
	GError *err = NULL;

	opt = g_option_context_new(NULL);
	g_option_context_add_main_entries(opt, opts, NULL);

	if (!g_option_context_parse(opt, &argc, &argv, &err)) {
		g_print("Option parsing failed: %s\n", err->message);
		g_error_free(err);
		g_option_context_free(opt);
		g_free(passbuf);
		g_strfreev(newargv);
		return EX_USAGE;
	}

	if (want_version) {
		g_option_context_free(opt);
		g_free(passbuf);
		g_strfreev(newargv);
		version(true);
		return EX_OK;
	}

	if (quiet && verbose) {
		fprintf(stderr, "--verbose and --quiet are contrary options, --verbose will have precedence\n");
		quiet = false;
	}

	if (use_stdout) {
#if defined(HAVE_FTRUNCATE) && defined(HAVE_MMAP)
		if (force_stdio && !quiet)
			fprintf(stderr, "--stdout already implies --stdio, no need to specify it\n");
		else
			force_stdio = true;
#endif
		if (force && !quiet)
			fprintf(stderr, "--force has no effect when --stdout in use\n");
	}

	if (passbuf) {
		mirage_set_password(passbuf);
		g_free(passbuf);
	}

	const char* in;
	if (!newargv || !(in = newargv[0])) {
		gchar* const helpmsg = g_option_context_get_help(opt, TRUE, NULL);
		fprintf(stderr, "No input file specified\n");
		g_print("%s", helpmsg);
		g_free(helpmsg);
		g_option_context_free(opt);
		g_strfreev(newargv);
		return EX_USAGE;
	}
	g_option_context_free(opt);

	const char* out = newargv[1];
	char* outbuf = NULL;
	if (!out) {
		if (!use_stdout) {
			const char* ext = strrchr(in, '.');

			if (ext && !strcmp(ext, ".iso")) {
				if (!force) {
					fprintf(stderr, "Input file has .iso suffix and no output file specified\n"
							"Either specify one or use --force to use '.iso.iso' output suffix\n");
					g_strfreev(newargv);
					return EX_USAGE;
				}
				ext = NULL;
			}

			const int namelen = strlen(in) - (ext ? strlen(ext) : 0);

			outbuf = malloc(namelen + 5);
			if (!outbuf) {
				perror("malloc() for output filename failed");
				g_strfreev(newargv);
				return EX_OSERR;
			}
			strncpy(outbuf, in, namelen);
			strcat(outbuf, ".iso");

			if (!force) {
				FILE *tmp = fopen(outbuf, "r");
				if (tmp || errno != ENOENT) {
					if (tmp && fclose(tmp))
						perror("fclose(tmp) failed");

					fprintf(stderr, "No output file specified and guessed filename matches existing file:\n\t%s\n", outbuf);
					free(outbuf);
					g_strfreev(newargv);
					return EX_USAGE;
				}
			}

			out = outbuf;
		}
	} else if (use_stdout) {
		fprintf(stderr, "Output file can't be specified with --stdout\n");
		g_strfreev(newargv);
		return EX_USAGE;
	}

	if (!miragewrap_init()) {
		g_strfreev(newargv);
		return EX_SOFTWARE;
	}

	if (verbose)
		version(true);

	if (!miragewrap_open(in, session_num)) {
		miragewrap_free();
		g_strfreev(newargv);
		return EX_NOINPUT;
	}
	if (verbose)
		fprintf(stderr, "Input file '%s' open\n", in);

	int tcount;
	if (((tcount = miragewrap_get_track_count())) > 1 && !quiet)
		fprintf(stderr, "NOTE: input session contains %d tracks; mirage2iso will read only the first usable one\n", tcount);

	int i, ret = !EX_OK;
	for (i = 0; ret != EX_OK && i < tcount; i++) {
		ret = output_track(out, i);

		if (ret != EX_OK && ret != EX_DATAERR) {
			miragewrap_free();
			g_strfreev(newargv);
			return ret;
		}
	}

	if (outbuf)
		free(outbuf);

	if (ret != EX_OK) /* no valid track found */
		fprintf(stderr, "No supported track found (audio CD?)\n");
	else if (verbose)
		fprintf(stderr, "Done\n");

	miragewrap_free();
	g_strfreev(newargv);
	return EX_OK;
}
