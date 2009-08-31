/* mirage2iso; libmirage-based .iso converter
 * (c) 2009 Michał Górny
 * 3-clause BSD license
 */

/* PART ONE: let user disable all of them easily */

#ifdef NO_POSIX /* shortcut to all POSIX-related capabilities */
#	define NO_MMAPIO
#	define NO_GNU
#endif

#ifdef NO_GNU /* shortcut to all GNU extensions */
#	define NO_GETOPT_LONG
#endif

/* PART TWO: check if user has disabled all of them (easily or not) */

#undef ANY_GNU
#undef ANY_POSIX

#ifndef NO_GETOPT_LONG
#	define ANY_GNU 1
#endif

#ifndef NO_MMAPIO
#	define ANY_POSIX 1
#endif

/* PART THREE: set right C standard */

#ifdef ANY_GNU
#	define _GNU_SOURCE 1
#else
#	ifdef ANY_POSIX
#		define _POSIX_C_SOURCE 200112L
#	else
#		define _ISOC99_SOURCE 1
#	endif
#endif
