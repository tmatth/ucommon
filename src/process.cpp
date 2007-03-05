#include <private.h>

#ifdef	HAVE_SIGWAIT2
#define	_POSIX_PTHREAD_SEMANTICS
#endif

#if defined(HAVE_SIGACTION) && defined(HAVE_BSD_SIGNAL_H)
#undef	HAVE_BSD_SIGNAL_H
#endif

#if defined(HAVE_BSD_SIGNAL_H)
#include <bsd/signal.h>
#define	HAVE_SIGNALS
#elif defined(HAVE_SIGNAL_H)
#include <signal.h>
#define	HAVE_SIGNALS
#endif

#ifdef	HAVE_SIGNALS
#ifndef	SA_ONESHOT
#define	SA_ONESHOT	SA_RESETHAND
#endif
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/access.h>
#include <inc/timers.h>
#include <inc/thread.h>
#include <inc/process.h>


using namespace UCOMMON_NAMESPACE;


void ucc::suspend(timeout_t timeout)
{
	timespec ts;
	ts.tv_sec = timeout / 1000l;
	ts.tv_nsec = (timeout % 1000l) * 1000000l;
#ifdef	HAVE_PTHREAD_DELAY
	pthread_delay(&ts);
#else
	nanosleep(&ts, NULL);
#endif
}

void ucc::suspend(void)
{
#ifdef	HAVE_PTHREAD_YIELD
	pthread_yield();
#else
	sched_yield();
#endif
}

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

#ifndef	_MSWINDOWS_
extern "C" void cpr_closeall(void)
{
	unsigned max = OPEN_MAX;
#if defined(HAVE_SYSCONF)
	max = sysconf(_SC_OPEN_MAX);
#endif
#if defined(HAVE_SYS_RESOURCE_H)
	struct rlimit rl;
	if(!getrlimit(RLIMIT_NOFILE, &rl))
		max = rl.rlim_cur;
#endif
	for(unsigned fd = 3; fd < max; ++fd)
		::close(fd);
}

extern "C" int cpr_getexit(pid_t pid)
{
	int status;

	if(waitpid(pid, &status, 0))
		return -1;

	return WEXITSTATUS(status);
}

#else
extern "C" void cpr_closeall(void)
{
}
#endif

