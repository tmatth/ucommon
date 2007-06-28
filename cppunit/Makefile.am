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

TESTS = ucommonTest 
check_PROGRAMS = $(TESTS)

ucommonTest_SOURCES = unit.cpp 
ucommonTest_CXXFLAGS = $(CPPUNIT_FLAGS) @UCOMMON_FLAGS@
ucommonTest_LDFLAGS = $(CPPUNIT_LIBS) $(LDADD)
