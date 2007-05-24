AC_INIT(reconfig)

VERSION="0.4.0"
LT_RELEASE="0.4"
LT_VERSION="0:0"

# use ~/.configure to hold default options when I use no options...

cfg_options=""
if test -z "$*" ; then
	if test -f ~/.configure ; then
		cfg_options=`grep ^ucommon: ~/.configure | sed -e s/^ucommon://`
	elif test -f /etc/configure.conf ; then
		cfg_options=`grep ^ucommon: /etc/configure.conf | sed -e s/^ucommon://`
	fi
fi

if test ! -z "$cfg_options" ; then
    echo "using ~/.configure defaults...$cfg_options"
    case "$SHELL" in
    */bash*)
        exec $SHELL $0 $cfg_options
        exit -1        
		;;
    esac
    exec $0 $cfg_options
	exit -1
fi

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
LT_MINOR="`echo $LT_VERSION | sed s/.*://`"
LT_SUBVER="$LT_MAJOR.$LT_MINOR"
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_VERSION)
AC_SUBST(LT_SUBVER)
AC_SUBST(LT_MAJOR)
AC_SUBST(LT_MINOR)

UCOMMON_VISIBILITY="0"
UCOMMON_LIBC="-lc"
UCOMMON_LIBS=""

POSIX=false
MSSYS=false

AC_ARG_WITH(libc,
	[  --with-libc             specify alternate libc],[
	if test x$with_libc != x ; then
		clib=`echo ${with_libc} | sed s/[-]l//`
        UCOMMON_LIBC=-l${clib} ; fi
])

AC_ARG_ENABLE(debug, [  --enable-debug          compile for debugging])
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

AC_CHECK_HEADERS(stdint.h poll.h sys/poll.h sys/timeb.h endian.h mach/clock.h)

AC_CHECK_LIB(msvcrt, fopen, [
	clib="msvcrt"
	UCOMMON_LIBS="$UCOMMON_LIBS -lmingwex -lmingw32 -lws2_32 -lwsock32 -lkernel32"
	UCOMMON_LIBC="-lmsvcrt -lgcc"
])

AC_CHECK_LIB($clib, socketpair, [
	AC_DEFINE(HAVE_SOCKETPAIR, [1], [have socketpair])
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

PTHREAD_FLAGS=""

AC_CHECK_HEADER(pthread.h, [
	PTHREAD_FLAGS="-mt -pthread -pthreads"
	UCOMMON_FLAGS="$UCOMMON_FLAGS -D_REENTRANT"
	AC_CHECK_LIB(pthread,pthread_create,[
		tlib="pthread"
		UCOMMON_LIBS="$UCOMMON_LIBS -lpthread"
	])
	if test -z "$tlib" ; then
		AC_CHECK_LIB(pthreadGC2,pthread_create,[
			tlib="pthreadGC2"
			UCOMMON_LIBS="$UCOMMON_LIBS -lpthreadGC2"
		])
	fi
	if test -z "$tlib" ; then
		AC_CHECK_LIB($clib, pthread_create, [tlib="$clib"])
	fi
	if test ! -z "$tlib" ; then
		AC_CHECK_LIB($tlib,pthread_condattr_setclock,[
			AC_DEFINE(HAVE_PTHREAD_CONDATTR_SETCLOCK, [1], ["pthread clocking"])
		])
		AC_CHECK_LIB($tlib,pthread_yield,[
			AC_DEFINE(HAVE_PTHREAD_YIELD, [1], ["pthread yield"])
		])
		AC_CHECK_LIB($tlib,pthread_delay,[
			AC_DEFINE(HAVE_PTHREAD_DELAY, [1], ["pthread delay"])
		])
	fi		
],[
	AC_MSG_ERROR("pthread support required")
])

socket_lib=""
posix4_lib=""
rt_lib=""

AC_CHECK_LIB(socket,socket,[
	AC_CHECK_LIB(socket, socketpair,[
		AC_DEFINE(HAVE_SOCKETPAIR, [1], [have socketpair in libsocket])
	])
	socket_lib="-lsocket"
])

for func in nanosleep clock_gettime localtime_r ; do
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
			])
		])
	])
	case $found in
	localtime_r)
		AC_DEFINE(HAVE_LOCALTIME_R, [1], [has localtime_r])
		;;
	nanosleep)
		AC_DEFINE(HAVE_NANOSLEEP, [1], [has nanosleep])
		;;
	clock_gettime)		
		AC_DEFINE(HAVE_POSIXTIME, [1], [has posix clock functions])
		;;
	esac
done

UCOMMON_LINKED="$socket_lib $posix4_lib $rt_lib $UCOMMON_LIBS"
UCOMMON_LIBS="$socket_lib $posix4_lib $rt_lib $UCOMMON_LIBS $UCOMMON_LIBC"

AC_CHECK_LIB(Systemstubs, printf, [
	UCOMMON_CLINK="$UCOMMON_CLINK -lSystemstubs"
])

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
COMPILER=""
UCOMMON_FLAGS=""
for flag in $CXXFLAGS ; do
	case $flag in
		-f*exceptions|-f*rtti|-f*check-new|-f*enforce-eh-specs|-finline|-f*except)
		;;
	-fvisibility=*)
		VISIBLE_FLAGS=""
		UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
		;;
	*)
		UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
		;;
	esac
done
export CXXFLAGS="$UCOMMON_FLAGS"
UCOMMON_FLAGS="-DNOSTDCPP"
UCOMMON_CLINK=""

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

flags=""
for flag in $CXXFLAGS ; do
	case "$flags" in
	*$flag*)
		;;
	*)
		flags="$flags $flag"
		;;
	esac
done
CXXFLAGS="$flags"

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

for flag in -fno-check-new -finline -fvisibility-inlines-hidden
do
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

UCOMMON_CLINK="$UCOMMON_CLINK $UCOMMON_LIBC"
UCOMMON_SHARED=""
UCOMMON_MODULE="-module -shared"

case "$target_os" in
osf*)
    UCOMMON_FLAGS="$UCOMMON_FLAGS -D_POSIX_C_SOURCE=1 -D_OSF_SOURCE=1 -D__USE_STD_IOSTREAM"
    ;;
cygwin*|mingw*)
    UCOMMON_SHARED="-no-undefined"
    UCOMMON_MODULE="-module -shared -no-undefined"
    ;;
darwin6*)
    UCOMMON_MODULE="-dynamic -bundle -undefined suppress -flat_namespace -read_only_relocs suppress"
    STAGE2="macosx"
    ;;
darwin*)
    UCOMMON_MODULE="-dynamic -bundle -undefined suppress -flat_namespace -read_only_relocs suppress"
    ;;
esac

AC_SUBST(CXXFLAGS)
AC_SUBST(UCOMMON_VISIBILITY)
AC_SUBST(UCOMMON_LINKED)
AC_SUBST(UCOMMON_SHARED)
AC_SUBST(UCOMMON_MODULE)
AC_SUBST(UCOMMON_FLAGS)
AC_SUBST(UCOMMON_CLINK)
AC_SUBST(UCOMMON_LIBC)
AC_SUBST(UCOMMON_LIBS)

AC_OUTPUT(Makefile src/Makefile inc/Makefile inc/ucommon/Makefile test/Makefile
ucommon.pc ucommon.spec)

