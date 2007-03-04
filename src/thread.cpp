#include <private.h>
#include <inc/config.h>
#include <inc/access.h>
#include <inc/thread.h>

#if	UCOMMON_THREADING > 0

using namespace UCOMMON_NAMESPACE;

Mutex::attribute Mutex::attr;

Mutex::attribute::attribute()
{
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
}

Mutex::Mutex()
{
	crit(pthread_mutex_init(&mutex, &attr.attr) == 0);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&mutex);
}

void Mutex::Exlock(void)
{
	pthread_mutex_lock(&mutex);
}

void Mutex::Unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

Spinlock::Spinlock()
{
	crit(pthread_spin_init(&spin, 0) == 0);
}

Spinlock::~Spinlock()
{
	pthread_spin_destroy(&spin);
}

void Spinlock::Exlock(void)
{
	pthread_spin_lock(&spin);
}

void Spinlock::Unlock(void)
{
	pthread_spin_unlock(&spin);
}

bool Spinlock::operator!()
{
	if(pthread_spin_trylock(&spin))
		return true;

	return false;
}

Barrier::Barrier(unsigned count)
{
	crit(pthread_barrier_init(&barrier, NULL, count) == 0);
}

Barrier::~Barrier()
{
	pthread_barrier_destroy(&barrier);
}

void Barrier::wait(void)
{
	pthread_barrier_wait(&barrier);
}

Threadlock::Threadlock()
{
	crit(pthread_rwlock_init(&lock, NULL) == 0);
}

Threadlock::~Threadlock()
{
	pthread_rwlock_destroy(&lock);
}

void Threadlock::Unlock(void)
{
	pthread_rwlock_unlock(&lock);
}

void Threadlock::Exlock(void)
{
	pthread_rwlock_wrlock(&lock);
}

void Threadlock::Shlock(void)
{
	pthread_rwlock_rdlock(&lock);
}

auto_cancellation::auto_cancellation(int ntype, int nmode)
{
	pthread_setcanceltype(ntype, &type);
	pthread_setcancelstate(nmode, &mode);
}

auto_cancellation::~auto_cancellation()
{
	int ign;

	pthread_setcancelstate(mode, &ign);
	pthread_setcanceltype(type, &ign);
}

#endif
