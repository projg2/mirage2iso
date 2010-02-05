#!/usr/bin/awk -f

BEGIN {
	print "MIRAGE2ISO = mirage2iso"
	print "INPUT = 00_input.iso"
	print
	print "all: tests"
	print "preclean:"
	print "	rm -f tests-failed"

	failpart = " || ( echo '	$<' >> tests-failed; false )"
}

{
	print $2 ": " $1 " preclean"
	print "	$(MIRAGE2ISO) -q -p test $< $@" failpart
	print "	cmp $(INPUT) $@" failpart

	all = all " " $2
}

END {
	print "tests: " all
	print "clean:"
	print "	rm -f " all
	print "distclean: clean"
	print "	rm -f Makefile"
	print ".PHONY: all tests clean distclean"
}
