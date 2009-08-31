/* mirage2iso; support of NO_* defines and setting _*_SOURCE
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifdef NO_POSIX
#	define NO_MMAPIO
#	define NO_GNU
#endif

#ifdef NO_GNU
#	define NO_GETOPT_LONG
#endif
