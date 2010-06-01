# Copyright (C) 2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
RELEASE = -version-info $(LT_VERSION)
AM_CXXFLAGS = -I$(top_srcdir)/inc @UCOMMON_FLAGS@

lib_LTLIBRARIES = libucommonscript.la 

libucommonscript_la_LDFLAGS = ../common/libucommon.la @UCOMMON_LIBS@ @UCOMMON_CLINK@ $(RELEASE)
libucommonscript_la_SOURCES = common.cpp interp.cpp methods.cpp checks.cpp


