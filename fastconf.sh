#!/bin/false
# fastconf basic definitions file
# Do not call directly, source within the ./configure script instead.
# (c) 2010 Michał Górny
# Released under the terms of the 3-clause BSD license.

# PART 0
# basic use checks

# The configure script should set PN and PV to the program name
# and version respectively before sourcing fastconf.sh, e.g.:
#	PN=foobar
#	PV=0.0.1
#	. ./fastconf.sh

if [ -z "${PN}" -o -z "${PV}" ]; then
	echo 'IMPORTANT: Please set ${PN} and ${PV} in the configure script!' >&2

	# We can try to guess PN but not PV.
	if [ -z "${PN}" ]; then
		PN=$(basename "${PWD}")
		echo "Falling back to PN=${PN}" >&2
	fi

	if [ -z "${PV}" ]; then
		PACKAGE=${PN}
	else
		PACKAGE=${PN}-${PV}
	fi
	echo "Guessing PACKAGE=${PACKAGE}" >&2
else
	PACKAGE=${PN}-${PV}
fi

# PART I
# command-line parsing

# Synopsis: _fc_cmdline_unset
# Cleans up the environment for a clean fc_cmdline_parse() call.
_fc_cmdline_unset() {
	unset PREFIX EXEC_PREFIX \
		BINDIR SBINDIR LIBEXECDIR SYSCONFDIR \
		LOCALSTATEDIR \
		LIBDIR INCLUDEDIR DATAROOTDIR DATADIR \
		LOCALEDIR MANDIR DOCDIR HTMLDIR
}

# Callback: conf_help
# Optional. Called after printing the standard help message. Should
# print additional option descriptions to stdout, and return true.
# stderr is dropped. If conf_help() succeeds, _fc_cmdline_help() prints
# additional blank line afterwards.

# Synopsis: _fc_cmdline_help
# Print the help message for command-line options.
_fc_cmdline_help() {
	cat <<_EOF_
Synopsis:
	./configure [options]

Options:
	--prefix=DIR		Prefix used to install arch-independent files
				(\${PREFIX}, default: /usr/local)
	--exec-prefix=DIR	Prefix used to install arch-dependent files
				(\${EXEC_PREFIX}, default: \${PREFIX})

	--bindir=DIR		Path to install user binaries
				(default: \${EXEC_PREFIX}/bin)
	--sbindir=DIR		Path to install system admin binaries
				(default: \${EXEC_PREFIX}/sbin)
	--libexecdir=DIR	Path to install program executables
				(default: \${EXEC_PREFIX}/libexec)
	--sysconfdir=DIR	Path to install read-only local data (config)
				(default: \${PREFIX}/etc)
	--localstatedir=DIR	Path to install writable local data
				(default: \${PREFIX}/var)
	--libdir=DIR		Path to install libraries
				(default: \${EXEC_PREFIX}/lib)
	--includedir=DIR	Path to install C header files
				(default: \${PREFIX}/include)
	--datarootdir=DIR	Path to install read-only system data
				(\${DATAROOTDIR}, default: \${PREFIX}/share)
	--datadir=DIR		Path to install read-only program data
				(default: \${DATAROOTDIR})
	--localedir=DIR		Path to install locale data
				(default: \${DATAROOTDIR}/locale)
	--mandir=DIR		Path to install manpages
				(default: \${DATAROOTDIR}/man)
	--docdir=DIR		Path to install documentation (\${DOCDIR},
				 default: \${DATAROOTDIR}/doc/\${PACKAGE})
	--htmldir=DIR		Path to install HTML docs
				(default: \${DOCDIR})

_EOF_

	conf_help 2>/dev/null && echo
}

# Callback: conf_arg_parse "${@}"
# Mandatory. Is called by fc_cmdline_parse() for unknown options,
# passing the remaining command-line as the argument. This function
# should return true if the option was parsed, false otherwise.

# Synopsis: _fc_cmdline_parse "${@}"
# Parses the passed command-line arguments, preserving the original
# argv.
_fc_cmdline_parse() {
	while [ ${#} -gt 0 ]; do
		case "${1}" in
			--prefix=*)
				PREFIX=${1#--prefix=}
				;;
			--exec-prefix=*)
				EXEC_PREFIX=${1#--exec-prefix=}
				;;
			--bindir=*)
				BINDIR=${1#--bindir=}
				;;
			--sbindir=*)
				SBINDIR=${1#--sbindir=}
				;;
			--libexecdir=*)
				LIBEXECDIR=${1#--libexecdir=}
				;;
			--sysconfdir=*)
				SYSCONFDIR=${1#--sysconfdir=}
				;;
			--localstatedir=*)
				LOCALSTATEDIR=${1#--localstatedir=}
				;;
			--libdir=*)
				LIBDIR=${1#--libdir=}
				;;
			--includedir=*)
				INCLUDEDIR=${1#--includedir=}
				;;
			--datarootdir=*)
				DATAROOTDIR=${1#--datarootdir=}
				;;
			--datadir=*)
				DATADIR=${1#--datadir=}
				;;
			--localedir=*)
				LOCALEDIR=${1#--localedir=}
				;;
			--mandir=*)
				MANDIR=${1#--mandir=}
				;;
			--docdir=*)
				DOCDIR=${1#--docdir=}
				;;
			--htmldir=*)
				HTMLDIR=${1#--htmldir=}
				;;
			--help)
				_fc_cmdline_help
				exit 1
				;;
			*)
				# XXX: support argument shifting in conf_arg_parse()
				if ! conf_arg_parse "${@}"; then
					# autoconf lists more than a single option here if applicable
					# but it's easier for us to print them one-by-one
					# and we keep the form to satisfy portage's QA checks
					echo "configure: WARNING: unrecognized options: ${1}" >&2
				fi
		esac

		shift
	done
}

# Callback: conf_cmdline_parsed
# Mandatory. Called when command-line parsing is done. Should setup
# local defaults and parse the results.

# Synopsis: _fc_cmdline_default
# Set default paths for directories not matched by _fc_cmdline_parse().
_fc_cmdline_default() {
	: ${PREFIX=/usr/local}
	: ${EXEC_PREFIX=${PREFIX}}

	: ${BINDIR=${EXEC_PREFIX}/bin}
	: ${SBINDIR=${EXEC_PREFIX}/sbin}
	: ${LIBEXECDIR=${EXEC_PREFIX}/libexec}
	: ${SYSCONFDIR=${PREFIX}/etc}
	: ${LOCALSTATEDIR=${PREFIX}/var}
	: ${LIBDIR=${EXEC_PREFIX}/lib}
	: ${INCLUDEDIR=${PREFIX}/include}
	: ${DATAROOTDIR=${PREFIX}/share}
	: ${DATADIR=${DATADIR}}
	: ${LOCALEDIR=${DATAROOTDIR}/locale}
	: ${MANDIR=${DATAROOTDIR}/man}
	: ${DOCDIR=${DATAROOTDIR}/doc/${PACKAGE}}
	: ${HTMLDIR=${DOCDIR}}
}

# PART II
# System checks support

# Synopsis: _fc_mkrule_code <name> <includes> <code>
# Output a Makefile rule creating a simple C program code using passed
# <includes> and <code>. The program would take the form of:
#	<includes>
#	int main(int argc, char *argv[]) { <code> }
# where <includes> and <code> can contain escapes and apostrophes have
# to be escaped.
_fc_mkrule_code() {
	printf "%s.c:\n\t@printf '%%b%s { %%b }%s' '%s' '%s' > \$@\n" \
		"${1}" '\nint main(int argc, char *argv[])' '\n' "${2}" "${3}"
}

# Synopsis: _fc_mkcall_link <infiles> [<cppflags>] [<libs>]
_fc_mkcall_link() {
	printf '\t%s %s %s %s %s %s\n' \
		'$(CC) $(CFLAGS) $(CPPFLAGS)' "${2}" \
		'$(LDFLAGS) -o $@' "${1}" \
		'$(LIBS)' "${3}"
}

# Synopsis: _fc_mkrule_compile_and_link <name> [<cppflags>] [<libs>]
_fc_mkrule_compile_and_link() {
	printf "%s: %s.c\n" "${1}" "${1}"
	_fc_mkcall_link '$<' "${2}" "${3}"
}

# Synopsis: _fc_append_test <name>
_fc_append_test() {
	FC_TESTLIST=${FC_TESTLIST+${FC_TESTLIST} }${1}
}

# Synopsis: _fc_append_source <name.c>
_fc_append_source() {
	FC_TESTLIST_SOURCES=${FC_TESTLIST_SOURCES+${FC_TESTLIST_SOURCES} }${1}
}

# Synopsis: fc_try_link <name> <includes> <code> [<cppflags>] [<libs>]
# Output a Makefile rule trying to link a test program <name>, passing
# <cppflags> and <libs> to the compiler. For the description of
# <includes> and <code> see _fc_mkrule_code().
fc_try_link() {
	_fc_mkrule_code "check-${1}" "${2}" "${3}"
	_fc_mkrule_compile_and_link "check-${1}" "${4}" "${5}"
	echo

	_fc_append_test "check-${1}"
	_fc_append_source "check-${1}.c"
}

fc_setup_makefile() {
	unset FC_TESTLIST FC_TESTLIST_SOURCES

	echo '# generated automatically by ./configure' > "${1}"
	echo '# please modify ./configure or Makefile.in instead' >> "${1}"
	echo >> "${1}"

	conf_get_tests >> "${1}"

	cat - "${2}" >> "${1}" <<_EOF_
.PHONY: config confclean

config:
	@echo "** MAKE CONFIG STARTING **" >&2
	@+\$(MAKE) confclean
	-+\$(MAKE) -k ${FC_TESTLIST}
	#@+\$(MAKE) confclean
	@echo "** MAKE CONFIG FINISHED **" >&2

confclean:
	@rm -f ${FC_TESTLIST} ${FC_TESTLIST_SOURCES}
_EOF_
}

# Synopsis: fc_def <name> [<val>]
# Output '#define <name> <val>'.
fc_def() {
	echo "#define ${1}${2+ ${2}}"
}

# Synopsis: fc_check <name> [<desc>]
# Check whether the check <name> succeeded and return either true
# or false. If <desc> is provided, print either "<desc> found"
# or "<desc> unavailable."
fc_check() {
	local testname testdesc
	testname=check-${1}
	testdesc=${2}

	set -- ${FC_TESTLIST}
	while [ ${#} -ge 1 ]; do
		if [ "${1}" = "${testname}" ]; then
			if [ -f "${1}" ]; then
				[ -n "${testdesc}" ] && echo "${testdesc} found." >&2
				return 0
			else
				[ -n "${testdesc}" ] && echo "${testdesc} unavailable." >&2
				return 1
			fi
		fi

		shift
	done

	[ -n "${testdesc}" ] && echo "${testdesc} explicitly disabled." >&2
	return 2
}

# INITIALIZATION RULES

_fc_cmdline_unset
_fc_cmdline_parse "${@}"
_fc_cmdline_default

conf_cmdline_parsed

fc_setup_makefile Makefile Makefile.in
