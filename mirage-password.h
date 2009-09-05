/* mirage2iso; password input support
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef _MIRAGE_PASSWORD_H
#define _MIRAGE_PASSWORD_H 1

const char* const mirage_input_password(void);
void mirage_forget_password(void);
bool mirage_set_password(const char* const pass);

#endif
