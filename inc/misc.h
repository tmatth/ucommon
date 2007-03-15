#ifndef _UCOMMON_MISC_H_
#define	_UCOMMON_MISC_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/config.h>
#endif

extern "C" {
	const size_t uuid_size = 38;

	__EXPORT void cpr_md5hash(char *out, const char *source, size_t size = 0);
	__EXPORT int cpr_uuid(char *out);
	__EXPORT size_t cpr_urldecode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_urlencode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_urlencodesize(char *str);
	__EXPORT size_t cpr_xmldecode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_xmlencode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_b64decode(caddr_t out, const char *src, size_t count);
	__EXPORT size_t cpr_b64encode(char *out, caddr_t src, size_t count);
	__EXPORT size_t cpr_b64len(const char *str);
	__EXPORT size_t cpr_snprintf(char *buf, size_t size, const char *fmt, ...);
	__EXPORT void cpr_printlog(const char *path, const char *fmt, ...);
};

#ifdef	DEBUG
#define cpr_printdbg(fmt, ...)	printf(fmt, ...)
#else
#define	cpr_printdbg(fmt, ...)
#endif

#endif
