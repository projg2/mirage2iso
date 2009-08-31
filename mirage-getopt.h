/* mirage2iso; getopt wrapper
 * (c) 2009 Michał Górny
 * released under 3-clause BSD license
 */

#ifdef NO_GETOPT_LONG
int optind;
#endif

enum mirage_optarg {
	mirage_arg_none,
	mirage_arg_int
};

struct mirage_opt {
	const char* const name;
	enum mirage_optarg arg;
	short int val;
	const char* const help;
};

const short int mirage_getopt(int argc, char* const argv[], const struct mirage_opt* const opts);
void mirage_getopt_help(const char* const argv0, const char* const synopsis, const struct mirage_opt* const opts);
