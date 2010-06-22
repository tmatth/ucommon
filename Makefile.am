# Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST = autogen.sh README *.pc.in *.spec.in *.spec *-config.in \
	ucommon-config.1 Doxyfile cmodel.sh BUILDS SUPPORT COPYING* \
	COPYRIGHT CMakeLists.txt config.h.cmake

DIST_SUBDIRS = common script audio openssl insecure utils inc test
SUBDIRS = common @UCOMMON_COMPONENTS@ inc test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ucommon.pc

scriptdir = $(bindir)
script_SCRIPTS = ucommon-config

man_MANS = ucommon-config.1

.phony: doxy

doxy:
	-rm -rf doc
	$(DOXYGEN) Doxyfile

docs: doxy
	(cd script ; make docs >/dev/null)

clean-local:
	-rm -rf doc

