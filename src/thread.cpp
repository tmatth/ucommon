#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/timers.h>
#include <ucommon/linked.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

using namespace UCOMMON_NAMESPACE;

static void gettimeout(timeout_t msec, struct timespec *ts)
{
#if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(_POSIX_MONOTONIC_CLOCK)
	clock_gettime(CLOCK_MONOTONIC, ts);
#elif _POSIX_TIMERS > 0
	clock_gettime(CLOCK_REALTIME, ts);
#else
	timeval tv;
	gettimeofday(&tv, NULL);
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000l;
#endif
	ts->tv_sec += msec / 1000;
	ts->tv_nsec += (msec % 1000) * 1000000l;
	while(ts->tv_nsec > 1000000000l) {
		++ts->tv_sec;
		ts->tv_nsec -= 1000000000l;
	}
}

Mutex::attribute Mutex::attr;
Conditional::attribute Conditional::attr;

Mutex::attribute::attribute()
{
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
}

ReusableAllocator::ReusableAllocator() :
Conditional()
{
	freelist = NULL;
	waiting = 0;
}

void ReusableAllocator::release(ReusableObject *obj)
{
	LinkedObject **ru = (LinkedObject **)freelist;

	obj->setNil();
	obj->release();

	lock();
	obj->enlist(ru);
	if(waiting)
		signal();
	unlock();
}

Event::Event() : 
Conditional()
{
	count = 0;
	signalled = false;
}

void Event::signal(void)
{
	lock();
    signalled = true;
    ++count;
	broadcast();
	unlock();
}

void Event::reset(void)
{
	lock();
    signalled = true;
	unlock();
}

bool Event::wait(timeout_t timeout)
{
	bool notimeout = true;
	Timer expires;
	unsigned last;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	last = count;
	while(!signalled && count == last && notimeout) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)		
			notimeout = Conditional::wait(*expires);
		else
			notimeout = false;
	}
	unlock();
	return notimeout;
}

void Event::wait(void)
{
	lock();
	unsigned last = count;
	while(!signalled && count == last)
		Conditional::wait();
	unlock();
}

Semaphore::Semaphore(unsigned limit) : 
Conditional()
{
	count = limit;
	waits = 0;
	used = 0;
}

unsigned Semaphore::getUsed(void)
{
	unsigned rtn;
	lock();
	rtn = used;
	unlock();
	return rtn;
}

unsigned Semaphore::getCount(void)
{
    unsigned rtn;
	lock();
    rtn = count;
	unlock();
    return rtn;
}

bool Semaphore::wait(timeout_t timeout)
{
	bool result = true;
	Timer expires;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	if(used >= count) {
		++waits;
		while(used >= count && result) {
			if(timeout == Timer::inf)
				Conditional::wait();
			else if(timeout)
				result = Conditional::wait(*expires);
			else
				result = false;
		}
		--waits;
	}
	if(result)
		++used;
	unlock();
	return result;
}
		
void Semaphore::wait(void)
{
	lock();
	if(used >= count) {
		++waits;
		while(used >= count)
			Conditional::wait();
		--waits;
	}
	++used;
	unlock();
}

void Semaphore::release(void)
{
	lock();
	if(used)
		--used;
	if(waits)
		signal();
	unlock();
}

void Semaphore::set(unsigned value)
{
	unsigned diff;

	lock();
	count = value;
	if(used >= count || !waits) {
		unlock();
		return;
	}
	diff = count - used;
	if(diff > waits)
		diff = waits;
	unlock();
	while(diff--) {
		lock();
		signal();
		unlock();
	}
}

Conditional::attribute::attribute()
{
	pthread_condattr_init(&attr);
#if _POSIX_TIMERS > 0 && defined(HAVE_PTHREAD_CONDATTR_SETCLOCK)
#if defined(_POSIX_MONOTONIC_CLOCK)
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#else
	pthread_condattr_setclock(&attr, CLOCK_REALTIME);
#endif
#endif
}

Conditional::Conditional()
{
	crit(pthread_cond_init(&cond, &attr.attr) == 0);
	crit(pthread_mutex_init(&mutex, NULL) == 0);
}

Conditional::~Conditional()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

bool Conditional::wait(timeout_t timeout)
{
	struct timespec ts;

	if(!timeout)
		return false;

	gettimeout(timeout, &ts);
	if(pthread_cond_timedwait(&cond, &mutex, &ts))
		return false;

	return true;
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

SharedLock::SharedLock() :
Conditional()
{
	waits = 0;
	reads = 0;
}

void SharedLock::lock(void)
{
	Conditional::lock();
	while(reads) {
		++waits;
		Conditional::wait();
		--waits;
	}
}

void SharedLock::unlock(void)
{
	Conditional::unlock();
}

void SharedLock::release(void)
{
	Conditional::lock();
	if(reads) {
		--reads;
		if(!reads && waits)
			Conditional::broadcast();
	}
	Conditional::unlock();
}

void SharedLock::access(void)
{
	Conditional::lock();
	++reads;
	Conditional::unlock();
}

Barrier::Barrier(unsigned limit) :
Conditional()
{
	count = limit;
	waits = 0;
}

Barrier::~Barrier()
{
	for(;;)
	{
		lock();
		if(waits)
			broadcast();
		unlock();
	}
}

void Barrier::set(unsigned limit)
{
	lock();
	count = limit;
	if(count <= waits) {
		waits = 0;
		broadcast();
	}
	unlock();
}

void Barrier::wait(void)
{
	Conditional::lock();
	if(!count) {
		Conditional::unlock();
		return;
	}
	if(++waits >= count) {
		waits = 0;
		Conditional::broadcast();
		Conditional::unlock();
		return;
	}
	Conditional::wait();
	Conditional::unlock();
}

ConditionalIndex::ConditionalIndex() :
OrderedIndex(), Conditional()
{
}

void ConditionalIndex::lock_index(void)
{
	Conditional::lock();
}

void ConditionalIndex::unlock_index(void)
{
	Conditional::unlock();
}

LockedIndex::LockedIndex() :
OrderedIndex(), Mutex()
{
}

void LockedIndex::lock_index(void)
{
	Mutex::lock();
}

void LockedIndex::unlock_index(void)
{
	Mutex::unlock();
}
	
LockedPointer::LockedPointer()
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&mutex, &lock, sizeof(mutex));
	pointer = NULL;
}

void LockedPointer::replace(Object *obj)
{
	pthread_mutex_lock(&mutex);
	obj->retain();
	if(pointer)
		pointer->release();
	pointer = obj;
	pthread_mutex_unlock(&mutex);
}

Object *LockedPointer::dup(void)
{
	Object *temp;
	pthread_mutex_lock(&mutex);
	temp = pointer;
	if(temp)
		temp->retain();
	pthread_mutex_unlock(&mutex);
	return temp;
}

SharedObject::~SharedObject()
{
}

SharedPointer::SharedPointer() :
SharedLock()
{
	pointer = NULL;
}

SharedPointer::~SharedPointer()
{
}

void SharedPointer::replace(SharedObject *ptr)
{
	SharedLock::lock();
	if(pointer)
		delete pointer;
	pointer = ptr;
	if(ptr)
		ptr->commit(this);
	SharedLock::unlock();
}

SharedObject *SharedPointer::share(void)
{
	SharedLock::access();
	return pointer;
}

Thread::Thread(size_t size)
{
	stack = size;
}

JoinableThread::JoinableThread(size_t size)
{
	running = false;
	stack = size;
}

DetachedThread::DetachedThread(size_t size)
{
	stack = size;
}

void Thread::pause(timeout_t timeout)
{
	int state;
	int mode;
	
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
	if(state == PTHREAD_CANCEL_ENABLE) {
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &mode);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		cpr_sleep(timeout);
		pthread_setcanceltype(mode, NULL);
	}
	else
		cpr_sleep(timeout);
}

void Thread::yield(void)
{
#ifdef	HAVE_PTHREAD_YIELD
	pthread_yield();
#else
	sched_yield();
#endif
	pthread_testcancel();
}

void PooledThread::exit(void)
{
	--poolused;
	if(!poolused)
		delete this;
}

void DetachedThread::exit(void)
{
	delete this;
}

bool DetachedThread::cancel(void)
{
	pthread_t self = pthread_self();

	if(pthread_equal(tid, self)) {
		pthread_testcancel();
	}
	else if(!pthread_cancel(tid)) {
		exit();
		return true;
	}
	return false;
}

bool PooledThread::cancel(void)
{
	return false;
}

void PooledThread::sync(void)
{
	lock();
	if(poolused < 2) {
		unlock();
		return;
	}
	if(++waits == poolused) {
		Conditional::broadcast();
		waits = 0;
	}
	else
		Conditional::wait();
	unlock();
}

bool PooledThread::suspend(timeout_t timeout)
{
	bool result;

    lock();
    ++waits;
	result = Conditional::wait(timeout);
    --waits;
    unlock();
	return result;
}

void PooledThread::suspend(void)
{
	lock();
	++waits;
	Conditional::wait();
	--waits;
	unlock();
}

unsigned PooledThread::wakeup(unsigned limit)
{
	unsigned waiting = 0;

	lock();
	while(waits >= limit) {
		++waiting;
		Conditional::signal();
		unlock();
		yield();
		lock();
	}
	unlock();
	return waiting;
}

PooledThread::PooledThread(size_t size) :
DetachedThread(size), Conditional()
{
	waits = 0;
	poolsize = 0;
	poolused = 0;
}

Thread::~Thread()
{
}

JoinableThread::~JoinableThread()
{
	cancel();
}

DetachedThread::~DetachedThread()
{
}
		
extern "C" {
	static void *exec_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);
		th->run();
		th->exit();
		return NULL;
	};
}

void JoinableThread::start(void)
{
	pthread_attr_t attr;
	if(running)
		return;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 
	pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
// we typically use "stack 1" for min stack...
#ifdef	PTHREAD_STACK_MIN
	if(stack && stack < PTHREAD_STACK_MIN)
		stack = PTHREAD_STACK_MIN;
#else
	if(stack && stack < 2)
		stack = 0;
#endif
	if(stack)
		pthread_attr_setstacksize(&attr, stack);
	pthread_create(&tid, &attr, &exec_thread, this);
	pthread_attr_destroy(&attr);
	running = true;
}

void PooledThread::start(void)
{
	lock();
	++poolsize;
	++poolused;
	DetachedThread::start();
	unlock();
}

void PooledThread::start(unsigned count)
{
	lock();
	poolsize = count;
	while(poolused < poolsize) {
		DetachedThread::start();
		++poolused;
	}
	unlock();
}

void DetachedThread::start(void)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
// we typically use "stack 1" for min stack...
#ifdef	PTHREAD_STACK_MIN
	if(stack && stack < PTHREAD_STACK_MIN)
		stack = PTHREAD_STACK_MIN;
#else
	if(stack && stack < 2)
		stack = 0;
#endif
	if(stack)
		pthread_attr_setstacksize(&attr, stack);
	pthread_create(&tid, &attr, &exec_thread, this);
	pthread_attr_destroy(&attr);
}

void JoinableThread::exit(void)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	for(;;) {
		pause(Timer::inf);
		pthread_testcancel();
	}
}

bool JoinableThread::cancel(void)
{
	pthread_t self = pthread_self();

	if(running && pthread_equal(tid, self)) {
		pthread_testcancel();
		return false;
	}

	if(running) {
		pthread_cancel(tid);
		if(!pthread_join(tid, NULL)) 
			running = false;
	}	
	if(running)
		return false;
	return true;
}

Queue::member::member(Queue *q, Object *o) :
OrderedObject(q)
{
	o->retain();
	object = o;
}

Queue::Queue(mempager *p) :
OrderedIndex(), Conditional()
{
	pager = p;
	freelist = NULL;
	limit = used = 0;
}

bool Queue::remove(Object *o)
{
	bool rtn = false;
	linked_pointer<member> node;
	lock();
	node = begin();
	while(node) {
		if(node->object == o)
			break;
		node.next();
	}
	if(node) {
		--used;
		rtn = true;
		node->object->release();
		node->delist(this);
		node->LinkedObject::enlist(&freelist);			
	}
	unlock();
	return rtn;
}

Object *Queue::lifo(timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
    member *member;
    Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(!tail && rtn) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(*expires);
		else
			rtn = false;
	}
    if(rtn && tail) {
        --used;
        member = static_cast<Queue::member *>(head);
        obj = member->object;
		member->delist(this);
        member->LinkedObject::enlist(&freelist);
    }
	if(rtn)
		signal();
	unlock();
    return obj;
}

Object *Queue::fifo(timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
	linked_pointer<member> node;
	Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && !head) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(*expires);
		else
			rtn = false;
	}

	if(rtn && head) {
		--used;
		node = begin();
		obj = node->object;
		head = head->getNext();
		if(!head)
			tail = NULL;
		node->LinkedObject::enlist(&freelist);
	}
	if(rtn)
		signal();
	unlock();
	return obj;
}

bool Queue::post(Object *object, timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
	member *node;
	LinkedObject *mem;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && limit && used == limit) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(*expires);
		else
			rtn = false;
	}

	if(!rtn) {
		unlock();
		return false;
	}

	++used;
	if(freelist) {
		mem = freelist;
		freelist = freelist->getNext();
		unlock();
		node = new((caddr_t)mem) member(this, object);
	}
	else {
		unlock();
		if(pager)
			node = new((caddr_t)(pager->alloc(sizeof(member)))) member(this, object);
		else
			node = new member(this, object);
	}
	lock();
	signal();
	unlock();
	return true;
}

size_t Queue::getCount(void)
{
	size_t count;
	lock();
	count = used;
	unlock();
	return count;
}


Stack::member::member(Stack *S, Object *o) :
LinkedObject((&S->usedlist))
{
	o->retain();
	object = o;
}

Stack::Stack(mempager *p) :
Conditional()
{
	pager = p;
	freelist = usedlist = NULL;
	limit = used = 0;
}

bool Stack::remove(Object *o)
{
	bool rtn = false;
	linked_pointer<member> node;
	lock();
	node = static_cast<member*>(usedlist);
	while(node) {
		if(node->object == o)
			break;
		node.next();
	}
	if(node) {
		--used;
		rtn = true;
		node->object->release();
		node->delist(&usedlist);
		node->enlist(&freelist);			
	}
	unlock();
	return rtn;
}

Object *Stack::pull(timeout_t timeout)
{
	bool rtn = true;
	Timer expires;	
    member *member;
    Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && !usedlist) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(*expires);
		else
			rtn = false;
	} 
	if(!rtn) {
		unlock();
		return NULL;
	}
    if(usedlist) {
        member = static_cast<Stack::member *>(usedlist);
        obj = member->object;
		usedlist = member->getNext();
        member->enlist(&freelist);
	}
	if(rtn)
		signal();
	unlock();
    return obj;
}

bool Stack::push(Object *object, timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
	member *node;
	LinkedObject *mem;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && limit && used == limit) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(*expires);
		else
			rtn = false;
	}

	if(!rtn) {
		unlock();
		return false;
	}

	++used;
	if(freelist) {
		mem = freelist;
		freelist = freelist->getNext();
		unlock();
		node = new((caddr_t)mem) member(this, object);
	}
	else {
		unlock();
		if(pager) {
			caddr_t ptr = (caddr_t)pager->alloc(sizeof(member));
			node = new(ptr) member(this, object);
		}
		else
			node = new member(this, object);
	}
	lock();
	signal();
	unlock();
	return true;
}

size_t Stack::getCount(void)
{
	size_t count;
	lock();
	count = used;
	unlock();
	return count;
}

Buffer::Buffer(size_t osize, unsigned c) : 
Conditional()
{
	size = osize * count;
	objsize = osize;
	count = 0;
	limit = c;

	if(osize) {
		buf = (char *)malloc(size);
		crit(buf != NULL);
	}
	else 
		buf = NULL;

	head = tail = buf;
}

Buffer::~Buffer()
{
	if(buf)
		free(buf);
	buf = NULL;
}

unsigned Buffer::getCount(void)
{
	unsigned count = 0;

	lock();
	if(tail > head) 
		count = (unsigned)((size_t)(tail - head) / objsize);
	else if(head > tail)
		count = (unsigned)((((buf + size) - head) + (tail - buf)) / objsize);
	unlock();
	return count;
}

unsigned Buffer::getSize(void)
{
	return size / objsize;
}

void *Buffer::get(void)
{
	caddr_t dbuf;

	lock();
	while(!count)
		wait();
	dbuf = head;
	unlock();
	return dbuf;
}

void *Buffer::get(timeout_t timeout)
{
	caddr_t dbuf = NULL;
	Timer expires;
	bool rtn = true;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(!count && rtn) {
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
	}
	if(count && rtn)
		dbuf = head;
	unlock();
	return dbuf;
}

void Buffer::release(void)
{
	lock();
	head += objsize;
	if(head >= buf + size)
		head = buf;
	--count;
	signal();
	unlock();
}

void Buffer::put(void *dbuf)
{
	lock();
	while(count == limit)
		wait();
	memcpy(tail, dbuf, objsize);
	tail += objsize;
	if(tail >= (buf + size))
		tail = 0;
	++count;
	signal();
	unlock();
}

bool Buffer::put(void *dbuf, timeout_t timeout)
{
	bool rtn = true;
	Timer expires;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(count == limit && rtn) {
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
	}
	if(rtn && count < limit) {
		memcpy(tail, dbuf, objsize);
		tail += objsize;
		if(tail >= (buf + size))
			tail = 0;
		++count;
		signal();
	}
	unlock();
	return rtn;
}


Buffer::operator bool()
{
	bool rtn = false;

	lock();
	if(buf && head != tail)
		rtn = true;
	unlock();
	return rtn;
}

bool Buffer::operator!()
{
	bool rtn = false;

	lock();
	if(!buf || head == tail)
		rtn = true;
	unlock();
	return rtn;
}

locked_release::locked_release(const locked_release &copy)
{
	object = copy.object;
	object->retain();
}

locked_release::locked_release()
{
	object = NULL;
}

locked_release::locked_release(LockedPointer &p)
{
	object = p.dup();
}

locked_release::~locked_release()
{
	release();
}

void locked_release::release(void)
{
	if(object)
		object->release();
	object = NULL;
}

locked_release &locked_release::operator=(LockedPointer &p)
{
	release();
	object = p.dup();
	return *this;
}

shared_release::shared_release(const shared_release &copy)
{
	ptr = copy.ptr;
}

shared_release::shared_release()
{
	ptr = NULL;
}

SharedObject *shared_release::get(void)
{
	if(ptr)
		return ptr->pointer;
	return NULL;
}

void SharedObject::commit(SharedPointer *pointer)
{
}

shared_release::shared_release(SharedPointer &p)
{
	ptr = &p;
	p.share(); // create rdlock
}

shared_release::~shared_release()
{
	release();
}

void shared_release::release(void)
{
	if(ptr)
		ptr->release();
	ptr = NULL;
}

shared_release &shared_release::operator=(SharedPointer &p)
{
	release();
	ptr = &p;
	p.share();
	return *this;
}

void Thread::cancel_async(cancellation *cancel)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &cancel->type);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &cancel->state);
}

void Thread::cancel_suspend(cancellation *cancel)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel->state);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &cancel->type);
}

void Thread::cancel_resume(cancellation *cancel)
{
	pthread_setcanceltype(cancel->type, NULL);
	pthread_setcancelstate(cancel->state, NULL);
	if(cancel->state == PTHREAD_CANCEL_ENABLE)
		pthread_testcancel();
}


