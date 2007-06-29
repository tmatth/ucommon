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
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS) 
MODEL=@UCOMMON_MODEL@
CXXLINK = $(LIBTOOL) --tag=$(MODEL) --mode=link $(CXX) $(AM_CXXFLAGS) $(CFLAGS) \
    $(AM_LDFLAGS) $(LDFLAGS) -o $@
LDADD = ../src/libucommon.la @UCOMMON_LIBS@ @UCOMMON_CLINK@
EXTRA_DIST = stdcpp.cpp

TESTS = ucommonLinked ucommonStrings ucommonThreads
check_PROGRAMS = $(TESTS)

ucommonThreads_SOURCES = thread.cpp
ucommonStrings_SOURCES = string.cpp
ucommonLinked_SOURCES = linked.cpp

# test using full stdc++ linkage...
stdcpp:	stdcpp.cpp
	$(CXX) @UCOMMON_FLAGS@ -o stdcpp -I../inc -L../src/.libs -lucommon stdcpp.cpp @UCOMMON_LIBS@
