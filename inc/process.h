#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

NAMESPACE_UCOMMON

__EXPORT void suspend(void);
__EXPORT void suspend(timeout_t timeout);

END_NAMESPACE

#endif
