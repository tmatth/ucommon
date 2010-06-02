# Copyright (C) 2009 David Sugar, Tycho Softworks.
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

noinst_HEADERS = local.h

lib_LTLIBRARIES = libuccaudio.la

libuccaudio_la_LDFLAGS = @UCOMMON_LIBS@ @MATH_LIB@ @UCOMMON_CLINK@ $(RELEASE)
libuccaudio_la_SOURCES = common.cpp g711.cpp buffer.cpp codecs.cpp \
	files.cpp reader.cpp writer.cpp

