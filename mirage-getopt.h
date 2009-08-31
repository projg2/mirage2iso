/* mirage2iso; getopt wrapper
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifndef NO_GETOPT_LONG
#	include <getopt.h>
#else

enum option_has_arg {
	no_argument,
	required_argument
};

struct option {
	const char *name;
	enum option_has_arg has_arg;
	int *flag;
	int val;
};

int optind;

#endif

const int mirage_getopt(int argc, char* const argv[], const struct option *longopts);
