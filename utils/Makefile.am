MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS) $(UCOMMON_CLINK)
CXXLINK = $(LIBTOOL) --tag=CC --mode=link $(CXX) $(AM_CXXFLAGS) $(CFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
LDADD = ../src/libucommon.la @UCOMMON_LIBS@

bin_PROGRAMS = ushmdump

if POSIX
bin_PROGRAMS += ucontrol
endif

ucontrol_SOURCES = ucontrol.cpp
ushmdump_SOURCES = ushmdump.cpp


