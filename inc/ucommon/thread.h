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

	void request(unsigned size);
	bool request(unsigned size, timeout_t timeout);
	void wait(void);
	bool wait(timeout_t timeout);
	unsigned getCount(void);
	unsigned getUsed(void);
	void set(unsigned limit);
	void release(void);
	void release(unsigned size);
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
	class __LOCAL attribute
	{
	public:
		pthread_mutexattr_t attr;
		attribute();
	};

	__LOCAL static attribute attr;

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

	static inline pthread_mutexattr_t *initializer(void)
		{return &attr.attr;};
};

class __EXPORT ConditionalIndex : public OrderedIndex, public Conditional
{
public:
	ConditionalIndex();

private:
	void lock_index(void);
	void unlock_index(void);
};

class __EXPORT LockedIndex : public OrderedIndex, public Mutex
{
public:
	LockedIndex();

private:
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

class __EXPORT Thread
{
protected:
	pthread_t tid;
	size_t stack;

	Thread(size_t stack = 0);

public:
	typedef struct {int state; int type;} cancellation;

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
	
class __EXPORT Queue : protected OrderedIndex, protected Conditional
{
private:
	mempager *pager;
	LinkedObject *freelist;
	size_t used;

	class __LOCAL member : public OrderedObject
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

class __EXPORT Stack : protected Conditional
{
private:
	LinkedObject *freelist, *usedlist;
	mempager *pager;
	size_t used;

	class __LOCAL member : public LinkedObject
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
	inline buffer(unsigned count) :
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
