#include <config.h>
#include <ucommon/access.h>

using namespace UCOMMON_NAMESPACE;

Shared::~Shared()
{
}

Exclusive::~Exclusive()
{
}

shared_lock::shared_lock(Shared *obj)
{
	lock = obj;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
	lock->Shlock();

};

exclusive_lock::exclusive_lock(Exclusive *obj)
{
	lock = obj;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
	lock->Exlock();
}

shared_lock::~shared_lock()
{
	if(lock) {
		lock->Unlock();
		pthread_setcancelstate(state, NULL);
		lock = NULL;
	}
}

exclusive_lock::~exclusive_lock()
{
	if(lock) {
		lock->Unlock();
		pthread_setcancelstate(state, NULL);
		lock = NULL;
	}
}

void shared_lock::release()
{
	if(lock) {
		lock->Unlock();
		pthread_setcancelstate(state, NULL);
		lock = NULL;
		if(state == PTHREAD_CANCEL_ENABLE)
			pthread_testcancel();
	}
}

void exclusive_lock::release()
{
    if(lock) {
        lock->Unlock();
		pthread_setcancelstate(state, NULL);
        lock = NULL;
		if(state == PTHREAD_CANCEL_ENABLE)
			pthread_testcancel();
    }
}


