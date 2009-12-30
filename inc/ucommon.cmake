# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set (UCOMMON_FLAGS)

if ((WIN32 AND CMAKE_COMPILER_IS_GNUCXX) OR MINGW OR MSYS)
	set (UCOMMON_LIBS --enable-stdcall-fixup ${UCOMMON_LIBS} -lmingwex -lmingw32)
endif()

if (WIN32 OR MINGW OR MSYS)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} -lws2_32 -lwsock32 -lkernel32)
endif()

find_package(Threads)
if (CMAKE_HAVE_PTHREAD_H)
	set(HAVE_PTHREAD_H TRUE)
endif()
set (UCOMMON_LIBS ${UCOMMON_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${WITH_LDFLAGS}) 

check_library_exists(dl dlopen "" HAVE_DL_LIB)
if (HAVE_DL_LIB)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} -ldl)
else()
	check_library_exists(compat dlopen "" HAVE_COMPAT_LIB)
	if(HAVE_COMPAT_LIB)
		set (UCOMMON_LIBS ${UCOMMON_LIBS} -lcompat)
	endif()
endif()

check_library_exists(dld shl_load "" HAVE DLD_LIB)
if (HAVE_DLD_LIB)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} -ldld)
endif()

check_library_exists(socket socket "" HAVE_SOCKET_LIB)
if (HAVE_SOCKET_LIB)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} -lsocket)
endif()

check_library_exists(posix4 sem_wait "" HAVE_POSIX4_LIB)
if (HAVE_POSIX4_LIB)
	set(UCOMMON_LIBS ${UCOMMON_LIBS} -lposix4)
endif()

check_library_exists(rt clock_gettime "" HAVE_RT_LIB)
if (HAVE_RT_LIB)
	set(UCOMMON_LIBS ${UCOMMON_LIBS} -lrt)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	if(ENABLE_DEBUG)
		add_definitions(-g)
	endif()
	add_definitions(-Wno-long-long)
endif()

if(WIN32)
	if(ENABLE_DEBUG)
		add_definitions(-DDEBUG)
	else(ENABLE_DEBUG)
		add_definitions(-DNDEBUG)
	endif()
endif()

