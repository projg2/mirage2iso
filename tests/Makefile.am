BASE_TESTS = 00_input.iso \
	01_ecm.iso.ecm \
	01_psp.cso \
	01_gburner-4.3.gbi \
	01_poweriso_bin.bin 01_poweriso_bin.cue 01_poweriso_bin.toc \
	01_ultraiso-9.6.6.3300.ccd 01_ultraiso-9.6.6.3300.img \
	02_poweriso-4.6-bestcompression.daa 02_poweriso-4.6-encrypted.daa \
	02_poweriso-4.6-split1.part01.daa 02_poweriso-4.6-split2.part001.daa 02_poweriso-4.6-split3.daa \
	03_nerolinux-4.0.0.nrg \
	04_alcohol120_2.0.0.1331.mds \
	04_nerolinux-4.0.0-multisession.nrg

EXTRA_TEST_FILES = \
	00_second.iso \
	01_ultraiso-9.6.6.3300.sub \
	02_poweriso-4.6-split1.part02.daa 02_poweriso-4.6-split2.part002.daa 02_poweriso-4.6-split3.d00 \
	04_alcohol120_2.0.0.1331.mdf

ISZ_DMG_TESTS = \
	20_ultraiso-9.6.6.3300.isz \
	20_hdiutil_udbz.dmg \
	20_hdiutil_udco.dmg \
	20_hdiutil_udzo.dmg

# not supported by libmirage at the moment
NOT_SUPPORTED_TESTS = \
	21_hdiutil_ulfo.dmg \
	22_magiciso-5.5.uif

TESTS = $(BASE_TESTS)
if HAVE_WORKING_ISZ_DMG
TESTS += $(ISZ_DMG_TESTS)
endif

EXTRA_DIST = perform-test \
	$(BASE_TESTS) \
	$(EXTRA_TEST_FILES) \
	$(ISZ_DMG_TESTS) \
	$(NOT_SUPPORTED_TESTS)

LOG_COMPILER = $(srcdir)/perform-test
AM_LOG_FLAGS = $(top_builddir)/mirage2iso $(srcdir) $(builddir)

check-tests-extra:
	@if [ ! -f $(srcdir)/00_input.iso ]; then echo 'Please download:'; echo 'https://github.com/mgorny/mirage2iso/releases/download/mirage2iso-0.4.2/mirage2iso-0.4.2-tests.tar.xz'; echo 'unpack it, then copy files from its tests subdirectory to our tests directory.'; false; fi

check-am: check-tests-extra

clean-tests-extra:
	for t in $(TESTS); do rm -f $${t}.iso $${t}.iso.2; done
	rm -f *.log *.trs

clean-am: clean-tests-extra
