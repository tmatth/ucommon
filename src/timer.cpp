#include <private.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/linked.h>
#include <inc/timers.h>

using namespace UCOMMON_NAMESPACE;

static time_t difftime(time_t ref)
{
	time_t now;
	time(&now);

	return ref - now;
}

#if _POSIX_TIMERS > 0
static void adj(struct timespec *ts)
{
	if(ts->tv_nsec >= 1000000000l)
		ts->tv_sec += (ts->tv_nsec / 1000000000l);
	ts->tv_nsec %= 1000000000l;
	if(ts->tv_nsec < 0)
		ts->tv_nsec = -ts->tv_nsec;
}
#else
static void adj(struct timeval *ts)
{
    if(ts->tv_usec >= 1000000l)
        ts->tv_sec += (ts->tv_usec / 1000000l);
    ts->tv_usec %= 1000000l;
    if(ts->tv_usec < 0)
        ts->tv_usec = -ts->tv_usec;
}
#endif

const timeout_t Timer::inf = ((timeout_t)(-1));
const time_t Timer::reset = ((time_t)0);

Timer::Timer(Timer **root) :
LinkedObject((LinkedObject **)(root))
{
	clear();
	list = root;
}

Timer::Timer() :
LinkedObject()
{
	list = NULL;
	set();
}

Timer::Timer(timeout_t in) :
LinkedObject()
{
	list = NULL;
	set();
	operator+=(in);
}

Timer::Timer(time_t in) :
LinkedObject()
{
	list = NULL;
	set();
	timer.tv_sec += difftime(in);
}

Timer::~Timer()
{
	release();
}

void Timer::release(void)
{
	if(list)
		delist((LinkedObject **)list);
	list = NULL;
}

void Timer::set(void)
{
#ifdef  _POSIX_MONOTONIC_CLOCK
    clock_gettime(CLOCK_MONOTONIC, &timer);
#elif _POSIX_TIMERS > 0
    clock_gettime(CLOCK_REALTIME, &timer);
#else
    gettimeofday(&timer, NULL);
#endif
};	

void Timer::expired(void)
{
}

void Timer::clear(void)
{
	timer.tv_sec = 0;
#if _POSIX_TIMERS > 0
	timer.tv_nsec = 0;
#else
	timer.tv_usec = 0;
#endif
};	

bool Timer::isExpired(void)
{
#if _POSIX_TIMERS > 0
	if(!timer.tv_sec && !timer.tv_nsec)
		return true;
#else
	if(!timer.tv_sec && !timer.tv_usec)
		return true;
#endif
	return false;
}

void Timer::arm(timeout_t timeout)
{
	if(!timeout) {
		clear();
		return;
	}
	set();
	operator+=(timeout);
}

timeout_t Timer::get(void)
{
	timeout_t diff;
#if _POSIX_TIMERS > 0
	struct timespec current;

#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &current);
#else
	clock_gettime(CLOCK_REALTIME, &current);
#endif
	adj(&current);
	if(current.tv_sec > timer.tv_sec)
		return 0;
	if(current.tv_sec == timer.tv_sec && current.tv_nsec > timer.tv_nsec)
		return 0;
	diff = (timer.tv_sec - current.tv_sec) * 1000;
	diff += ((timer.tv_nsec - current.tv_nsec) / 1000000l);
#else
	struct timeval current;
	gettimeofday(&current, NULL);
	adj(&current);
	if(current.tv_sec > timer.tv_sec)
		return 0;
	if(current.tv_sec == timer.tv_sec && current.tv_usec > timer.tv_usec)
		return 0;
	diff = (timer.tv_sec - current.tv_sec) * 1000;
	diff += ((timer.tv_usec - current.tv_usec) / 1000);
#endif
	return diff;
}

bool Timer::operator!()
{
	if(get())
		return true;

	return false;
}

void Timer::operator=(timeout_t to)
{
#ifdef	_POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &timer);
#elif _POSIX_TIMERS > 0
	clock_gettime(CLOCK_REALTIME, &timer);
#else
	gettimeofday(&timer, NULL);
#endif
	operator+=(to);
}

void Timer::operator+=(timeout_t to)
{
	if(isExpired())
		set();

	timer.tv_sec += (to / 1000);
#if _POSIX_TIMERS > 0
	timer.tv_nsec += (to % 1000l) * 1000000l;
#else
	timer.tv_usec += (to % 1000l) * 1000l;
#endif
	adj(&timer);
}

void Timer::operator-=(timeout_t to)
{
	if(isExpired())
		set();
    timer.tv_sec -= (to / 1000);
#if _POSIX_TIMERS > 0
    timer.tv_nsec -= (to % 1000l) * 1000000l;
#else
    timer.tv_usec -= (to % 1000l) * 1000l;
#endif
    adj(&timer);
}


void Timer::operator+=(time_t abs)
{
	if(isExpired())
		set();
	timer.tv_sec += difftime(abs);
}

void Timer::operator-=(time_t abs)
{
	if(isExpired())
		set();
	timer.tv_sec -= difftime(abs);
}

void Timer::operator=(time_t abs)
{
#ifdef	_POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &timer);
#elif _POSIX_TIMERS > 0
	clock_gettime(CLOCK_REALTIME, &timer);
#else
	gettimeofday(&timer, NULL);
#endif
	if(!abs)
		return;

	timer.tv_sec += difftime(abs);
}

void Timer::sync(Timer &t)
{
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t.timer, NULL);
#elif _POSIX_TIMERS > 0
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t.timer, NULL);
#else
	usleep(t.get());
#endif
}

timeout_t Timer::expire(Timer *timer)
{
	timeout_t first = inf, next;

	while(timer) {
		if(!(timer->isExpired())) {
			next = timer->get();
			if(!next) {
				timer->clear();
				timer->expired();
				next = timer->get();
			}
			if(next && next < first)
				first = next;
		}
		timer = static_cast<Timer *>(timer->getNext());
	}
	return first;	
}
