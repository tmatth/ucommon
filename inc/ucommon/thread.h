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
	class __EXPORT attribute
	{
	public:
		pthread_condattr_t attr;
		attribute();
	};
	static attribute attr;

	pthread_cond_t cond;
	pthread_mutex_t mutex;

protected:
	inline void wait(void)
		{pthread_cond_wait(&cond, &mutex);};

	bool wait(timeout_t timeout);

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
};

class __EXPORT SharedLock : public Conditional
{
private:
	unsigned waits;
	unsigned reads;

public:
	SharedLock();

	void lock(void);
	void unlock(void);
	void access(void);
	void release(void);
	void protect(int *state);
	void release(int *state);

	inline void operator++()
		{access();};

	inline void operator--()
		{release();};
};	

class __EXPORT Barrier : public Conditional 
{
private:
	unsigned count;
	unsigned waits;

public:
	Barrier(unsigned count);
	~Barrier();

	void set(unsigned count);
	void wait(void);
};

class __EXPORT Semaphore : public Shared, public Conditional
{
private:
	unsigned count, waits, used;

public:
	Semaphore(unsigned limit = 0);

	inline void Shlock(void)
		{wait();};

	inline void Unlock(void)
		{release();};

	void wait(void);
	bool wait(timeout_t timeout);
	unsigned getCount(void);
	unsigned getUsed(void);
	void set(unsigned limit);
	void release(void);
};

class __EXPORT Event : public Conditional
{
private:
	unsigned count;
	bool signalled;

public:
	Event();

	void signal(void);
	void reset(void);

	bool wait(timeout_t timeout);
	void wait(void);
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
		{pthread_mutex_lock(&mutex);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};

	inline void release(void)
		{pthread_mutex_unlock(&mutex);};
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

class __EXPORT SharedPointer : public SharedLock
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

class __EXPORT Threadlock : public Exclusive, public Shared
{
private:
	pthread_rwlock_t lock;
	unsigned count;

public:
	Threadlock();
	~Threadlock();

	void Exlock(void);
	void Shlock(void);
	void Unlock(void);

	bool shared(void);
	bool shared(timeout_t timeout);

	bool exclusive(void);
	bool exclusive(timeout_t timeout);

	void release(void);
};

class __EXPORT Thread
{
protected:
	size_t stack;

	Thread(size_t stack = 0);

public:
	virtual void start(void) = 0;

	virtual void run(void) = 0;
	
	virtual ~Thread();

	virtual void release(void) = 0;
};

class __EXPORT CancelableThread : protected Thread
{
protected:
	pthread_t tid;

	volatile bool running;

	CancelableThread(size_t size = 0);
	virtual ~CancelableThread();
	void pause(void);
	void pause(timeout_t timeout);

public:
	inline bool isRunning(void)
		{return running;};

	inline bool isDetached(void)
		{return false;};

	void release(void);
	void start(void);
};

class __EXPORT DetachedThread : protected Thread
{
protected:
	DetachedThread(size_t size = 0);
	~DetachedThread();

	virtual void release(void);
	virtual void dealloc(void);
	void pause(void);
	void pause(timeout_t timeout);

public:
	void start(void);

	void stop(void);

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
	void pause(void);
	void pause(timeout_t timeout);
	void wait(void);
	bool wait(timeout_t timeout);
	void release(void);

public:
	bool signal(void);
	bool wakeup(unsigned limit = 1);
	void start(void);
	void start(unsigned count);
};
	
class __EXPORT Queue : protected OrderedIndex, protected Conditional
{
private:
	mempager *pager;
	LinkedObject *freelist;
	size_t used;

	class __EXPORT member : public OrderedObject
	{
	public:
		member(Queue *q, Object *obj);
		Object *object;
	};

protected:
	size_t limit;

public:
	Queue(mempager *mem);

	bool remove(Object *obj);
	bool post(Object *obj, timeout_t timeout = 0);
	Object *fifo(timeout_t timeout = 0);
	Object *lifo(timeout_t timeout = 0);
	size_t getCount(void);
};

class __EXPORT Stack : protected Conditional, protected mempager
{
private:
	LinkedObject *freelist, *usedlist;
	mempager *pager;
	size_t used;

	class __EXPORT member : public LinkedObject
	{
	public:
		member(Stack *s, Object *obj);
		Object *object;
	};

	friend class member;

protected:
	size_t limit;

public:
	Stack(mempager *pager);

	bool remove(Object *obj);
	bool push(Object *obj, timeout_t timeout = 0);
	Object *pull(timeout_t timeout = 0);
	size_t getCount(void);
};

class __EXPORT Buffer : public Conditional
{
private:
	size_t size, used, objsize;
	char *buf, *head, *tail;

protected:
	virtual size_t onPull(void *buf);
	virtual size_t onPeek(void *buf);
	virtual size_t onWait(void *buf);
	virtual size_t onPost(void *buf);

public:
	static const size_t timeout;

	Buffer(size_t capacity, size_t osize = 0);
	virtual ~Buffer();

	inline size_t getSize(void)
		{return size;};

	inline size_t getUsed(void)
		{return used;};

	size_t pull(void *buf, timeout_t timeout = 0);
	size_t wait(void *buf, timeout_t timeout = 0);
	size_t post(void *buf, timeout_t timeout = 0);
	size_t peek(void *buf);

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
	int state;

	shared_release();
	shared_release(const shared_release &copy);

public:
	shared_release(SharedPointer &p);
	~shared_release();

	void release(void);

	SharedObject *get(void);

	shared_release &operator=(SharedPointer &p);
};

class __EXPORT cancel_state
{
private:
	int state, type;

public:
	cancel_state();
	~cancel_state();

	void async(void);
	void disable(void);
	void release(void);
};

inline void cancel_async(cancel_state &cancel)
	{cancel.async();};

inline void cancel_disable(cancel_state &cancel)
	{cancel.disable();};

inline void cancel_release(cancel_state &cancel)
	{cancel.release();};

template<class T, mempager *P = NULL, size_t L = 0>
class queue : public Queue
{
public:
	inline queue() : Queue(P) {limit = L;};

	inline bool remove(T *obj)
		{return Queue::remove(obj);};	

	inline bool post(T *obj, timeout_t timeout = 0)
		{return Queue::post(obj);};

	inline T *fifo(timeout_t timeout = 0)
		{return static_cast<T *>(Queue::fifo(timeout));};

    inline T *lifo(timeout_t timeout = 0)
        {return static_cast<T *>(Queue::lifo(timeout));};
};

template<class T, mempager *P = NULL, size_t L = 0>
class stack : public Stack
{
public:
	inline stack() : Stack(P) {limit = L;};

	inline bool remove(T *obj)
		{return Stack::remove(obj);};	

	inline bool push(T *obj, timeout_t timeout = 0)
		{return Stack::push(obj);};

	inline T *pull(timeout_t timeout = 0)
		{return static_cast<T *>(Stack::pull(timeout));};
};

template<class T>
class buffer : public Buffer
{
public:
	inline buffer(size_t size) : Buffer(size, sizeof(T)) {};

	inline size_t wait(T *obj, timeout_t timeout = 0)
		{return Buffer::wait(obj, timeout);};

	inline size_t pull(T *obj, timeout_t timeout = 0)
		{return Buffer::pull(obj, timeout);};

	inline size_t post(T *obj, timeout_t timeout = 0)
		{return Buffer::post(obj, timeout);};

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

inline void start(Thread *th)
	{th->start();};

inline void cancel(CancelableThread *th)
	{th->release();};

END_NAMESPACE

#endif
