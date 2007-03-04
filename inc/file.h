#ifndef _UCOMMON_FILE_H_
#define	_UCOMMON_FILE_H_

#ifndef _UCOMMON_OBJECT_H_
#include ucommon/object.h
#endif

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

NAMESPACE_UCOMMON

class __EXPORT auto_close : private AutoObject
{
private:
	union {
#ifdef	_MSWINDOWS_
		HANDLE h;
#endif
		FILE *fp;
		DIR *dp;
		int fd;
	}	obj;

	enum {
#ifdef	_MSWINDOWS_
		T_HANDLE,
#endif
		T_FILE,
		T_DIR,
		T_FD,
		T_CLOSED
	} type;

	void release(void);

public:

#ifdef	_MSWINDOWS_
	auto_close(HANDLE hv);
#endif

	auto_close(FILE *fp);
	auto_close(DIR *dp);
	auto_close(int fd);
	~auto_close();
};

#define	autoclose(x)	auto_close __access_name(__ac__)(x)

extern "C" {
	__EXPORT bool cpr_isfile(const char *fn);	
	__EXPORT bool cpr_isdir(const char *fn);
}

END_NAMESPACE

#endif
