#ifndef _UCOMMON_TIMERS_H_
#define	_UCOMMON_TIMERS_H_

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

typedef unsigned long timeout_t;

NAMESPACE_UCOMMON

class __EXPORT Timer : public LinkedObject
{
private:
	friend class __EXPORT Conditional;
	friend class __EXPORT Semaphore;
	friend class __EXPORT Event;

#if _POSIX_TIMERS > 0
	timespec timer;
#else
	timeval timer;
#endif
	Timer **list;

protected:
	virtual void expired(void);
	virtual void release(void);

public:
	static const timeout_t inf;
	static const time_t reset;

	Timer(Timer **root);
	Timer();
	Timer(timeout_t offset);
	Timer(time_t offset);
	virtual ~Timer();

	bool isExpired(void);

	virtual void arm(timeout_t timeout);
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
	static timeout_t expire(Timer *list);
};

END_NAMESPACE

#endif
