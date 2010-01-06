# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# This is a simplified build system for GNU uCommon.  In particular, it
# offers limited cmodel linkage support, at least for mingw32 support (and
# probably visual c 6 also).  If one really needs to do such things outside
# these use cases, it is suggested to continue using the existing autotools 
# configure script driven build environment instead.  Mostly the cmake one is 
# meant for generating project files for those working with IDE's to do 
# otherwise generic builds of the library and supporting applications.

cmake_minimum_required(VERSION 2.6)
PROJECT(ucommon)
set (VERSION 2.0.7)
set (SOVERSION 2)

MESSAGE( STATUS "Configuring GNU ucommon ${VERSION}...")
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)

# set to true for debug and trace during CMakeLists development
set(CMAKE_VERBOSE_MAKEFILE FALSE)
 
# add module path of project if it exists...
if (EXISTS "${CMAKE_SOURCE_DIR}/inc/")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/inc/")
endif()

include (inc/ucommon.cmake)
set (CMAKE_REQUIRED_LIBRARIES ${UCOMMON_LIBS})
check_function_exists(getaddrinfo HAVE_GETADDRINFO)
check_function_exists(socketpair HAVE_SOCKETPAIR)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_function_exists(gethostbyname2 HAVE_GETHOSTBYNAME2)
check_function_exists(stricmp HAVE_STRICMP)
check_function_exists(stristr HAVE_STRISTR)
check_function_exists(sysconf HAVE_SYSCONF)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(dlopen HAVE_DLOPEN)
check_function_exists(shl_open HAVE_SHL_OPEN)
check_function_exists(pthread_condattr_setclock HAVE_PTHREAD_CONDATTR_SETCLOCK)
check_function_exists(pthread_setconcurrency HAVE_PTHREAD_SETCONCURRENCY)
check_function_exists(pthread_yield HAVE_PTHREAD_YIELD)
check_function_exists(pthread_yield_np HAVE_PTHREAD_YIELD_NP)
check_function_exists(pthread_delay HAVE_PTHREAD_DELAY)
check_function_exists(pthread_delay_np HAVE_PTHREAD_DELAY_NP)
check_function_exists(pthread_setschedprio HAVE_PTHREAD_SETSCHEDPRIO)
check_function_exists(ftok HAVE_FTOK)
check_function_exists(shm_open HAVE_SHM_OPEN)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(nanosleep HAVE_NANOSLEEP)
check_function_exists(clock_nanosleep HAVE_CLOCK_NANOSLEEP)
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
check_function_exists(posix_fadvise HAVE_POSIX_FADVISE)
check_function_exists(posixtime HAVE_POSIXTIME)

check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(poll.h HAVE_POLL_H)
check_include_files(sys/mman.h HAVE_SYS_MMAN_H)
check_include_files(sys/shm.h HAVE_SYS_SHM_H)
check_include_files(sys/poll.h HAVE_SYS_POLL_H)
check_include_files(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(endian.h HAVE_ENDIAN_H)
check_include_files(sys/filio.h HAVE_SYS_FILIO_H)
check_include_files(dirent.h HAVE_DIRENT_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(mach/clock.h HAVE_MACH_CLOCK_H)
check_include_files(mach-o/dyld.h HAVE_MACH_O_DYLD_H)
check_include_files(linux/version.h HAVE_LINUX_VERSION_H)

# for some reason, normal library searches always fail on broken windows
if (WIN32 AND NOT UNIX AND NOT MINGW AND NOT MSYS)
	set(HAVE_GETADDRINFO True)
	set(HAVE_INET_NTOP True)
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc)
add_definitions(${UCOMMON_FLAGS} ${WITH_FLAGS})

add_library(ucommon SHARED 
	src/object.cpp src/linked.cpp src/string.cpp src/mapped.cpp src/shell.cpp 
	src/counter.cpp src/bitmap.cpp src/timer.cpp src/memory.cpp 
	src/socket.cpp src/access.cpp src/thread.cpp src/fsys.cpp src/cpr.cpp 
	src/vector.cpp src/xml.cpp src/stream.cpp src/persist.cpp src/keydata.cpp)
set_target_properties(ucommon PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(ucommon ${UCOMMON_LIBS} ${WITH_LIBS})
install(TARGETS ucommon DESTINATION lib)
