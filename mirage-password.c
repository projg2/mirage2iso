/* mirage2iso; password input support
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#include "mirage-config.h"

#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifndef NO_ASSUAN
#	include <stddef.h>
#	include <assuan.h>
#endif

static const int password_bufsize = 256;

static char *buf = NULL;

/* We use this ternary state like that:
 * - error means that something terrible has happened (I/O failure etc),
 * - success means requested operation succedeed,
 * - failure means operation failed but not because of critical error.
 *
 * Note that checking !var matches 'error' state only (that is intended).
 */
typedef enum mirage_tristate {
	error = false,
	success = true,
	failure
} mirage_tristate_t;

void mirage_forget_password(void) {
	if (buf) {
		/* wipe out the buf */
		memset(buf, 0, strlen(buf));
		free(buf);
		buf = NULL;
	}
}

#ifndef NO_ASSUAN

/* XXX: more portable solution? */
static const char* const mirage_getshell(void) {
	const char* const shvar = getenv("SHELL");

	if (shvar && *shvar)
		return shvar;

	return "/bin/sh";
}

static const mirage_tristate_t mirage_pinentry_set(assuan_context_t ctx, const char* const cmd) {
	assuan_error_t err;
	char *rcvbuf;
	size_t rcvlen;

	if (((err = assuan_write_line(ctx, cmd))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to send a command to pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to receive a response from pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	/* this is not critical */
	if (rcvlen != 2 || strncmp(rcvbuf, "OK", 2)) {
		fprintf(stderr, "pinentry setting failed: %s\n", cmd);
		return failure;
	}

	return success;
}

static const mirage_tristate_t mirage_input_password_pinentry(void) {
	const char* const shell = mirage_getshell();
	const char* const args[] = { shell, "-c", "exec pinentry", NULL };
	int noclose[] = { -1 };

	assuan_context_t ctx;
	assuan_error_t err;

	if (((err = assuan_pipe_connect(&ctx, shell, args, noclose))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to launch pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	if (!mirage_pinentry_set(ctx, "SETDESC Enter passphrase for the encrypted image:")
			|| !mirage_pinentry_set(ctx, "SETPROMPT Pass:")
			|| !mirage_pinentry_set(ctx, "SETTITLE mirage2iso")) {

		assuan_disconnect(ctx);
		return error;
	}

	assuan_begin_confidential(ctx);

	char *rcvbuf;
	size_t rcvlen;

	if (((err = assuan_write_line(ctx, "GETPIN"))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to send the password request to pinentry: %s\n", assuan_strerror(err));
		assuan_disconnect(ctx);
		return error;
	}

	/* we get either 'D <password>' here or 'ERR nnnnn desc' */
	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to receive the password response from pinentry: %s\n", assuan_strerror(err));
		assuan_disconnect(ctx);
		return error;
	}

	if (rcvlen <= 2 || strncmp(rcvbuf, "D ", 2)) {
		fprintf(stderr, "No password supplied\n");
		assuan_disconnect(ctx);
		return failure;
	}

	buf = malloc(rcvlen - 1); /* two less for 'D ', one more for \0 */
	if (!buf) {
		fprintf(stderr, "malloc() for password buffer failed\n");
		assuan_disconnect(ctx);
		return error;
	}
	strncpy(buf, &rcvbuf[2], rcvlen - 2);
	buf[rcvlen - 2] = 0;

	/* and we should get one more 'OK' too; if we don't, we assume that connection was broken */
	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		fprintf(stderr, "Failed to receive a confirmation from pinentry: %s\n", assuan_strerror(err));
		mirage_forget_password();
		assuan_disconnect(ctx);
		return error;
	}

	if (rcvlen != 2 || strncmp(rcvbuf, "OK", 2)) {
		fprintf(stderr, "pinentry didn't confirm sent password\n");
		mirage_forget_password();
		assuan_disconnect(ctx);
		return error;
	}

	assuan_end_confidential(ctx);
	assuan_disconnect(ctx);

	return success;
}

#endif

static const mirage_tristate_t mirage_input_password_stdio(void) {
	if (!buf) {
		buf = malloc(password_bufsize);
		if (!buf) {
			fprintf(stderr, "malloc() for password buffer failed\n");
			return error;
		}

		fprintf(stderr, "Please input password to the encrypted image: ");

		if (!fgets(buf, password_bufsize, stdin)) {
			fprintf(stderr, "Password input failed\n");
			mirage_forget_password();
			return error;
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
			mirage_forget_password();
			return failure;
		}
	}

	return success;
}

const char* const mirage_input_password(void) {
#ifndef NO_ASSUAN
	switch (mirage_input_password_pinentry()) {
		case error: break;
		case success: return buf;
		case failure: return NULL;
	}
#endif

	switch (mirage_input_password_stdio()) {
		case error: break;
		case success: return buf;
		case failure: return NULL;
	}

	fprintf(stderr, "All supported methods of password input have failed\n");
	return NULL;
}
