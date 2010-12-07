/* mirage2iso; password input support
 * (c) 2009 Michał Górny
 * Released under the terms of the 3-clause BSD license.
 */

#ifndef _MIRAGE_PASSWORD_H
#define _MIRAGE_PASSWORD_H 1

#include <glib.h>

const gchar* mirage_input_password(void);
void mirage_forget_password(void);
gboolean mirage_set_password(const gchar* const pass);

#endif
