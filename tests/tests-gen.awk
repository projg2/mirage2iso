#!/usr/bin/awk -f

BEGIN {
	print "MIRAGE2ISO = mirage2iso"
	print "INPUT = 00_input.iso"
	print
	print "all: tests"
}

{
	print $2 ": " $1
	print "	$(MIRAGE2ISO) -q -p test $< $@"
	print "	cmp $(INPUT) $@"

	all = all " " $2
}

END {
	print "tests: " all
	print "clean:"
	print "	rm -f " all
	print ".PHONY: all tests clean"
}
