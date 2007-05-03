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

typedef unsigned long timeout_t;

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
	class __EXPORT event : private Timer, public LinkedList
	{
	protected:
		friend class TimerQueue;

		event(timeout_t arm);
		event(TimerQueue *tq, timeout_t arm);

		void attach(TimerQueue *tq);
		void detach(void);

		virtual void expired(void) = 0;
		virtual timeout_t timeout(void);

	public:
		virtual ~event();

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

END_NAMESPACE

extern "C" {
#if defined(WIN32)
	__EXPORT int gettimeofday(struct timeval *tv, void *tz);
#endif
};

#endif
