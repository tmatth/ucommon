#ifndef _UCOMMON_MISC_H_
#define	_UCOMMON_MISC_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/config.h>
#endif

extern "C" {
	void __EXPORT cpr_md5hash(char *out, const char *source, size_t size = 0);
};

#endif
