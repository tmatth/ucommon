// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

/**
 * Realtime timers and timer queues.
 * This offers ucommon support for realtime high-resolution threadsafe
 * timers and timer queues.  Threads may be scheduled by timers and timer
 * queues may be used to inject timer events into callback objects or through
 * virtuals.
 * @file ucommon/timers.h
 */

#ifndef _UCOMMON_TIMERS_H_
#define	_UCOMMON_TIMERS_H_

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef _MSWINDOWS_
#include <unistd.h>
#include <sys/time.h>
#endif

#include <time.h>

NAMESPACE_UCOMMON

class __EXPORT Timer
{
private:
	friend class Conditional;
	friend class Semaphore;
	friend class Event;

#if _POSIX_TIMERS > 0
	timespec timer;
#else
	timeval timer;
#endif
	bool updated;

public:
	static const timeout_t inf;
	static const time_t reset;

	Timer();
	Timer(timeout_t offset);
	Timer(time_t offset);

	bool isExpired(void);
	bool isUpdated(void);

	void set(timeout_t offset);
	void set(time_t offset);
	void set(void);
	void clear(void);	
	timeout_t get(void);

	inline timeout_t operator*()
		{return get();};

	bool operator!();
	void operator=(time_t offset);
	void operator=(timeout_t offset);
	void operator+=(time_t adj);
	void operator+=(timeout_t adj);
	void operator-=(time_t adj);
	void operator-=(timeout_t adj);

	static void sync(Timer &timer);
};

	
class __EXPORT TimerQueue : public OrderedIndex
{
public:
	class __EXPORT event : protected Timer, public LinkedList
	{
	protected:
		friend class TimerQueue;

		event(timeout_t arm);
		event(TimerQueue *tq, timeout_t arm);

		virtual void expired(void) = 0;
		virtual timeout_t timeout(void);

	public:
		virtual ~event();

		void attach(TimerQueue *tq);
		void detach(void);

        inline void arm(timeout_t timeout)
			{set(timeout);};

		inline void disarm(void)
			{clear();};

		inline bool isExpired(void)
			{return Timer::isExpired();};

		inline timeout_t get(void)
			{return Timer::get();};

		void update(void);

		inline TimerQueue *getQueue(void)
			{return static_cast<TimerQueue*>(root);};
	};	

protected:
	friend class event;

	virtual void update(void) = 0;
	virtual void modify(void) = 0;

public:
	TimerQueue();
	virtual ~TimerQueue();

	void operator+=(event &te);
	void operator-=(event &te);

	timeout_t expire();
};	

typedef TimerQueue::event TimerEvent;
typedef	Timer timer_t;

END_NAMESPACE

extern "C" {
#if defined(WIN32)
	__EXPORT int gettimeofday(struct timeval *tv, void *tz);
#endif
};

#endif
