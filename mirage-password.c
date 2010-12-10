/* mirage2iso; password input support
 * (c) 2009/10 Michał Górny
 * Released under the terms of the 3-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "mirage-config.h"
#endif

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_TERMIOS
#	include <termios.h>
#	include <unistd.h>
#endif

#ifdef HAVE_LIBASSUAN
#	include <stddef.h>
#	ifdef HAVE_LIBASSUAN2
#		include <gpg-error.h>
#	endif
#	include <assuan.h>
#endif

/* We use this ternary state like that:
 * - error means that something terrible has happened (I/O failure etc),
 * - success means requested operation succedeed,
 * - failure means operation failed but not because of critical error.
 *
 * Note that checking !var matches 'error' state only (that is intended).
 */
typedef enum mirage_tristate {
	error = FALSE,
	success = TRUE,
	failure
} mirage_tristate_t;

static gchar *mirage_current_password = NULL;

void mirage_forget_password(void) {
	if (mirage_current_password) {
		/* wipe out the buf */
		memset(mirage_current_password, 0, strlen(mirage_current_password));
		g_free(mirage_current_password);
		mirage_current_password = NULL;
	}
}

void mirage_set_password(gchar* const pass) {
	mirage_forget_password();
	mirage_current_password = pass;
}

#ifdef HAVE_LIBASSUAN

/* XXX: more portable solution? */
static const gchar* mirage_getshell(void) {
	const gchar* const shvar = g_getenv("SHELL");

	if (shvar && *shvar)
		return shvar;

	return "/bin/sh";
}

#ifdef HAVE_LIBASSUAN2
/* this should be less risky than redefining gpg_* for assuan1 */

typedef gpg_error_t assuan_error_t;
const assuan_error_t ASSUAN_No_Error = GPG_ERR_NO_ERROR;

const char* assuan_strerror(assuan_error_t err) {
	return gpg_strerror(err);
}

#else

assuan_error_t assuan_release(const assuan_context_t ctx) {
	assuan_disconnect(ctx);
	return 0;
}

#endif

static mirage_tristate_t mirage_pinentry_set(assuan_context_t ctx, const gchar* const cmd) {
	assuan_error_t err;
	gchar *rcvbuf;
	gsize rcvlen;

	if (((err = assuan_write_line(ctx, cmd))) != ASSUAN_No_Error) {
		g_printerr("Failed to send a command to pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		g_printerr("Failed to receive a response from pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	/* this is not critical */
	if (rcvlen != 2 || strncmp(rcvbuf, "OK", 2)) {
		g_printerr("pinentry setting failed: %s\n", cmd);
		return failure;
	}

	return success;
}

static mirage_tristate_t mirage_input_password_pinentry(void) {
	const gchar* const shell = mirage_getshell();
	const gchar* args[] = { shell, "-c", "exec pinentry", NULL };
	gint noclose[] = { -1 };
	gchar *rcvbuf;
	gsize rcvlen;

	assuan_context_t ctx;
	assuan_error_t err;

#ifdef HAVE_LIBASSUAN2
	if (((err = assuan_new(&ctx))) != GPG_ERR_NO_ERROR) {
		g_printerr("Failed to initialize libassuan: %s\n", assuan_strerror(err));
		return error;
	}

	if (((err = assuan_pipe_connect(ctx, shell, args, noclose, NULL, NULL, 0))) != GPG_ERR_NO_ERROR) {
#else
	if (((err = assuan_pipe_connect(&ctx, shell, args, noclose))) != ASSUAN_No_Error) {
#endif
		g_printerr("Failed to launch pinentry: %s\n", assuan_strerror(err));
		return error;
	}

	if (!mirage_pinentry_set(ctx, "SETDESC Enter passphrase for the encrypted image:")
			|| !mirage_pinentry_set(ctx, "SETPROMPT Pass:")
			|| !mirage_pinentry_set(ctx, "SETTITLE mirage2iso")) {

		assuan_release(ctx);
		return error;
	}

	assuan_begin_confidential(ctx);

	if (((err = assuan_write_line(ctx, "GETPIN"))) != ASSUAN_No_Error) {
		g_printerr("Failed to send the password request to pinentry: %s\n", assuan_strerror(err));
		assuan_release(ctx);
		return error;
	}

	/* we get either 'D <password>' here or 'ERR nnnnn desc' */
	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		g_printerr("Failed to receive the password response from pinentry: %s\n", assuan_strerror(err));
		assuan_release(ctx);
		return error;
	}

	if (rcvlen <= 2 || strncmp(rcvbuf, "D ", 2)) {
		g_printerr("No password supplied\n");
		assuan_release(ctx);
		return failure;
	}

	mirage_set_password(g_strndup(&rcvbuf[2], rcvlen - 2));

	/* and we should get one more 'OK' too; if we don't, we assume that connection was broken */
	if (((err = assuan_read_line(ctx, &rcvbuf, &rcvlen))) != ASSUAN_No_Error) {
		g_printerr("Failed to receive a confirmation from pinentry: %s\n", assuan_strerror(err));
		mirage_forget_password();
		assuan_release(ctx);
		return error;
	}

	if (rcvlen != 2 || strncmp(rcvbuf, "OK", 2)) {
		g_printerr("pinentry didn't confirm sent password\n");
		mirage_forget_password();
		assuan_release(ctx);
		return error;
	}

	assuan_end_confidential(ctx);
	assuan_release(ctx);

	return success;
}

#endif

static gboolean mirage_echo(const int fd, const gboolean newstate) {
#ifdef HAVE_TERMIOS
	struct termios term;

	/* noecho state should have eaten a newline */
	if (newstate)
		g_printerr("\n");

	if (tcgetattr(fd, &term) == -1)
		g_printerr("tcgetattr() failed: %s", g_strerror(errno));
	else {
		if (newstate)
			term.c_lflag |= ECHO;
		else
			term.c_lflag &= !ECHO;

		if (tcsetattr(fd, TCSANOW, &term) == -1)
			g_printerr("tcsetattr() failed: %s", g_strerror(errno));
		else
			return TRUE;
	}
#endif

	return FALSE;
}

static mirage_tristate_t mirage_input_password_stdio(void) {
	const int stdin_fileno = fileno(stdin);
	/* disable the echo before the prompt as we may output error */
	const gboolean echooff = mirage_echo(stdin_fileno, FALSE);

	GIOChannel *stdin_ch = g_io_channel_unix_new(stdin_fileno);
	GError *err = NULL;
	gchar *buf = NULL;

	g_printerr("Please input password for the encrypted image: ");

	if (g_io_channel_read_line(stdin_ch, &buf, NULL, NULL, &err) == G_IO_STATUS_ERROR) {
		if (echooff)
			mirage_echo(stdin_fileno, TRUE);
		g_printerr("Password input failed: %s\n", err->message);
		g_error_free(err);
		g_io_channel_unref(stdin_ch);
		return error;
	}
	g_io_channel_unref(stdin_ch);
	if (echooff)
		mirage_echo(stdin_fileno, TRUE);

	{
		/* remove trailing newline */
		const gint len = buf ? strlen(buf) : 0;
		gchar *last = &buf[len - 1];
		gchar *plast = &buf[len - 2];

		/* support single LF, single CR, CR/LF, LF/CR */
		if (len >= 1 && (*last == '\n' || *last == '\r')) {
			if (len >= 2 && *plast != *last && (*plast == '\r' || *plast == '\n'))
				*plast = 0;
			*last = 0;
		}
	}

	/* buf got wiped? */
	if (!buf || !*buf) {
		g_printerr("No password supplied\n");
		g_free(buf);
		return failure;
	}

	mirage_set_password(buf);
	return success;
}

const gchar* mirage_input_password(void) {
	if (mirage_current_password) /* password already there */
		return mirage_current_password;

#ifdef HAVE_LIBASSUAN
	switch (mirage_input_password_pinentry()) {
		case error: break;
		case success: return mirage_current_password;
		case failure: return NULL;
	}
#endif

	switch (mirage_input_password_stdio()) {
		case error: break;
		case success: return mirage_current_password;
		case failure: return NULL;
	}

	g_printerr("All supported methods of password input have failed\n");
	return NULL;
}
