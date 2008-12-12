// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/timers.h>
#include <ucommon/thread.h>

using namespace UCOMMON_NAMESPACE;

#if _POSIX_TIMERS > 0 && defined(HAVE_MACH_CLOCK_H) && !defined(HAVE_CLOCK_GETTIME)
// wonder what was here...??
#endif

static long difftime(time_t ref)
{
	time_t now;
	time(&now);

	return (long)(ref - now);
}

#if _POSIX_TIMERS > 0
static void adj(struct timespec *ts)
{
	assert(ts != NULL);

	if(ts->tv_nsec >= 1000000000l)
		ts->tv_sec += (ts->tv_nsec / 1000000000l);
	ts->tv_nsec %= 1000000000l;
	if(ts->tv_nsec < 0)
		ts->tv_nsec = -ts->tv_nsec;
}
#else
static void adj(struct timeval *ts)
{
	assert(ts != NULL);

    if(ts->tv_usec >= 1000000l)
        ts->tv_sec += (ts->tv_usec / 1000000l);
    ts->tv_usec %= 1000000l;
    if(ts->tv_usec < 0)
        ts->tv_usec = -ts->tv_usec;
}
#endif

#ifdef	WIN32
#ifdef	_WIN32_WCE
extern "C" int gettimeofday(struct timeval *tv_,  void *tz_)
{
	assert(tv_ != NULL);

	// We could use _ftime(), but it is not available on WinCE.
	// (WinCE also lacks time.h)
	// Note also that the average error of _ftime is around 20 ms :)
	DWORD ms = GetTickCount();
	tv_->tv_sec = ms / 1000;
	tv_->tv_usec = ms * 1000;
	return 0;
}
#else
#ifdef	HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

extern "C" int gettimeofday(struct timeval *tv_, void *tz_)
{
	assert(tv_ != NULL);

#if defined(_MSC_VER) && _MSC_VER >= 1300 
	struct __timeb64 tb;
	_ftime64(&tb);
#else
# ifndef __BORLANDC__
	struct _timeb tb;
	_ftime(&tb);
# else
	struct timeb tb;
	ftime(&tb);
# endif
#endif
	tv_->tv_sec = (long)tb.time;
	tv_->tv_usec = tb.millitm * 1000;
	return 0;
}
#endif
#endif

const timeout_t Timer::inf = ((timeout_t)(-1));
const time_t Timer::reset = ((time_t)0);

TimerQueue::event::event(timeout_t timeout) :
Timer(), LinkedList()
{
	set(timeout);
}

TimerQueue::event::event(TimerQueue *tq, timeout_t timeout) :
Timer(), LinkedList()
{
	set(timeout);
	isUpdated();
	attach(tq);
}

TimerQueue::event::~event()
{
	detach();
}

Timer::Timer()
{
	clear();
}

Timer::Timer(timeout_t in)
{
	set();
	operator+=(in);
}

Timer::Timer(time_t in)
{
	set();
	timer.tv_sec += difftime(in);
}

void Timer::set(timeout_t timeout)
{
	set();
	operator+=(timeout);
}

void Timer::set(time_t time)
{
	set();
	timer.tv_sec += difftime(time);
}

void TimerQueue::event::attach(TimerQueue *tq)
{
	if(tq == getQueue())
		return;

	detach();
	if(!tq)
		return;

	tq->modify();
	enlist(tq);
	isUpdated();
	tq->update();
}

void TimerQueue::event::arm(timeout_t timeout)
{
	TimerQueue *tq = getQueue();
	if(tq)
		tq->modify();
	set(timeout);
	if(tq)
		tq->update();
}

void TimerQueue::event::disarm(void)
{
	TimerQueue *tq = getQueue();
	bool flag = isExpired();

	if(tq && !flag)
		tq->modify();
	clear();
	if(tq && !flag)
		tq->update();
}

void TimerQueue::event::update(void)
{
	TimerQueue *tq = getQueue();
	if(isUpdated() && tq) {
		tq->modify();
		tq->update();
	} 
}

void TimerQueue::event::detach(void)
{
	TimerQueue *tq = getQueue();
	if(tq) {
		tq->modify();
		clear();
		delist();
		tq->update();
	}
}

void Timer::set(void)
{
#if _POSIX_TIMERS > 0 && defined(_POSIX_MONOTONIC_CLOCK)
    clock_gettime(CLOCK_MONOTONIC, &timer);
#elif _POSIX_TIMERS > 0
    clock_gettime(CLOCK_REALTIME, &timer);
#else
    gettimeofday(&timer, NULL);
#endif
	updated = true;
}

void Timer::clear(void)
{
	timer.tv_sec = 0;
#if _POSIX_TIMERS > 0
	timer.tv_nsec = 0;
#else
	timer.tv_usec = 0;
#endif
	updated = false;
}

bool Timer::isUpdated(void)
{
	bool rtn = updated;
	updated = false;
	return rtn;
}

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
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_TIMERS > 0
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
	updated = true;
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
	updated = true;
}

void Timer::operator-=(time_t abs)
{
	if(isExpired())
		set();
	timer.tv_sec -= difftime(abs);
}

void Timer::operator=(time_t abs)
{
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_TIMERS > 0
	clock_gettime(CLOCK_MONOTONIC, &timer);
#elif _POSIX_TIMERS > 0
	clock_gettime(CLOCK_REALTIME, &timer);
#else
	gettimeofday(&timer, NULL);
#endif
	if(!abs)
		return;

	timer.tv_sec += difftime(abs);
	updated = true;
}

void Timer::sync(Timer &t)
{
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_TIMERS > 0 && defined(HAVE_CLOCK_NANOSLEEP)
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t.timer, NULL);
#elif _POSIX_TIMERS > 0 && defined(HAVE_CLOCK_NANOSLEEP)
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t.timer, NULL);
#elif defined(_MSWINDOWS_)
	SleepEx(t.get(), FALSE);
#else
	usleep(t.get());
#endif
}


timeout_t TQEvent::timeout(void)
{
	timeout_t timeout = get();
	if(!isExpired() && !timeout) {
		disarm();
		expired();
		timeout = get();
		isUpdated();
	}
	return timeout;
}

TimerQueue::TimerQueue() : OrderedIndex()
{
}

TimerQueue::~TimerQueue()
{
}

timeout_t TimerQueue::expire(void)
{
	timeout_t first = Timer::inf, next;
	linked_pointer<TimerQueue::event> timer = begin();
	TimerQueue::event *tp;

	while(timer) {
		tp = *timer;
		timer.next();
		next = tp->timeout();
		if(next && next < first)
			first = next;
	}
	return first;	
}

void TimerQueue::operator+=(event &te) { te.attach(this); }

void TimerQueue::operator-=(event &te)
{
	if(te.getQueue() == this)
		te.detach();
}
