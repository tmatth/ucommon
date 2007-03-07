#include <private.h>
#include <inc/config.h>
#include <inc/access.h>
#include <inc/object.h>
#include <inc/linked.h>
#include <inc/timers.h>
#include <inc/memory.h>
#include <inc/thread.h>
#include <inc/file.h>
#include <inc/process.h>
#include <errno.h>

#if	UCOMMON_THREADING > 0

using namespace UCOMMON_NAMESPACE;

Mutex::attribute Mutex::attr;
Conditional::attribute Conditional::attr;

Mutex::attribute::attribute()
{
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
}

Event::Event()
{
	count = 0;
	signalled = false;

    crit(pthread_cond_init(&cond, &Conditional::attr.attr) == 0);
    crit(pthread_mutex_init(&mutex, NULL) == 0);
}

Event::~Event()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void Event::signal(void)
{
    pthread_mutex_lock(&mutex);
    signalled = true;
    ++count;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void Event::reset(void)
{
    pthread_mutex_lock(&mutex);
    signalled = true;
	pthread_mutex_unlock(&mutex);
}

bool Event::wait(Timer &timer)
{
	pthread_mutex_lock(&mutex);
	int rc = 0;
	long last = count;
	while(!signalled && count == last && rc != ETIMEDOUT)
		rc = pthread_cond_timedwait(&cond, &mutex, &timer.timer);
	pthread_mutex_unlock(&mutex);
	if(rc == ETIMEDOUT)
		return false;
	return true;
}

bool Event::wait(timeout_t timeout)
{
	Timer timer;
	timer += timeout;
	return wait(timer);
}

void Event::wait(void)
{
	pthread_mutex_lock(&mutex);
	long last = count;
	while(!signalled && count == last)
		pthread_cond_wait(&cond, &mutex);
 	pthread_mutex_unlock(&mutex);
}

Semaphore::Semaphore(unsigned limit)
{
	count = limit;
	waits = 0;

	crit(pthread_cond_init(&cond, &Conditional::attr.attr) == 0);
	crit(pthread_mutex_init(&mutex, NULL) == 0);
}

Semaphore::~Semaphore()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

bool Semaphore::wait(Timer &timer)
{
	int rtn = 0;
	bool result = true;
	pthread_mutex_lock(&mutex);
	if(!count) {
		++waits;
		while(!count && rtn != ETIMEDOUT)
			pthread_cond_timedwait(&cond, &mutex, &timer.timer);
		--waits;
	}
	if(rtn == ETIMEDOUT)
		result = false;
	else
		--count;
	pthread_mutex_unlock(&mutex);
	return result;
}

bool Semaphore::wait(timeout_t timeout)
{
	if(timeout) {
		Timer timer;
		timer += timeout;
		return wait(timer);
	}
	Semaphore::Shlock();
	return true;
}
		
void Semaphore::Shlock(void)
{
	pthread_mutex_lock(&mutex);
	if(!count) {
		++waits;
		while(!count)
			pthread_cond_wait(&cond, &mutex);
		--waits;
	}
	--count;
	pthread_mutex_unlock(&mutex);
}

void Semaphore::Unlock(void)
{
	pthread_mutex_lock(&mutex);
	++count;
	if(waits)
		pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

Conditional::attribute::attribute()
{
	pthread_condattr_init(&attr);
#ifdef	_POSIX_MONOTONIC_CLOCK
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#else
	pthread_condattr_setclock(&attr, CLOCK_REALTIME);
#endif
}

Conditional::Conditional()
{
	crit(pthread_cond_init(&cond, &attr.attr) == 0);
	crit(pthread_mutex_init(&mutex, NULL) == 0);
	locker = 0;
}

void Conditional::Exlock(void)
{
	pthread_mutex_lock(&mutex);
	locker = pthread_self();
}

void Conditional::Unlock(void)
{
	locker = 0;
	pthread_mutex_unlock(&mutex);
}

void Conditional::signal(bool broadcast)
{
	if(broadcast)
		pthread_cond_broadcast(&cond);
	else
		pthread_cond_signal(&cond);
}

bool Conditional::wait(timeout_t timeout)
{
	if(timeout) {
		Timer timer;
		timer += timeout;
		return wait(timer);
	}

	pthread_t self = pthread_self();
	if(!locker || !pthread_equal(locker, self))
		pthread_mutex_lock(&mutex);

	pthread_cond_wait(&cond, &mutex);

	if(!locker || !pthread_equal(locker, self))
		pthread_mutex_unlock(&mutex);
	return true;
}		

bool Conditional::wait(Timer &timer)
{
	int rtn;
	pthread_t self = pthread_self();
	if(!locker || !pthread_equal(locker, self))
		pthread_mutex_lock(&mutex);

	rtn = pthread_cond_timedwait(&cond, &mutex, &timer.timer);

	if(!locker || !pthread_equal(locker, self))
		pthread_mutex_unlock(&mutex);

	if(rtn == ETIMEDOUT)
		return false;
	return true;
}		


Conditional::~Conditional()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
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

Thread::Thread(int p, size_t size)
{
	priority = p;
	stack = size;
}

Thread::~Thread()
{
	release();
}

extern "C" {
	static void *exec_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);
		th->run();
		th->release();
	}
};

void Thread::start(bool detach)
{
	pthread_attr_t attr;
	if(running)
		return;

	detached = detach;
	pthread_attr_init(&attr);
	if(detach)
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	else
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 
	if(stack)
		pthread_attr_setstacksize(&attr, stack);
	pthread_create(&tid, &attr, &exec_thread, this);
	pthread_attr_destroy(&attr);
}

void Thread::release(void)
{
	pthread_t self = pthread_self();

	if(tid != 0 && pthread_equal(tid, self)) {
		running = false;
		if(detached) {
			tid = 0;
			delete this;
		}
		else {
			suspend();
			pthread_testcancel();
		}
		pthread_exit(NULL);
	}

	if(tid != 0 && !detached) {
		suspend();
		if(running)
			pthread_cancel(tid);
		if(!pthread_join(tid, NULL)) {
			running = false;
			tid = 0;
		}
	}	
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
