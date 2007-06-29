// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

#ifndef _UCOMMON_THREAD_H_
#define	_UCOMMON_THREAD_H_

#ifndef _UCOMMON_ACCESS_H_
#include <ucommon/access.h>
#endif

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef _UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

NAMESPACE_UCOMMON

class SharedPointer;

class __EXPORT Conditional 
{
private:
	class __LOCAL attribute
	{
	public:
		pthread_condattr_t attr;
		attribute();
	};

	__LOCAL static attribute attr;

	pthread_cond_t cond;
	pthread_mutex_t mutex;

protected:
	inline void wait(void)
		{pthread_cond_wait(&cond, &mutex);};

	bool wait(timeout_t expires);

	inline void lock(void)
		{pthread_mutex_lock(&mutex);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};

	inline void signal(void)
		{pthread_cond_signal(&cond);};

	inline void broadcast(void)
		{pthread_cond_broadcast(&cond);};

	Conditional();
	~Conditional();

public:
	static inline pthread_condattr_t *initializer(void)
		{return &attr.attr;};

};

class __EXPORT rwlock : private Conditional, public Exclusive, public Shared
{
private:
	unsigned waiting;
	unsigned reading;
	bool writers;

	__LOCAL void Exlock(void);
	__LOCAL void Shlock(void);
	__LOCAL void Unlock(void);

public:
	rwlock();

	bool exclusive(timeout_t timeout = Timer::inf);
	bool shared(timeout_t timeout = Timer::inf);
	void release(void);

	inline static bool exclusive(rwlock &lock, timeout_t timeout = Timer::inf)
		{return lock.exclusive(timeout);};

	inline static bool shared(rwlock &lock, timeout_t timeout = Timer::inf)
		{return lock.shared(timeout);};

	inline static void release(rwlock &lock)
		{lock.release();};
};

class __EXPORT ReusableAllocator : public Conditional
{
protected:
	ReusableObject *freelist;
	unsigned waiting;

	ReusableAllocator();

	inline ReusableObject *next(ReusableObject *obj)
		{return obj->getNext();};
	
	void release(ReusableObject *obj);
};

class __EXPORT ConditionalLock : public Conditional, public Shared
{
private:
	unsigned waits;
	volatile unsigned reads;

	__LOCAL void Shlock(void);
	__LOCAL void Unlock(void);

public:
	ConditionalLock();

	void exclusive(void);
	void shared(void);
	void release(void);

	inline void operator++()
		{shared();};

	inline void operator--()
		{release();};

	inline static void exclusive(ConditionalLock &s)
		{s.exclusive();};

	inline static void release(ConditionalLock &s)
		{s.release();};

	inline static void shared(ConditionalLock &s)
		{s.shared();};
};	

class __EXPORT barrier : private Conditional 
{
private:
	unsigned count;
	unsigned waits;

public:
	barrier(unsigned count);
	~barrier();

	void set(unsigned count);
	void wait(void);

	inline static void wait(barrier &b)
		{b.wait();};

	inline static void set(barrier &b, unsigned count)
		{b.set(count);};
};

class __EXPORT semaphore : public Shared, private Conditional
{
private:
	unsigned count, waits, used;

	__LOCAL void Shlock(void);
	__LOCAL void Unlock(void);

public:
	semaphore(unsigned limit = 0);

	void request(unsigned size);
	bool request(unsigned size, timeout_t timeout);
	void wait(void);
	bool wait(timeout_t timeout);
	unsigned getCount(void);
	unsigned getUsed(void);
	void set(unsigned limit);
	void release(void);
	void release(unsigned size);

	inline static void wait(semaphore &s)
		{s.wait();};

	inline static bool wait(semaphore &s, timeout_t timeout)
		{return s.wait(timeout);};

	inline static void release(semaphore &s)
		{s.release();};
};

class __EXPORT Completion : private Conditional
{
private:
	unsigned count;
	bool signalled;

public:
	Completion();

	unsigned request(void);
	bool accepts(unsigned id);
	bool wait(timeout_t timeout);
	bool wait(void);

	inline void release(void)
		{unlock();};

	inline static unsigned request(Completion &e)
		{return e.request();};

	inline static bool accepts(Completion &e, unsigned id)
		{return e.accepts(id);};

	inline static void release(Completion &e)
		{e.unlock();};

	inline static bool wait(Completion &e, timeout_t timeout)
		{return e.wait(timeout);};

	inline static bool wait(Completion &e)
		{return e.wait();};
};

class __EXPORT mutex : public Exclusive
{
private:
	pthread_mutex_t mlock;

	__LOCAL void Exlock(void);
	__LOCAL void Unlock(void);
		
public:
	mutex();
	~mutex();

	inline void acquire(void)
		{pthread_mutex_lock(&mlock);};

	inline void lock(void)
		{pthread_mutex_lock(&mlock);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mlock);};

	inline void release(void)
		{pthread_mutex_unlock(&mlock);};

	inline static void acquire(mutex &m)
		{pthread_mutex_lock(&m.mlock);};

	inline static void lock(mutex &m)
		{pthread_mutex_lock(&m.mlock);};

	inline static void unlock(mutex &m)
		{pthread_mutex_unlock(&m.mlock);};

	inline static void release(mutex &m)
		{pthread_mutex_unlock(&m.mlock);};

	inline static void acquire(pthread_mutex_t *lock)
		{pthread_mutex_lock(lock);};

	inline static void lock(pthread_mutex_t *lock)
		{pthread_mutex_lock(lock);};

	inline static void unlock(pthread_mutex_t *lock)
		{pthread_mutex_unlock(lock);};

	inline static void release(pthread_mutex_t *lock)
		{pthread_mutex_unlock(lock);};
};

class __EXPORT recursive_mutex : public Exclusive
{
private:
	class __LOCAL attribute
	{
	public:
		pthread_mutexattr_t attr;
		attribute();
	};

	__LOCAL static attribute attr;

	pthread_mutex_t mutex;

	__LOCAL void Exlock(void);
	__LOCAL void Unlock(void);
		
public:
	recursive_mutex();
	~recursive_mutex();

	inline void acquire(void)
		{pthread_mutex_lock(&mutex);};

	inline void lock(void)
		{pthread_mutex_lock(&mutex);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};

	inline void release(void)
		{pthread_mutex_unlock(&mutex);};

	inline static pthread_mutexattr_t *initializer(void)
		{return &attr.attr;};

	inline static void acquire(recursive_mutex &m)
		{pthread_mutex_lock(&m.mutex);};

	inline static void lock(recursive_mutex &m)
		{pthread_mutex_lock(&m.mutex);};

	inline static void unlock(recursive_mutex &m)
		{pthread_mutex_unlock(&m.mutex);};

	inline static void release(recursive_mutex &m)
		{pthread_mutex_unlock(&m.mutex);};

};

class __EXPORT ConditionalIndex : public OrderedIndex, public Conditional
{
public:
	ConditionalIndex();

protected:
	void lock_index(void);
	void unlock_index(void);
};

class __EXPORT LockedIndex : public OrderedIndex
{
public:
	LockedIndex();

private:
	pthread_mutex_t mutex;

protected:
	void lock_index(void);
	void unlock_index(void);
};

class __EXPORT LockedPointer
{
private:
	friend class locked_release;
	pthread_mutex_t mutex;
	Object *pointer;

protected:
	LockedPointer();

	void replace(Object *ptr);
	Object *dup(void);

	LockedPointer &operator=(Object *o);
};

class __EXPORT SharedObject
{
protected:
	friend class SharedPointer;
	
	virtual void commit(SharedPointer *pointer);

public:
	virtual ~SharedObject();
};

class __EXPORT SharedPointer : public ConditionalLock
{
private:
	friend class shared_release;
	SharedObject *pointer;

protected:
	SharedPointer();
	~SharedPointer();

	void replace(SharedObject *ptr);
	SharedObject *share(void);
};

class __EXPORT Thread
{
protected:
	pthread_t tid;
	size_t stack;

	Thread(size_t stack = 0);

public:
	typedef struct {int state; int type;} cancellation;

	inline static void check(void)
		{pthread_testcancel();};

	static void yield(void);

	static void sleep(timeout_t timeout);

	virtual void run(void) = 0;
	
	virtual ~Thread();

	virtual void exit(void) = 0;

	static void cancel_suspend(cancellation *cancel);
	static void cancel_resume(cancellation *cancel);
	static void cancel_async(cancellation *cancel);
};

class __EXPORT JoinableThread : protected Thread
{
private:
	volatile bool running;

protected:
	JoinableThread(size_t size = 0);
	virtual ~JoinableThread();
	void exit(void);

public:
	inline bool isRunning(void)
		{return running;};

	inline bool isDetached(void)
		{return false;};

	bool cancel(void);
	void start(void);
};

class __EXPORT DetachedThread : protected Thread
{
protected:
	DetachedThread(size_t size = 0);
	~DetachedThread();

	virtual void exit(void);

public:
	void start(void);

	bool cancel(void);

	inline bool isDetached(void)
		{return true;};

	inline bool isRunning(void)
		{return true;};
};

class __EXPORT PooledThread : public DetachedThread, protected Conditional
{
protected:
	unsigned poolsize, poolused, waits;

	PooledThread(size_t stack = 0);
	void suspend(void);
	bool suspend(timeout_t timeout);
	void sync(void);
	
	void exit(void);
	bool cancel(void);

public:
	unsigned wakeup(unsigned limit = 1);
	void start(void);
	void start(unsigned count);
};
	
class __EXPORT queue : protected OrderedIndex, protected Conditional
{
private:
	mempager *pager;
	LinkedObject *freelist;
	size_t used;

	class __LOCAL member : public OrderedObject
	{
	public:
		member(queue *q, Object *obj);
		Object *object;
	};

protected:
	size_t limit;

public:
	queue(mempager *mem, size_t size);

	bool remove(Object *obj);
	bool post(Object *obj, timeout_t timeout = 0);
	Object *fifo(timeout_t timeout = 0);
	Object *lifo(timeout_t timeout = 0);
	size_t getCount(void);

	static bool remove(queue &q, Object *obj)
		{return q.remove(obj);};

	static bool post(queue &q, Object *obj, timeout_t timeout = 0)
		{return q.post(obj, timeout);};

	static Object *fifo(queue &q, timeout_t timeout = 0)
		{return q.fifo(timeout);};

	static Object *lifo(queue &q, timeout_t timeout = 0)
		{return q.lifo(timeout);};

	static size_t count(queue &q)
		{return q.getCount();};
};

class __EXPORT stack : protected Conditional
{
private:
	LinkedObject *freelist, *usedlist;
	mempager *pager;
	size_t used;

	class __LOCAL member : public LinkedObject
	{
	public:
		member(stack *s, Object *obj);
		Object *object;
	};

	friend class member;

protected:
	size_t limit;

public:
	stack(mempager *pager, size_t size);

	bool remove(Object *obj);
	bool push(Object *obj, timeout_t timeout = 0);
	Object *pull(timeout_t timeout = 0);
	size_t getCount(void);

	static inline bool remove(stack &stack, Object *obj)
		{return stack.remove(obj);};

	static inline bool push(stack &stack, Object *obj, timeout_t timeout = 0)
		{return stack.push(obj, timeout);};

	static inline Object *pull(stack &stack, timeout_t timeout = 0)
		{return stack.pull(timeout);};  

	static inline size_t count(stack &stack)
		{return stack.getCount();};
};

class __EXPORT Buffer : public Conditional
{
private:
	size_t size, objsize;
	caddr_t buf, head, tail;
	unsigned count, limit;

public:
	Buffer(size_t objsize, size_t count);
	virtual ~Buffer();

	unsigned getSize(void);
	unsigned getCount(void);

	void *get(timeout_t timeout);
	void *get(void);
	void put(void *data);
	bool put(void *data, timeout_t timeout);
	void release(void);	// release lock from get

	operator bool();

	virtual bool operator!();
};

class __EXPORT locked_release
{
protected:
	Object *object;

	locked_release();
	locked_release(const locked_release &copy);

public:
	locked_release(LockedPointer &p);
	~locked_release();

	void release(void);

	locked_release &operator=(LockedPointer &p);
};

class __EXPORT shared_release
{
protected:
	SharedPointer *ptr;

	shared_release();
	shared_release(const shared_release &copy);

public:
	shared_release(SharedPointer &p);
	~shared_release();

	void release(void);

	SharedObject *get(void);

	shared_release &operator=(SharedPointer &p);
};

template<class T, mempager *P = NULL, size_t L = 0>
class queueof : public queue
{
public:
	inline queueof() : queue(P, L) {};

	inline bool remove(T *obj)
		{return queue::remove(obj);};	

	inline bool post(T *obj, timeout_t timeout = 0)
		{return queue::post(obj);};

	inline T *fifo(timeout_t timeout = 0)
		{return static_cast<T *>(queue::fifo(timeout));};

    inline T *lifo(timeout_t timeout = 0)
        {return static_cast<T *>(queue::lifo(timeout));};
};

template<class T, mempager *P = NULL, size_t L = 0>
class stackof : public stack
{
public:
	inline stackof() : stack(P, L) {};

	inline bool remove(T *obj)
		{return stack::remove(obj);};	

	inline bool push(T *obj, timeout_t timeout = 0)
		{return stack::push(obj);};

	inline T *pull(timeout_t timeout = 0)
		{return static_cast<T *>(stack::pull(timeout));};
};

template<class T>
class bufferof : public Buffer
{
public:
	inline bufferof(unsigned count) :
		Buffer(sizeof(T), count) {};

	inline T *get(void)
		{return static_cast<T*>(get());};

	inline T *get(timeout_t timeout)
		{return static_cast<T*>(get(timeout));};

	inline void put(T *obj)
		{put(obj);};

	inline bool put(T *obj, timeout_t timeout)
		{return put(obj, timeout);};
};
 
template<class T>
class shared_pointer : public SharedPointer
{
public:
	inline shared_pointer() : SharedPointer() {};

	inline shared_pointer(void *p) : SharedPointer(p) {};

	inline const T *dup(void)
		{return static_cast<const T*>(SharedPointer::share());};

	inline void replace(T *p)
		{SharedPointer::replace(p);};
};	

template<class T>
class locked_pointer : public LockedPointer
{
public:
	inline locked_pointer() : LockedPointer() {};

	inline T* dup(void)
		{return static_cast<T *>(LockedPointer::dup());};

	inline void replace(T *p)
		{LockedPointer::replace(p);};

	inline locked_pointer<T>& operator=(T *obj)
		{LockedPointer::operator=(obj); return *this;};

	inline T *operator*()
		{return dup();};
};

template<class T>
class locked_instance : public locked_release
{
public:
    inline locked_instance() : locked_release() {};

    inline locked_instance(locked_pointer<T> &p) : locked_release(p) {};

    inline T& operator*() const
        {return *(static_cast<T *>(object));};

    inline T* operator->() const
        {return static_cast<T*>(object);};

    inline T* get(void) const
        {return static_cast<T*>(object);};
};

template<class T>
class shared_instance : public shared_release
{
public:
	inline shared_instance() : shared_release() {};

	inline shared_instance(shared_pointer<T> &p) : shared_release(p) {};

	inline const T& operator*() const
		{return *(static_cast<const T *>(ptr->pointer));};

	inline const T* operator->() const
		{return static_cast<const T*>(ptr->pointer);};

	inline const T* get(void) const
		{return static_cast<const T*>(ptr->pointer);};
};

inline void start(JoinableThread *th)
	{th->start();};

inline bool cancel(JoinableThread *th)
	{return th->cancel();};

inline void start(DetachedThread *th)
    {th->start();};

inline bool cancel(DetachedThread *th)
	{return th->cancel();};

typedef	mutex mutex_t;
typedef rwlock rwlock_t;
typedef semaphore semaphore_t;
typedef recursive_mutex recursive_mutex_t;
typedef	barrier barrier_t;
typedef stack stack_t;
typedef	queue queue_t;

inline void wait(barrier_t &b)
	{b.wait();};

inline void wait(semaphore_t &s, timeout_t timeout = Timer::inf)
	{s.wait(timeout);};

inline void acquire(recursive_mutex_t &rm)
	{rm.lock();};

inline void acquire(mutex_t &ml)
	{ml.lock();};

inline void release(recursive_mutex_t &rm)
	{rm.release();};

inline void release(mutex_t &ml)
	{ml.release();};

inline bool exclusive(rwlock_t &rw, timeout_t timeout = Timer::inf)
	{return rw.exclusive(timeout);};

inline bool shared(rwlock_t &rw, timeout_t timeout = Timer::inf)
	{return rw.shared(timeout);};

inline void release(rwlock_t &rw)
	{rw.release();};

inline void push(stack_t &s, Object *obj)
	{s.push(obj);};

inline Object *pull(stack_t &s, timeout_t timeout = Timer::inf)
	{return s.pull(timeout);};

inline void remove(stack_t &s, Object *obj)
	{s.remove(obj);};

inline void push(queue_t &s, Object *obj)
	{s.post(obj);};

inline Object *pull(queue_t &s, timeout_t timeout = Timer::inf)
	{return s.fifo(timeout);};

inline void remove(queue_t &s, Object *obj)
	{s.remove(obj);};

END_NAMESPACE

#define	ENTER_EXCLUSIVE	\
	do { static pthread_mutex_t __sync__ = PTHREAD_MUTEX_INITIALIZER; \
		pthread_mutex_lock(&__sync__);

#define EXIT_EXCLUSIVE \
	pthread_mutex_unlock(&__sync__);} while(0);

#define	SUSPEND_CANCELLATION \
	do { Thread::cancellation __cancel__; Thread::cancel_suspend(&__cancel__);

#define ASYNC_CANCELLATION \
	do { Thread::cancellation __cancel__; Thread::cancel_async(&__cancel__);

#define	RESUME_CANCELLATION \
	Thread::cancel_resume(&__cancel__);} while(0);

#endif
