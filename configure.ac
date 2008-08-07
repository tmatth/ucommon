# Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT(autogen.sh)

VERSION="1.9.7"
LT_VERSION="2:1:0"

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXXCPP
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(ucommon, [$VERSION])
AM_CONFIG_HEADER(config.h)

LT_MAJOR="`echo $LT_VERSION | sed s/:.*$//`"
AC_SUBST(LT_VERSION)
AC_SUBST(LT_MAJOR)

UCOMMON_VISIBILITY="0"
UCOMMON_FLAGS=""
UCOMMON_MODEL="CXX"
UCOMMON_CLINK=""
UCOMMON_LIBC="-lc"
UCOMMON_LIBS=""

CHECKFLAGS="$CHECKFLAGS"
POSIX=false
MSSYS=false

socket_lib=""
posix4_lib=""
rt_lib=""

AC_DEFUN([AC_SUBST_DIR], [
        ifelse($2,,,$1="[$]$2")
        result="***"
        prior="A"
        while test "$prior" != "$result" ; do
                prior=`(echo "[$]$1")`
                $1=`(
                test "x$prefix" = xNONE && prefix="$ac_default_prefix"
                test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
                eval echo \""[$]$1"\"
                )`
                result=`(echo "[$]$1")`
        done
        AC_SUBST($1)
])

AC_MSG_CHECKING([full c++ support and linking model])

# for very old gcc and for minix we disable full ucommon build by default
# we also disable for mingw32 or when no "shared" library support because 
# libstdc++ is not dll and really bloats in linkage of plugins.  Any case, 
# you can always override with --enable-stdcpp...

if test -z "$enable_stdcpp" ; then
	if test "x$GCC" = "xyes" ; then
		gccver=`$CC -dumpversion | cut -f1 -d.`
	fi
	if test -z "$gccver" ; then
		gccver="3"
	fi
	if test "$gccver" -lt "3" ; then
		enable_stdcpp="no"
	fi
	if test "x$enable_shared" = "xno" ; then
		enable_stdcpp="no"
	fi
	case "$target_os" in
	*minix*|mingw*)
		enable_stdcpp="no"
		;;
	esac
fi
		
AC_ARG_ENABLE(stdcpp, 
	AC_HELP_STRING([--disable-stdcpp],[compile without stdc++ runtime overhead]))

if test "x$enable_stdcpp" = "xno" ; then
	UCOMMON_MODEL="CC"
	LIBTOOL='$(SHELL) $(top_srcdir)/cmodel.sh'" ${LIBTOOL}"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi

AC_ARG_WITH(libc, 
	AC_HELP_STRING([--with-libc=path],[specify path to alternate libc]),[
	if test x$with_libc != x ; then
		clib=`echo ${with_libc} | sed s/[-]l//`
        UCOMMON_LIBC=-l${clib} ; fi
])

AC_ARG_ENABLE(debug, 
	AC_HELP_STRING([--enable-debug],[compile for debugging]))
if test -z "$enable_debug" ; then
	enable_debug="no"
elif test $enable_debug = "yes" ; then
	CXXFLAGS="${CXXFLAGS} -g -DDEBUG"
fi

AH_BOTTOM([
#include <ucommon/export.h>
])

clib=`echo ${UCOMMON_LIBC} | sed s/[-]l//`
tlib=""

AC_CHECK_HEADERS(stdint.h poll.h sys/mman.h sys/shm.h sys/poll.h sys/timeb.h endian.h sys/filio.h dirent.h sys/resource.h)
AC_CHECK_HEADERS(mach/clock.h mach-o/dyld.h)

AC_CHECK_LIB(msvcrt, fopen, [
	clib="msvcrt"
	UCOMMON_VISIBILITY="0"
	UCOMMON_LIBS="--enable-stdcall-fixup $UCOMMON_LIBS -lmingwex -lmingw32 -lws2_32 -lwsock32 -lkernel32"
	UCOMMON_LIBC="-lmsvcrt -lgcc"
	AC_DEFINE(HAVE_INET_NTOP, [1], [have inet_ntop])
	AC_DEFINE(HAVE_GETADDRINFO, [1], [have getaddrinfo])
],[
	AC_CHECK_LIB(gcc, __modsi3, [
		UCOMMON_LIBC="$UCOMMON_LIBC -lgcc"
	])
	AC_CHECK_LIB($clib, socketpair, [
		AC_DEFINE(HAVE_SOCKETPAIR, [1], [have socketpair])
	],[
			AC_CHECK_LIB(socket, socketpair, [AC_DEFINE(HAVE_SOCKETPAIR, [1], [have socketpair])])
	])
	AC_CHECK_LIB($clib, getaddrinfo, [
		AC_DEFINE(HAVE_GETADDRINFO, [1], [have getaddrinfo])
	],[
			AC_CHECK_LIB(socket, getaddrinfo, [AC_DEFINE(HAVE_GETADDRINFO, [1], [have getaddrinfo])])
	])
	AC_CHECK_LIB($clib, gethostbyname2, [
		AC_DEFINE(HAVE_GETHOSTBYNAME2, [1], [have gethostbyname2])
	],[
			AC_CHECK_LIB(socket, gethostbyname2, [AC_DEFINE(HAVE_GETHOSTBYNAME2, [1], [have gethostbyname2])])
	])



	AC_CHECK_LIB($clib, inet_ntop, [
		AC_DEFINE(HAVE_INET_NTOP, [1], [have inet ntop])
	],[
			AC_CHECK_LIB(socket, inet_ntop, [AC_DEFINE(HAVE_INET_NTOP, [1], [have inet ntop])])
	])

	AC_CHECK_LIB(socket,socket,[
		AC_CHECK_LIB(socket, socketpair,[
			AC_DEFINE(HAVE_SOCKETPAIR, [1], [have socketpair in libsocket])
		])
		socket_lib="-lsocket"
	])
	AC_CHECK_LIB(nsl,inet_ntop,[
		AC_DEFINE(HAVE_INET_NTOP, [1], [have inet_ntop])
		socket_lib="$socket_lib -lnsl"
	])
])

AC_CHECK_LIB($clib, stricmp, [
	AC_DEFINE(HAVE_STRICMP, [1], [string icmp])
])

AC_CHECK_LIB($clib, stristr, [
    AC_DEFINE(HAVE_STRISTR, [1], [string istr])
])

AC_CHECK_LIB($clib, sysconf, [
	AC_DEFINE(HAVE_SYSCONF, [1], [system config])
])

AC_CHECK_LIB($clib, posix_memalign, [
	AC_DEFINE(HAVE_POSIX_MEMALIGN, [1], [posix memory alignment])
])

AC_CHECK_LIB($clib, dl,,[
	AC_CHECK_LIB(dl, dlopen, [UCOMMON_LIBS="$UCOMMON_LIBS -ldl"],[
		AC_CHECK_LIB(compat, dlopen, [UCOMMON_LIBS="$UCOMMON_LIBS -lcompat"])
		AC_CHECK_LIB(dld, shl_load,[
			AC_DEFINE(HAVE_SHL_LOAD, [1], [alternate dyn loader])
			UCOMMON_LIBS="$UCOMMON_LIBS -ldld"
		])
	])
])


PTHREAD_FLAGS=""

if test "$enable_shared" = "no" ; then
	UCOMMON_FLAGS="$UCOMMON_FLAGS -DUCOMMON_STATIC" ; fi

AC_ARG_ENABLE(pth, [
	AC_HELP_STRING([--enable-pth],[always use GNU pth for threading])
])

if test "$enable_pth" != "yes" ; then
	AC_CHECK_HEADER(pthread.h, [
		PTHREAD_FLAGS="-mt -pthread -pthreads"
		AC_CHECK_LIB(pthread,pthread_create,[
			tlib="pthread"
			UCOMMON_LIBS="$UCOMMON_LIBS -lpthread"
		])
		if test -z "$tlib" ; then
			AC_CHECK_LIB($clib, pthread_create, [tlib="$clib"])
		fi
		if test -z "$tlib" ; then
			AC_CHECK_LIB(c_r, pthread_create, [
				tlib="c_r"
				if test "$clib" = "c" ; then
					AC_CHECK_LIB(c_r, malloc, [clib="c_r"])
				fi
			])
		fi
		if test ! -z "$tlib" ; then
			AC_CHECK_LIB($tlib,pthread_condattr_setclock,[
				AC_DEFINE(HAVE_PTHREAD_CONDATTR_SETCLOCK, [1], ["pthread clocking"])
			])
			AC_CHECK_LIB($tlib,pthread_setconcurrency,[
				AC_DEFINE(HAVE_PTHREAD_SETCONCURRENCY, [1], ["pthread concurrency"])
			])

			AC_CHECK_LIB($tlib,pthread_yield,[
				AC_DEFINE(HAVE_PTHREAD_YIELD, [1], ["pthread yield"],[
					AC_CHECK_LIB($tlib,pthread_yield_np,[
						AC_DEFINE(HAVE_PTHREAD_YIELD_NP, [1], ["pthread yield np"])
					])
				])
			])

			AC_CHECK_LIB($tlib,pthread_delay,[
				AC_DEFINE(HAVE_PTHREAD_DELAY, [1], ["pthread delay"])
			])
			AC_CHECK_LIB($tlib,pthread_delay_np,[
				AC_DEFINE(HAVE_PTHREAD_DELAY_NP, [1], ["pthread delay np"])
			])
			AC_CHECK_LIB($tlib,pthread_setschedprio,[
				AC_DEFINE(HAVE_PTHREAD_SETSCHEDPRIO, [1], ["pthread scheduling"])
			])
		fi		
	],[
		AC_CHECK_HEADER(windows.h,, [
			AC_CHECK_HEADER(pth.h,[
				UCOMMON_LIBS="$UCOMMON_LIBS -lpth"
				UCOMMON_FLAGS="$UCOMMON_FLAGS -D__PTH__"
			],[
				AC_MSG_ERROR("pthread or GNU pth support required")
			])
		])
	])
else
	AC_CHECK_HEADER(pth.h,[
		UCOMMON_LIBS="$UCOMMON_LIBS -lpth"
		UCOMMON_FLAGS="$UCOMMON_FLAGS -D__PTH__"
	],[
		AC_MSG_ERROR("GNU pth required")
	])
fi

for func in ftok shm_open nanosleep clock_nanosleep clock_gettime localtime_r ; do
	found="no"
	AC_CHECK_FUNC($func,[
		found=$func
	],[
		AC_CHECK_LIB(posix4, $func, [
			found=$func
			posix4_lib="-lposix4"
		],[
			AC_CHECK_LIB(rt, $func, [
				found=$func
				rt_lib="-lrt"
			],[
				if test ! -z "$tlib" ; then
					AC_CHECK_LIB($tlib, $func, [
						found=$func
					])
				fi
			])
		])
	])
	case $found in
	shm_open)
		AC_DEFINE(HAVE_SHM_OPEN, [1], [shared memory open])
		;;
	ftok)
		AC_DEFINE(HAVE_FTOK, [1], [has ftok])
		;;
	localtime_r)
		AC_DEFINE(HAVE_LOCALTIME_R, [1], [has localtime_r])
		;;
	nanosleep)
		AC_DEFINE(HAVE_NANOSLEEP, [1], [has nanosleep])
		;;
	clock_nanosleep)
		AC_DEFINE(HAVE_CLOCK_NANOSLEEP, [1], [has clocked nanosleep])
		;;
	clock_gettime)		
		AC_DEFINE(HAVE_POSIXTIME, [1], [has posix clock functions])
		;;
	esac
done

UCOMMON_LINKED="$socket_lib $posix4_lib $rt_lib $UCOMMON_LIBS"
UCOMMON_LIBS="$socket_lib $posix4_lib $rt_lib $UCOMMON_LIBS $UCOMMON_LIBC"

if test "$UCOMMON_MODEL" = "CC" ; then
	AC_CHECK_LIB(Systemstubs, printf, [
		UCOMMON_CLINK="$UCOMMON_CLINK -lSystemstubs"
		UCOMMON_LIBC="$UCOMMON_LIBC -lgcc"
	])
	if test "$GCC" != "yes" ; then
		AC_CHECK_LIB(Crun, _ex_register, [
			UCOMMON_LIBC="-L/opt/SUNWspro/lib -lCrun $UCOMMON_LIBC"
		])
	fi
fi

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
COMPILER_FLAGS=""
for flag in $CXXFLAGS ; do
	case $flag in
	-f*exceptions|-f*rtti|-f*check-new|-f*enforce-eh-specs|-finline|-f*except)
		if test "$UCOMMON_MODEL" = "CXX" ; then
			UCOMMON_FLAGS="$UCOMMON_FLAGS $flag" ; fi
		;;
	-fvisibility=*)
		VISIBLE_FLAGS=""
		COMPILER_FLAGS="$COMPILER_FLAGS $flag"
		;;
	*)
		COMPILER_FLAGS="$COMPILER_FLAGS $flag"
		;;
	esac
done
export CXXFLAGS="$COMPILER_FLAGS"

if test "$UCOMMON_MODEL" = "CC" ; then	
	UCOMMON_FLAGS="$UCOMMON_FLAGS"

	if test "$GCC" = yes ; then
		ucflags="-fno-exceptions -fno-rtti -fno-enforce-eh-specs"
		lcflags="-nodefaultlibs -nostdinc++"
	else
		ucflags="-features=no%except -features=namespace -features=no%rtti"
		lcflags="-xnolib"
	fi

	for flag in $ucflags ; do
		AC_MSG_CHECKING([whether ${CXX} supports $flag])
		echo 'void f(){}' >conftest.cpp
		if test -z "`${CXX} $flag -c conftest.cpp 2>&1`"; then
			UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
			AC_MSG_RESULT(yes)
		else
			AC_MSG_RESULT(no)
		fi
		rm -f conftest*
	done

	for flag in $lcflags ; do
		AC_MSG_CHECKING([whether ${CXX} supports $flag])
		echo 'void f(){}' >conftest.cpp
		if test -z "`${CXX} $flag -c conftest.cpp 2>&1`"; then
			UCOMMON_CLINK="$UCOMMON_CLINK $flag"
			AC_MSG_RESULT(yes)
		else
			AC_MSG_RESULT(no)
		fi
		rm -f conftest*
	done
else
	AC_LANG_SAVE
	AC_LANG_CPLUSPLUS
	AC_CHECK_HEADERS(stdexcept)
	AC_MSG_CHECKING(for old style iostream)
    AC_TRY_COMPILE([
		#include <iostream>
		using namespace std;

		class mystr : public streambuf, public iostream
		{
			mystr();
		};

		mystr::mystr() : streambuf(), iostream((streambuf *)this)
		{
		}
    ],[return 0;],[
		AC_MSG_RESULT(no)
		UCOMMON_FLAGS="$UCOMMON_FLAGS -DNEW_STDCPP"
	],[
		AC_MSG_RESULT(yes)
		UCOMMON_FLAGS="$UCOMMON_FLAGS -DOLD_STDCPP"
	])
	AC_LANG_RESTORE
fi

if test ! -z "$PTHREAD_FLAGS" ; then
	for flag in $PTHREAD_FLAGS ; do
	    AC_MSG_CHECKING([whether ${CXX} supports $flag])
    	echo 'void f(){}' >conftest.cpp
    	if test -z "`${CXX} $flag -c conftest.cpp 2>&1`"; then
			AC_MSG_RESULT(yes)
			UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
			break
		else
			AC_MSG_RESULT(no)
		fi
	done
fi

export CXXFLAGS

# libstdc++ on some targets broken badly if inlines hidden.  Might be
# deeper issue with ansi c++ library model...
flags="-fno-check-new -finline"
if test "$UCOMMON_MODEL" = "CC" ; then
	if test "$enable_shared" != "no" ; then
		flags="flags -fvisibility-inlines-hidden"
	fi
fi

for flag in $flags ; do
    AC_MSG_CHECKING([whether ${CXX} supports $flag])
    echo 'void f(){}' >conftest.cpp
    if test -z "`${CXX} $flag -c conftest.cpp 2>&1`"; then
        UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
    fi
    rm -f conftest*
done

AM_CONDITIONAL(POSIX, test "$POSIX" = "true")

AC_MSG_CHECKING([whether ${CXX} supports visibility])
echo 'void f(){}' >conftest.cpp
if test -z "`${CXX} -fvisibility=hidden -c conftest.cpp 2>&1`"; then
	UCOMMON_FLAGS="$UCOMMON_FLAGS -fvisibility=hidden -DUCOMMON_VISIBILITY=1"
    AC_MSG_RESULT(yes)
	UCOMMON_VISIBILITY="1"
else
	UCOMMON_FLAGS="$UCOMMON_FLAGS -DUCOMMON_VISIBILITY=0"
    AC_MSG_RESULT(no)
fi
rm -f conftest*

AC_LANG_RESTORE

AC_SUBST_DIR(includes, includedir)
AC_SUBST_DIR(libs, libdir)

if test "$libs" != "/usr/lib" ; then
	UCOMMON_LINKED="-L$libs $UCOMMON_LINKED" ; fi

if test "$includes" != "/usr/include" ; then
	UCOMMON_FLAGS="$UCOMMON_FLAGS -I$includes" ; fi

case "$target_os" in
*bsd*)
	if test "$UCOMMON_MODEL" = "CC" ; then
		if test "$GCC" = "yes" -a "$UCOMMON_LIBC" = "-lc" ; then
			UCOMMON_LIBC="-Wl,-lc"
		elif test "$GCC" = "yes" -a "$UCOMMON_LIBC" = "-lc_r" ; then
			UCOMMON_LIBC="-Wl,-lc_r"
		fi
    fi
	;;
osf*)
    UCOMMON_FLAGS="$UCOMMON_FLAGS -D_POSIX_C_SOURCE=1 -D_OSF_SOURCE=1 -D__USE_STD_IOSTREAM"
    ;;
cygwin*|mingw*)
    UCOMMON_LINKED="$UCOMMON_LINKED -no-undefined"
	UCOMMON_LIBS="$UCOMMON_LIBS -no-undefined"
    ;;
esac

UCOMMON_CLINK="$UCOMMON_CLINK $UCOMMON_LIBC"

AC_PATH_PROG(DOXYGEN, doxygen, doxygen)
AC_SUBST(DOXYGEN)
AC_SUBST(CHECKFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(UCOMMON_VISIBILITY)
AC_SUBST(UCOMMON_LINKED)
AC_SUBST(UCOMMON_MODEL)
AC_SUBST(UCOMMON_FLAGS)
AC_SUBST(UCOMMON_CLINK)
AC_SUBST(UCOMMON_LIBC)
AC_SUBST(UCOMMON_LIBS)

AC_OUTPUT(Makefile src/Makefile inc/Makefile inc/ucommon/Makefile test/Makefile
ucommon.pc ucommon.spec ucommon-config)

