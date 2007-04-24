MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS)
CXXLINK = $(LIBTOOL) --tag=CC --mode=link $(CXX) $(AM_CXXFLAGS) $(CFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
LDADD = ../src/libucommon.la @UCOMMON_LIBS@

bin_PROGRAMS = ushmdump

if POSIX
bin_PROGRAMS += udetach uattach ucontrol 
endif

udetach_SOURCES = udetach.cpp
uattach_SOURCES = uattach.cpp
ucontrol_SOURCES = ucontrol.cpp
ushmdump_SOURCES = ushmdump.cpp


