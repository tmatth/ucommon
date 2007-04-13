#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/process.h>
#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

const size_t Buffer::timeout = ((size_t)(-1));
Mutex::attribute Mutex::attr;
Conditional::attribute Conditional::attr;

Mutex::attribute::attribute()
{
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
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
	unsigned last;

	lock();
	last = count;
	while(!signalled && count == last && notimeout)
		notimeout = wait(timeout);
	unlock();
	return notimeout;
}

void Event::wait(void)
{
	lock();
	unsigned last = count;
	while(!signalled && count == last)
		wait();
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
	lock();
	if(used >= count) {
		++waits;
		while(used >= count && result)
			result = Conditional::wait(timeout);
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

	cpr_gettimeout(timeout, &ts);
	if(pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT)
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

Threadlock::Threadlock()
{
	crit(pthread_rwlock_init(&lock, NULL) == 0);
	count = 0;
}

Threadlock::~Threadlock()
{
	pthread_rwlock_destroy(&lock);
}

void Threadlock::Unlock(void)
{
	release();
}

void Threadlock::Exlock(void)
{
	while(!exclusive())
		cpr_yield();
}

void Threadlock::Shlock(void)
{
	while(!shared())
		cpr_yield();
}

void Threadlock::release(void)
{
	if(count) {
		--count;
		return;
	}
	pthread_rwlock_unlock(&lock);
}

bool Threadlock::exclusive(void)
{
	if(!pthread_rwlock_wrlock(&lock))
		return true;

	if(errno == EDEADLK) {
		++count;
		return true;
	}
	return false;
}

bool Threadlock::shared(void)
{
    if(!pthread_rwlock_rdlock(&lock))
        return true;

    if(errno == EDEADLK) {
        ++count;
        return true;
    }
    return false;
}

bool Threadlock::shared(timeout_t timeout)
{
    struct timespec ts;

#if _POSIX_TIMERS > 0
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (timeout % 1000) * 1000000l;
    ts.tv_sec += timeout / 1000l;
    while(ts.tv_nsec > 1000000000l) {
        ts.tv_nsec -= 1000000000l;
        ++ts.tv_sec;
	}
#else
    ts.tv_nsec = 0;
    time(&ts.tv_sec);
    ts.tv_sec += timeout / 1000l;
#endif
    if(!pthread_rwlock_timedrdlock(&lock, &ts))
        return true;
    if(errno == EDEADLK) {
        ++count;
        return true;
    }
    return false;
}

bool Threadlock::exclusive(timeout_t timeout)
{
    struct timespec ts;

#if _POSIX_TIMERS > 0
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += (timeout % 1000) * 1000000l;
	ts.tv_sec += timeout / 1000l;
	while(ts.tv_nsec > 1000000000l) {
		ts.tv_nsec -= 1000000000l;
		++ts.tv_sec;
	}	
#else
	ts.tv_nsec = 0;
	time(&ts.tv_sec);
	ts.tv_sec += timeout / 1000l;
#endif
	if(!pthread_rwlock_timedwrlock(&lock, &ts))
		return true;
	if(errno == EDEADLK) {
		++count;
		return true;
	}
	return false;
}

Thread::Thread(size_t size)
{
	stack = size;
}

CancelableThread::CancelableThread(size_t size)
{
	running = false;
	stack = size;
	cancel_state = PTHREAD_CANCEL_ENABLE;
	cancel_type = PTHREAD_CANCEL_DEFERRED;
}

void CancelableThread::release_cancel(void)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcanceltype(cancel_type, NULL);
	pthread_setcancelstate(cancel_state, NULL);
	if(cancel_state == PTHREAD_CANCEL_ENABLE)
		pthread_testcancel();
}

void CancelableThread::async_cancel(void)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &cancel_type);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &cancel_state);
}

void CancelableThread::disable_cancel(void)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel_state);
}

DetachedThread::DetachedThread(size_t size)
{
	stack = size;
}

void PooledThread::release(void)
{
	bool done = false;
	lock();
	--poolused;
	if(!poolused)
		done = true;
	unlock();
	if(done)
		dealloc();
	pthread_exit(NULL);
}

bool PooledThread::wait(timeout_t timeout)
{
    bool over = false;
	bool result;
    lock();
    ++waits;
    result = Conditional::wait(timeout);
    --waits;
    if(poolused > poolsize) {
        --poolused;
        over = true;
    }
    unlock();
    if(over) {
        dealloc();
        pthread_exit(NULL);
	}
	return result;
}

void PooledThread::wait(void)
{
	bool over = false;
	lock();
	++waits;
	Conditional::wait();
	--waits;
	if(poolused > poolsize) {
		--poolused;
		over = true;
	}
	unlock();
	if(over) {
		dealloc();
		pthread_exit(NULL);
	}
}

bool PooledThread::signal(void)
{
	bool waiting = true;

	lock();
	if(waits)
		Conditional::signal();
	else
		waiting = false;
	unlock();
	return waiting;
}

bool PooledThread::wakeup(unsigned limit)
{
	bool waiting = true;

	lock();
	if(waits >= limit)
		Conditional::broadcast();
	else
		waiting = false;
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

CancelableThread::~CancelableThread()
{
	release();
}

DetachedThread::~DetachedThread()
{
}
		
extern "C" {
	static void *exec_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);
		th->run();
		th->release();
		return NULL;
	};
}

void CancelableThread::pause(void)
{
	pthread_testcancel();
	cpr_yield();
}

void CancelableThread::pause(timeout_t timeout)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &cancel_type);
	cpr_sleep(timeout);
	pthread_setcanceltype(cancel_type, NULL);
}

void DetachedThread::pause(void)
{
	cpr_yield();
}

void DetachedThread::pause(timeout_t timeout)
{
	cpr_sleep(timeout);
}

void CancelableThread::start(void)
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
	pthread_t tid;

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

void PooledThread::pause(timeout_t timeout)
{
	bool over = false;
	cpr_sleep(timeout);
	lock();
    if(poolused > poolsize) {
        over = true;
		--poolused;
	}
	unlock();
    if(over) {
		dealloc();
		pthread_exit(NULL);
	}		
}

void PooledThread::pause(void)
{
	bool over = false;

	lock();
	if(poolused > poolsize) {
		over = true;
		--poolused;
	}
	unlock();
	if(over) {
		dealloc();
		pthread_exit(NULL);
	}	
	else
		cpr_yield();
}

void DetachedThread::dealloc(void)
{
	delete this;
}

void CancelableThread::release(void)
{
	pthread_t self = pthread_self();

	if(running && pthread_equal(tid, self)) {
		running = false;
		cpr_yield();
		pthread_testcancel();
		pthread_exit(NULL);
	}

	cpr_yield();
	if(running) {
		pthread_cancel(tid);
		if(!pthread_join(tid, NULL)) 
			running = false;
	}	
}

void DetachedThread::release(void)
{
	dealloc();
	pthread_exit(NULL);
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
    member *member;
    Object *obj = NULL;
	lock();
    if(!Conditional::wait(timeout)) {
		unlock();
        return NULL;
    }
    if(tail) {
        --used;
        member = static_cast<Queue::member *>(head);
        obj = member->object;
		member->delist(this);
        member->LinkedObject::enlist(&freelist);
    }
	signal();
	unlock();
    return obj;
}

Object *Queue::fifo(timeout_t timeout)
{
	linked_pointer<member> node;
	Object *obj = NULL;
	lock();
	if(!Conditional::wait(timeout)) {
		unlock();
		return NULL;
	}
	if(head) {
		--used;
		node = begin();
		obj = node->object;
		head = head->getNext();
		if(!head)
			tail = NULL;
		node->LinkedObject::enlist(&freelist);
	}
	signal();
	unlock();
	return obj;
}

bool Queue::post(Object *object, timeout_t timeout)
{
	member *node;
	LinkedObject *mem;
	lock();
	while(limit && used == limit) {
		if(!Conditional::wait(timeout)) {
			unlock();
			return false;	
		}
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
    member *member;
    Object *obj = NULL;
	lock();
    if(!Conditional::wait(timeout)) {
		unlock();
        return NULL;
    }
    if(usedlist) {
        member = static_cast<Stack::member *>(usedlist);
        obj = member->object;
		usedlist = member->getNext();
        member->enlist(&freelist);
    }
	signal();
	unlock();
    return obj;
}

bool Stack::push(Object *object, timeout_t timeout)
{
	member *node;
	LinkedObject *mem;
	lock();
	while(limit && used == limit) {
		if(!Conditional::wait(timeout)) {
			unlock();
			return false;	
		}
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

Buffer::Buffer(size_t capacity, size_t osize) : 
Conditional()
{
	size = capacity;
	objsize = osize;
	used = 0;

	if(osize) {
		buf = (char *)malloc(capacity * osize);
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

size_t Buffer::onWait(void *data)
{
	if(objsize) {
		memcpy(data, head, objsize);
		if((head += objsize) >= buf + (size * objsize))
			head = buf;
	}
	return objsize;
}

size_t Buffer::onPull(void *data)
{
    if(objsize) {
		if(tail == buf)
			tail = buf + (size * objsize);
		tail -= objsize;
        memcpy(data, tail, objsize);
    }
    return objsize;
}

size_t Buffer::onPost(void *data)
{
	if(objsize) {
		memcpy(tail, data, objsize);
		if((tail += objsize) > buf + (size *objsize))
			tail = buf;
	}
	return objsize;
}

size_t Buffer::onPeek(void *data)
{
	if(objsize)
		memcpy(data, head, objsize);
	return objsize;
}

size_t Buffer::wait(void *buf, timeout_t timeout)
{
	size_t rc = 0;

	lock();	
	if(!Conditional::wait(timeout)) {
		unlock();
		return Buffer::timeout;
	}
	rc = onWait(buf);
	--used;
	signal();
	unlock();
	return rc;
}

size_t Buffer::pull(void *buf, timeout_t timeout)
{
    size_t rc = 0;

	lock();
    if(!Conditional::wait(timeout)) {
		unlock();
        return Buffer::timeout;
    }
    rc = onPull(buf);
    --used;
	signal();
	unlock();
    return rc;
}

size_t Buffer::post(void *buf, timeout_t timeout)
{
	size_t rc = 0;
	lock();
	while(used == size) {
		if(!Conditional::wait(timeout)) {
			unlock();
			return Buffer::timeout;
		}
	}
	rc = onPost(buf);
	++used;
	signal();
	unlock();
	return rc;
}

size_t Buffer::peek(void *buf)
{
	size_t rc = 0;

	lock();
	if(!used) {
		unlock();
		return 0;
	}
	rc = onPeek(buf);
	unlock();
	return rc;
}

bool Buffer::operator!()
{
	if(head && tail)
		return false;

	return true;
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

