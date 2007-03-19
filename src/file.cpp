#include <private.h>
#include <ucommon/file.h>
#include <stdlib.h>
#include <unistd.h>


using namespace UCOMMON_NAMESPACE;

auto_close::auto_close(FILE *fp) :
AutoObject()
{
	obj.fp = fp;
	type = T_FILE;
}

auto_close::auto_close(DIR *dp) :
AutoObject()
{
	obj.dp = dp;
	type = T_DIR;
}

#ifdef	_MSWINDOWS_
auto_close::auto_close(HANDLE hv) :
AutoObject()
{
    obj.h = hv;
    type = T_HANDLE;
}

#endif

auto_close::auto_close(int fd) :
AutoObject()
{
	obj.fd = fd;
	type = T_FD;
}

auto_close::~auto_close()
{
	delist();
	auto_close::release();
}

void auto_close::release(void)
{
	switch(type) {
#ifdef	_MSWINDOWS_
	case T_HANDLE:
		::CloseHandle(obj.h);
		break;
#endif
	case T_DIR:
		::closedir(obj.dp);
		break;
	case T_FILE:
		::fclose(obj.fp);
		break;
	case T_FD:
		::close(obj.fd);
	case T_CLOSED:
		break;
	}
	type = T_CLOSED;
}

extern "C" bool cpr_isdir(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISDIR(ino.st_mode))
		return true;

	return false;
}

extern "C" bool cpr_isfile(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISREG(ino.st_mode))
		return true;

	return false;
}


