/* mirage2iso; libmirage backwards compatibility
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef _MIRAGE_COMPAT_H
#define _MIRAGE_COMPAT_H 1

#if MIRAGE_MAJOR_VERSION >= 1 && MIRAGE_MINOR_VERSION >= 2
#	define MIRAGE_HAS_PASSWORD_SUPPORT
#elif MIRAGE_MAJOR_VERSION <= 1 && MIRAGE_MINOR_VERSION < 2
#	define MIRAGE_HAS_MIRAGE_OBJ
#endif

#endif
