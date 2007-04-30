MAINTAINERCLEANFILES = Makefile.in Makefile
RELEASE = $(LT_VERSION) -release $(LT_RELEASE)
AM_CXXFLAGS = -I$(top_srcdir)/inc $(UCOMMON_FLAGS) $(UCOMMON_CLINK)
CXXLINK = $(LIBTOOL) --tag=CC --mode=link $(CXX) $(AM_CFLAGS) $(CFLAGS) \
    $(AM_LDFLAGS) $(LDFLAGS) -o $@

lib_LTLIBRARIES = libucommon.la 

# libucommon_la_CXXFLAGS = $(UCOMMON_FLAGS)
libucommon_la_LDFLAGS = @UCOMMON_LIBS@ $(RELEASE) @UCOMMON_SHARED@
libucommon_la_SOURCES = object.cpp linked.cpp string.cpp new.cpp \
	file.cpp counter.cpp bitmap.cpp timer.cpp memory.cpp socket.cpp \
	access.cpp thread.cpp process.cpp vector.cpp misc.cpp config.cpp


