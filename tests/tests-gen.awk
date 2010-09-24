#!/usr/bin/awk -f

BEGIN {
	print "MIRAGE2ISO = mirage2iso"
	print "INPUT = 00_input.iso"
	print "SECOND = 00_second.iso"
	print
	print "all:"
	print "preclean:"
	print "	rm -f tests-failed"

	failpart = " || ( echo '	$<' >> tests-failed; false )"
}

{
	print $2 ": " $1 " preclean"
	print "	$(MIRAGE2ISO) -q -s 0 -p test $< $@" failpart
	print "	cmp $(INPUT) $@" failpart

	all = all " " $2

	if ($3) {
		print $3 ": " $1 " preclean"
		print "	$(MIRAGE2ISO) -q -s 1 -p test $< $@" failpart
		print "	cmp $(SECOND) $@" failpart

		all = all " " $3
	}
}

END {
	print "tests: " all
	print "clean: preclean"
	print "	rm -f " all
	print "distclean: clean"
	print "	rm -f Makefile"
	print "install:"
	print ".PHONY: all tests clean distclean install"
}
