#ifndef _UCOMMON_THREAD_H_
#define	_UCOMMON_THREAD_H_

#if UCOMMON_THREADING > 0

#ifndef _UCOMMON_ACCESS_H_
#include ucommon/access.h
#endif

#include <pthread.h>

NAMESPACE_UCOMMON

class __EXPORT Barrier 
{
private:
	pthread_barrier_t barrier;

public:
	Barrier(unsigned count);
	~Barrier();

	void wait(void);
};

class __EXPORT Spinlock : public Exclusive
{
private:
	pthread_spinlock_t spin;

public:
	Spinlock();
	~Spinlock();

	bool operator!(void);

	void Exlock(void);
	void Unlock(void);	

	inline void lock(void)
		{return Exlock();};

	inline void unlock(void)
		{return Unlock();};

	inline void release(void)
		{return Unlock();};
};

class __EXPORT Mutex : public Exclusive
{
private:
	class __EXPORT attribute
	{
	public:
		pthread_mutexattr_t attr;
		attribute();
	};

	static attribute attr;
	pthread_mutex_t mutex;
		
public:
	Mutex();
	~Mutex();

	void Exlock(void);
	void Unlock(void);

	inline void lock(void)
		{Exlock();};

	inline void unlock(void)
		{Unlock();};

	inline void release(void)
		{Unlock();};
};

class __EXPORT Threadlock : public Exclusive, public Shared
{
private:
	pthread_rwlock_t lock;

public:
	Threadlock();
	~Threadlock();

	void Exlock(void);
	void Shlock(void);
	void Unlock(void);

	inline void shared(void)
		{return Shlock();};

	inline void exclusive(void)
		{return Exlock();};

	inline void release(void)
		{return Unlock();};
};

class __EXPORT auto_cancellation
{
private:
	int type, mode;

public:
	auto_cancellation(int type, int mode);
	~auto_cancellation();
};

#define	cancel_immediate()	auto_cancellation \
	__cancel__(PTHREAD_CANCEL_ASYNCHRONOUS, PTHREAD_CANCEL_ENABLE)

#define	cancel_deferred() auto_cancellation \
	__cancel__(PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_ENABLE)

#define	cancel_disabled() auto_cancellation \
	__cancel__(PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_DISABLE)

END_NAMESPACE

#endif
#endif
