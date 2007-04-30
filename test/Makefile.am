MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS) $(UCOMMON_CLINK)
CXXLINK = $(LIBTOOL) --tag=CC --mode=link $(CXX) $(AM_CXXFLAGS) $(CFLAGS) \
    $(AM_LDFLAGS) $(LDFLAGS) -o $@
LDADD = ../src/libucommon.la @UCOMMON_LIBS@
EXTRA_DIST = stdcpp.cpp

noinst_PROGRAMS = string thread linked

string_SOURCES = string.cpp
thread_SOURCES = thread.cpp
linked_SOURCES = linked.cpp

# test using full stdc++ linkage...
stdcpp:	stdcpp.cpp
	$(CXX) @UCOMMON_FLAGS@ -o stdcpp -I../inc -L../src/.libs -lucommon stdcpp.cpp @UCOMMON_LIBS@
