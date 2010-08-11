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
set (VERSION 3.2.1)
set (SOVERSION 3)

# project options
if (WIN32)
	option(BUILD_STATIC "Set to OFF to build shared libraries" ON)
else()
	option(BUILD_STATIC "Set to ON to build static libraries" OFF)
endif()

set(STDLIB TRUE)
if(MSVC60)
	set(STDLIB FALSE)
endif()

if((MSYS OR MINGW OR WIN32) AND CMAKE_COMPILER_IS_GNUCXX)
	set(STDLIB FALSE)
endif()

option(BUILD_STDLIB "Set to ON to build with C++ stdlib" ${STDLIB})

option(BUILD_TESTS "Set to ON to build test programs" OFF)

MESSAGE( STATUS "Configuring GNU ${PROJECT_NAME} ${VERSION}...")
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} library")
SET(CPACK_PACKAGE_VENDOR              "David Sugar")
SET(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(PACKAGE_FILE_NAME                 ${PROJECT_NAME})
set(PACKAGE_FILE_VERSION              ${VERSION})

IF (WIN32)
	SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-${PACKAGE_FILE_VERSION}")
ELSE (WIN32)
	SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-bin-${PACKAGE_FILE_VERSION}")
	if (NOT CPACK_GENERATOR)
		SET(CPACK_GENERATOR "TBZ2")
	endif()
ENDIF (WIN32)

IF(WIN32 AND NOT UNIX)
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_FILE_NAME}")

    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    # SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
    # SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ${PROJECT_NAME}$")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.gnutelephony.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.gnutelephony.org")
    SET(CPACK_NSIS_CONTACT "dyfet@gnutelephony.org")
    SET(CPACK_NSIS_MODIFY_PATH ON)

ENDIF(WIN32 AND NOT UNIX)

set(CPACK_COMPONENTS_ALL libraries headers)
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME   "C++ Headers")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION  "Dynamic library")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
	"Header files needed to build applications using ucommon library")
SET(CPACK_SOURCE_IGNORE_FILES ".bzr" "build" ".swp$" ".*~" ".svn" ".git")
SET(CPACK_SOURCE_GENERATOR "TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-${PACKAGE_FILE_VERSION}")

 # This must always be last!
INCLUDE(CPack)

# set to true for debug and trace during CMakeLists development
set(CMAKE_VERBOSE_MAKEFILE FALSE)
 
# add module path of project if it exists...
if (EXISTS "${CMAKE_SOURCE_DIR}/inc/")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/inc/")
endif()

include (inc/ucommon.cmake)

find_package(Threads)
if (CMAKE_HAVE_PTHREAD_H)
	set(HAVE_PTHREAD_H TRUE)
endif()
set (UCOMMON_LIBS ${UCOMMON_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${WITH_LDFLAGS}) 
set (MATH_LIB "")

if ((WIN32 AND CMAKE_COMPILER_IS_GNUCXX) OR MINGW OR MSYS)
	set (UCOMMON_LIBS --enable-stdcall-fixup ${UCOMMON_LIBS} mingwex mingw32)
endif()

if (WIN32 OR MINGW OR MSYS)
	set (UCOMMON_LIBS ${UCOMMON_LIBS} advapi32 user32 ws2_32 wsock32 kernel32)
endif()

if(WITH_NOSTDLIB AND UNIX)
	set(UCOMMON_LIBS c)
endif()

if(WITH_NOSTDLIB AND CMAKE_COMPILER_IS_GNUCXX)
	check_library_exists(gcc __modsi3 "" HAVE_GCC_LIB)
	if(HAVE_GCC_LIB)
			set(UCOMMON_LIBS ${UCOMMON_LIBS} gcc)
	endif()
endif()

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
endif()

check_library_exists(m sqrt "" HAVE_MATHLIB)
if (HAVE_MATHLIB)
	set(MATH_LIB m)
endif()

# this may have to change for mswindows...
if (WITH_PLUGINDIR)
	set(UCOMMON_PLUGINS ${WITH_PLUGINDIR})
else()
	set(UCOMMON_PLUGINS ${CMAKE_INSTALL_PREFIX}/lib/ucommon)
endif()

if (WITH_LOCALEDIR)
	set(UCOMMON_LOCALE ${WITH_LOCALEDIR})
else()
	set(UCOMMON_LOCALE ${CMAKE_INSTALL_PREFIX}/share/locale)
endif()

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
check_function_exists(ftruncate HAVE_FTRUNCATE)
check_function_exists(pwrite HAVE_PWRITE)
check_function_exists(setpgrp HAVE_SETPGRP)
check_function_exists(setlocale HAVE_SETLOCALE)
check_function_exists(gettext HAVE_GETTEXT)
check_function_exists(execvp HAVE_EXECVP)

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
check_include_files(wchar.h HAVE_WCHAR_H)
check_include_files(mach/clock.h HAVE_MACH_CLOCK_H)
check_include_files(mach-o/dyld.h HAVE_MACH_O_DYLD_H)
check_include_files(linux/version.h HAVE_LINUX_VERSION_H)
check_include_files(regex.h HAVE_REGEX_H)
check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
check_include_files(sys/event.h HAVE_SYS_EVENT_H)
check_include_files(syslog.h HAVE_SYSLOG_H)
check_include_files(openssl/ssl.h HAVE_OPENSSL)
check_include_files(libintl.h HAVE_LIBINTL_H)

# for some reason, normal library searches always fail on broken windows
if (WIN32 AND NOT UNIX AND NOT MINGW AND NOT MSYS)
	set(HAVE_GETADDRINFO True)
	set(HAVE_INET_NTOP True)
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
if(NOT WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc)
endif(NOT WIN32)

# common build options can be passed to cmake using WITH_CFLAGS, WITH_LIBS,
# and WITH_INCLUDES.
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc ${WITH_INCLUDES})
add_definitions(${UCOMMON_FLAGS} ${WITH_CFLAGS})
link_libraries(${WITH_LIBS})

# by default we build static libs for windows, shared libs for unix.  
# we may also set this from a top level cmake or -DWITH_XX_LIBS
if(BUILD_STATIC)
	set(BUILD_LIBRARY_TYPE STATIC)
else()
	set(BUILD_LIBRARY_TYPE SHARED)
endif()

if(LIB_SUFFIX)
	if (NOT LIB_SUFFIX EQUAL 64)
		message (FATAL_ERROR "LIB_SUFFIX must be empty (32 bits) or 64 (64 bits)")
	endif()
endif()

file(GLOB audio_src audio/*.cpp)
file(GLOB common_src common/*.cpp)
file(GLOB script_src script/*.cpp)
file(GLOB ucommon_inc inc/ucommon/*.h)

if(HAVE_OPENSSL)
	file(GLOB secure_src openssl/*.cpp)
	set(SECURE_LIBS ssl crypto)
else()
	file(GLOB secure_src insecure/*.cpp)
endif()

add_library(ucommon ${BUILD_LIBRARY_TYPE} ${common_src})
set_target_properties(ucommon PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(ucommon ${UCOMMON_LIBS} ${WITH_LIBS})

add_library(uccscript ${BUILD_LIBRARY_TYPE} ${script_src})
set_target_properties(uccscript PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(uccscript ${UCOMMON_LIBS} ${WITH_LIBS})
add_dependencies(uccscript ucommon)

add_library(uccaudio ${BUILD_LIBRARY_TYPE} ${audio_src})
set_target_properties(uccaudio PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(uccaudio ucommon ${UCOMMON_LIBS} ${WITH_LIBS} ${MATH_LIB})
add_dependencies(uccaudio ucommon)

add_library(uccsecure ${BUILD_LIBRARY_TYPE} ${secure_src})
set_target_properties(uccsecure PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(uccsecure ucommon ${UCOMMON_LIBS} ${WITH_LIBS} ${SECURE_LIBS})
add_dependencies(uccsecure ucommon)

add_executable(audiotool utils/audiotool.cpp)
add_dependencies(audiotool uccaudio ucommon)
target_link_libraries(audiotool uccaudio ucommon ${UCOMMON_LIBS} ${WITH_LIBS} ${MATH_LIB})

add_executable(args utils/args.cpp)
add_dependencies(args ucommon)
target_link_libraries(args ucommon ${UCOMMON_LIBS} ${WITH_LIBS})

message (STATUS)
message (STATUS "-------------------------------------------------------------------------------")
message (STATUS "CMAKE_INSTALL_PREFIX " ${CMAKE_INSTALL_PREFIX} )
message (STATUS "LIB_SUFFIX           " ${LIB_SUFFIX})
message (STATUS "BUILD_STATIC         " ${BUILD_STATIC})
message (STATUS "BUILD STDLIB         " ${BUILD_STDLIB})
message (STATUS "BUILD_TESTS          " ${BUILD_TESTS})
message (STATUS "-------------------------------------------------------------------------------")
message (STATUS)
message (STATUS "Change a value with: cmake -D<Variable>=<Value>")
message (STATUS)

if (BUILD_TESTS)
	add_subdirectory(test)
endif()

if(NOT WIN32)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
endif(NOT WIN32)

install(FILES   ${ucommon_inc}  DESTINATION include/ucommon  COMPONENT headers)
install(TARGETS ucommon uccscript uccaudio DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)
install(TARGETS audiotool DESTINATION bin)
install(TARGETS args DESTINATION bin)

