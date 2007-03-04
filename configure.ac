AC_INIT(reconfig)

VERSION="0.4.0"
LT_RELEASE="0.4"
LT_VERSION="0:0"

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXXCPP
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(ucommon, [$VERSION])
AM_CONFIG_HEADER(private.h)

LT_MAJOR="`echo $LT_VERSION | sed s/:.*$//`"
LT_MINOR="`echo $LT_VERSION | sed s/.*://`"
LT_SUBVER="$LT_MAJOR.$LT_MINOR"
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_SUBVER)
AC_SUBST(LT_MAJOR)
AC_SUBST(LT_MINOR)

UCOMMON_VISIBILITY="0"
UCOMMON_THREADING="0"
UCOMMON_LIBC="-lc"
UCOMMON_LIBS=""

AC_ARG_WITH(libc,
	[  --with-libc          specify alternate libc],[
	if test x$with_libc != x ; then
		clib=`echo ${with_libc} | sed s/[-]l//`
        UCOMMON_LIBC=-l${clib} ; fi
])

clib=`echo ${UCOMMON_LIBC} | sed s/[-]l//`
tlib=""

AC_CHECK_HEADERS(poll.h sys/poll.h)

AC_CHECK_LIB($clib, stricmp, [
	AC_DEFINE(HAVE_STRICMP, [1], [string icmp])
])

AC_CHECK_LIB($clib, strcasecmp, [
    AC_DEFINE(HAVE_STRCASECMP, [1], [string icmp])
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
	UCOMMON_THREADING="1"
	UCOMMON_FLAGS="$UCOMMON_FLAGS -D_REENTRANT"
	AC_CHECK_LIB(pthread,pthread_create,[
		tlib="pthread"
		UCOMMON_LIBS="$UCOMMON_LIBS -lpthread"
	])
	if test -z "$tlib" ; then
		AC_CHECK_LIB($clib, pthread_create, [tlib="$clib"])
	fi
	if test ! -z "$tlib" ; then
		AC_CHECK_LIB($tlib,pthread_atfork,[
			AC_DEFINE(HAVE_PTHREAD_ATFORK, [1], ["fork support"])			
		])
	fi		
],[
	AC_MSG_WARN("no pthread support; threading disabled")
])

socket_lib=""
posix4_lib=""
rt_lib=""

AC_CHECK_LIB(socket,socket,[
	socket_lib="-lsocket"])

for func in sched_setscheduler nanosleep clock_gettime ; do
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
	sched_setscheduler)
		AC_DEFINE(HAVE_SCHEDULING, [1], [has posix scheduling])
		;;
	nanosleep)
		AC_DEFINE(HAVE_NANOSLEEP, [1], [has nanosleep])
		;;
	clock_gettime)		
		AC_DEFINE(HAVE_POSIXTIME, [1], [has posix clock functions])
		;;
	esac
done

UCOMMON_LIBS="$socket_lib $posix4_lib $rt_lib $UCOMMON_LIBS $UCOMMON_LIBC"

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
COMPILER=""
UCOMMON_FLAGS=""
for flag in $CXXFLAGS $CFLAGS; do
	case $flag in
		-f*exceptions|-f*rtti|-f*check-new|-f*enforce-eh-specs|-finline|-f*except)
		;;
	-fvisibility=*)
		VISIBLE_FLAGS=""
		UCOMMON_FLAGS="$UCOMMON_FLAGS $flags"
		;;
	-*)
		UCOMMON_FLAGS="$UCOMMON_FLAGS $flag"
		;;
	*)
		;;
	esac
done
export CXXFLAGS="$UCOMMON_FLAGS"
UCOMMON_FLAGS="-DNOSTDCPP"

if test "$GCC" = yes ; then
	ucflags="-fno-exceptions -fno-rtti -fno-enforce-eh-specs -nodefaultlibs -nostdinc++"
else
	ucflags="-features=no%except -features=namespace -features=no%rtti -xnolib"
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

AC_MSG_CHECKING([whether ${CXX} supports visibility])
echo 'void f(){}' >conftest.cpp
if test -z "`${CXX} -fvisibility=hidden -c conftest.cpp 2>&1`"; then
	UCOMMON_FLAGS="$UCOMMON_FLAGS -fvisibility=hidden"
    AC_MSG_RESULT(yes)
	UCOMMON_VISIBILITY="1"
else
    AC_MSG_RESULT(no)
fi
rm -f conftest*

AC_LANG_RESTORE

AC_SUBST(UCOMMON_VISIBILITY)
AC_SUBST(UCOMMON_THREADING)
AC_SUBST(UCOMMON_SHARED)
AC_SUBST(UCOMMON_MODULE)
AC_SUBST(UCOMMON_FLAGS)
AC_SUBST(UCOMMON_LIBS)

AC_OUTPUT(Makefile src/Makefile test/Makefile inc/Makefile inc/config.h)

