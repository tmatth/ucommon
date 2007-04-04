#include <config.h>
#include <ucommon/access.h>

using namespace UCOMMON_NAMESPACE;

Shared::~Shared()
{
}

Exclusive::~Exclusive()
{
}

auto_shared::auto_shared(Shared *obj)
{
	lock = obj;
	lock->Shlock();
};

auto_exclusive::auto_exclusive(Exclusive *obj)
{
	lock = obj;
	lock->Exlock();
}

auto_shared::~auto_shared()
{
	release();
}

auto_exclusive::~auto_exclusive()
{
	release();
}

void auto_shared::release()
{
	if(lock) {
		lock->Unlock();
		lock = NULL;
	}
}

void auto_exclusive::release()
{
    if(lock) {
        lock->Unlock();
        lock = NULL;
    }
}


