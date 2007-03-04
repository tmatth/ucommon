#ifndef _UCOMMON_TIMERS_H_
#define	_UCOMMON_TIMERS_H_

#ifndef _UCOMMON_CONFIG_H_
#include ucommon/config.h
#endif

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

typedef unsigned long timeout_t;

NAMESPACE_UCOMMON

class __EXPORT Timer
{
private:
#if _POSIX_TIMERS > 0
	timespec timer;
#else
	timeval timer;
#endif

public:
	static const timeout_t inf;
	static const time_t reset;

	Timer();
	Timer(timeout_t offset);
	Timer(time_t offset);
	
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

END_NAMESPACE

#endif
