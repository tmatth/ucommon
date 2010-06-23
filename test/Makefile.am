# Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS) $(CHECKFLAGS)
LDADD = ../common/libucommon.la @UCOMMON_LIBS@ @UCOMMON_CLINK@
EXTRA_DIST = *.cpp keydata.conf testscript.scr mergescript.scr

TESTS = ucommonLinked ucommonSocket ucommonStrings ucommonThreads \
	ucommonKeydata ucommonStream ucommonUnicode ucommonQueue \
	ucommonDatetime

if TEST_SCRIPT
TESTS += ucommonScript
endif

if TEST_AUDIO
TESTS += ucommonAudio
endif

if TEST_SECURE
TESTS += ucommonDigest ucommonCipher

noinst_PROGRAMS = demoSSL
demoSSL_SOURCES = ssl.cpp
demoSSL_LDFLAGS = @SECURE_LOCAL@

endif

check_PROGRAMS = $(TESTS)

testing:  $(TESTS)

ucommonThreads_SOURCES = thread.cpp
ucommonStrings_SOURCES = string.cpp
ucommonLinked_SOURCES = linked.cpp
ucommonSocket_SOURCES = socket.cpp
ucommonStream_SOURCES = stream.cpp
ucommonKeydata_SOURCES = keydata.cpp
ucommonUnicode_SOURCES = unicode.cpp
ucommonDatetime_SOURCES = datetime.cpp
ucommonQueue_SOURCES = queue.cpp

if TEST_SECURE
ucommonDigest_SOURCES = digest.cpp
ucommonDigest_LDFLAGS = @SECURE_LOCAL@

ucommonCipher_SOURCES = cipher.cpp
ucommonCipher_LDFLAGS = @SECURE_LOCAL@

endif

if TEST_SCRIPT
ucommonScript_SOURCES = script.cpp
ucommonScript_LDFLAGS = ../script/libuccscript.la
endif

if TEST_AUDIO
ucommonAudio_SOURCES = audio.cpp
ucommonAudio_LDFLAGS = ../audio/libuccaudio.la @MATH_LIB@
endif

# test using full stdc++ linkage...
stdcpp:	stdcpp.cpp
	$(CXX) @UCOMMON_FLAGS@ -o stdcpp -I../inc -L../src/.libs -lucommon stdcpp.cpp @UCOMMON_LIBS@
