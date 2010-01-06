# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if ((WIN32 AND CMAKE_COMPILER_IS_GNUCXX) OR MINGW OR MSYS)
	set (UCOMMON_LIBS --enable-stdcall-fixup ${UCOMMON_LIBS} mingwex mingw32)
endif()

if (WIN32 OR MINGW OR MSYS)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} ws2_32 wsock32 kernel32)
endif()

# if (MSVC60)
#	set (UCOMMON_FLAGS ${UCOMMON_FLAGS} -DWINVER=0x0400)
# endif()

find_package(Threads)
if (CMAKE_HAVE_PTHREAD_H)
	set(HAVE_PTHREAD_H TRUE)
endif()
set (UCOMMON_LIBS ${UCOMMON_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${WITH_LDFLAGS}) 

if(UNIX OR MSYS OR MINGW OR CYGWIN)
	check_library_exists(dl dlopen "" HAVE_DL_LIB)
	if (HAVE_DL_LIB)
		set (UCOMMON_LIBS ${UCOMMON_LIBS} dl)
	else()
		check_library_exists(compat dlopen "" HAVE_COMPAT_LIB)
		if(HAVE_COMPAT_LIB)
			set (UCOMMON_LIBS ${UCOMMON_LIBS} compat)
		endif()
	endif()

	check_library_exists(dld shl_load "" HAVE DLD_LIB)
	if (HAVE_DLD_LIB)
		set (UCOMMON_LIBS ${UCOMMON_LIBS} dld)
	endif()

	check_library_exists(socket socket "" HAVE_SOCKET_LIB)
	if (HAVE_SOCKET_LIB)
		set (UCOMMON_LIBS ${UCOMMON_LIBS} socket)
	endif()

	check_library_exists(posix4 sem_wait "" HAVE_POSIX4_LIB)
	if (HAVE_POSIX4_LIB)
		set(UCOMMON_LIBS ${UCOMMON_LIBS} posix4)
	endif()

	check_library_exists(rt clock_gettime "" HAVE_RT_LIB)
	if (HAVE_RT_LIB)
		set(UCOMMON_LIBS ${UCOMMON_LIBS} rt)
	endif()

	if(CMAKE_COMPILER_IS_GNUCXX)
		if(ENABLE_DEBUG)
			add_definitions(-g)
		endif()
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -Wno-long-long -fvisibility-inlines-hidden -fvisibility=hidden)
	endif()
endif()

if(WIN32)
	if(ENABLE_DEBUG)
		add_definitions(-DDEBUG)
	else(ENABLE_DEBUG)
		add_definitions(-DNDEBUG)
	endif()
endif()

# some platforms we can only build non-c++ stdlib versions without issues...

if(MSVC60)
	set(DISABLE_STDLIB TRUE)
endif()

if((MSYS OR MINGW OR WIN32) AND CMAKE_COMPILER_IS_GNUCXX)
	set(DISABLE_STDLIB TRUE)
endif()

# see if we are building with or without std c++ libraries...
if (NOT ENABLE_STDLIB OR DISABLE_STDLIB)
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -fno-exceptions -fno-rtti -fno-enforce-eh-specs)
		if(MINGW OR MSYS)
			set(UCOMMON_LIBS ${UCOMMON_LIBS} -nodefaultlibs -nostdinc++ msvcrt)
		else()
			set(UCOMMON_LIBS ${UCOMMON_LIBS} -nodefaultlibs -nostdinc++ c)
		endif()
		check_library_exists(gcc __modsi3 "" HAVE_GCC_LIB)
		if(HAVE_GCC_LIB)
			set(UCOMMON_LIBS ${UCOMMON_LIBS} gcc)
		endif()
	endif()	
else()
	# for now assume newer c++ stdlib always....
	set(UCOMMON_FLAGS ${UCOMMON_FLAGS} -DNEW_STDLIB)
endif()

