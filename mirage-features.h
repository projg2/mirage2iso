/* mirage2iso; support of NO_* defines and configure results
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef _MIRAGE_FEATURES_H
#define _MIRAGE_FEATURES_H 1

#ifdef USE_CONFIG
#	include "mirage-config.h"
#else
#	warning "You should consider calling 'make configure' first"
#endif

#ifdef NO_POSIX
#	define NO_MMAPIO
#	define NO_BSD
#endif

#ifdef NO_BSD
#	define NO_SYSEXITS
#	define NO_GNU
#endif

#ifdef NO_GNU
#	define NO_GETOPT_LONG
#endif

#endif
