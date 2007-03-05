#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

NAMESPACE_UCOMMON

__EXPORT void suspend(void);
__EXPORT void suspend(timeout_t timeout);

END_NAMESPACE

extern "C" {

	__EXPORT void cpr_closeall(void);

	__EXPORT int cpr_getexit(pid_t pid);

	inline void cpr_sleep(timeout_t timeout)
		{ucc::suspend(timeout);};

	inline void cpr_yield(void)
		{ucc::suspend();};
};

#endif
