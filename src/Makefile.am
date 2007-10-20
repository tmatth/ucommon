# Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
RELEASE = -version-info $(LT_VERSION) -release $(LT_RELEASE)
MODEL = $(UCOMMON_MODEL)
LIBTOOL += --tag=$(MODEL)
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS)
CXXLINK = $(LIBTOOL) --tag=$(MODEL) --mode=link $(CXX) $(AM_CFLAGS) $(CFLAGS) \
    $(AM_LDFLAGS) $(LDFLAGS) -o $@

lib_LTLIBRARIES = libucommon.la 

# libucommon_la_CXXFLAGS = $(UCOMMON_FLAGS)
libucommon_la_LDFLAGS = --tag=$(MODEL) @UCOMMON_LIBS@ @UCOMMON_CLINK@ $(RELEASE) 
libucommon_la_SOURCES = object.cpp linked.cpp string.cpp \
	counter.cpp bitmap.cpp timer.cpp memory.cpp socket.cpp \
	access.cpp thread.cpp cpr.cpp vector.cpp


