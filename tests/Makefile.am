TESTS = 00_input.iso \
	01_poweriso_bin.bin 01_poweriso_bin.cue 01_poweriso_bin.toc \
	02_poweriso-4.6-bestcompression.daa 02_poweriso-4.6-encrypted.daa \
	02_poweriso-4.6-split1.part01.daa 02_poweriso-4.6-split2.part001.daa 02_poweriso-4.6-split3.daa \
	03_nerolinux-4.0.0.nrg \
	04_alcohol120_2.0.0.1331.mds \
	04_nerolinux-4.0.0-multisession.nrg

EXTRA_DIST = perform-test

LOG_COMPILER = $(srcdir)/perform-test
AM_LOG_FLAGS = $(top_builddir)/mirage2iso

check-tests-extra:
	@if [ ! -f $(srcdir)/00_input.iso ]; then echo 'Please download:'; echo 'https://github.com/mgorny/mirage2iso/releases/download/mirage2iso-0.3/mirage2iso-0.3-tests.tar.xz'; echo 'unpack it, then copy files from its tests subdirectory to our tests directory.'; false; fi

check-am: check-tests-extra

clean-tests-extra:
	for t in $(TESTS); do rm -f $${t}.iso $${t}.iso.2; done

clean-am: clean-tests-extra
