#include <private.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
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
