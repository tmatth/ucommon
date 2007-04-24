#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/process.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

using namespace UCOMMON_NAMESPACE;

Mutex::attribute Mutex::attr;
Conditional::attribute Conditional::attr;

Mutex::attribute::attribute()
{
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutexattr_init(&pattr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
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
	pthread_condattr_init(&pattr);
#if _POSIX_TIMERS > 0 && defined(HAVE_PTHREAD_CONDATTR_SETCLOCK)
#if defined(_POSIX_MONOTONIC_CLOCK)
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	pthread_condattr_setclock(&pattr, CLOCK_MONOTONIC);
#else
	pthread_condattr_setclock(&attr, CLOCK_REALTIME);
	pthread_condattr_setclock(&pattr, CLOCK_REALTIME);
#endif
#endif
	pthread_condattr_setpshared(&pattr, PTHREAD_PROCESS_SHARED);
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

void Conditional::mapped(Conditional *c)
{
	pthread_mutex_init(&c->mutex, Mutex::pinitializer());
	pthread_cond_init(&c->cond, pinitializer());
}

bool Conditional::wait(timeout_t timeout)
{
	struct timespec ts;
	if(!timeout)
		return false;

	if(timeout == Timer::inf) {
		wait();
		return true;
	}

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

void Thread::dealloc(void)
{
}

void PooledThread::dealloc(void)
{
	if(!poolused)
		delete this;
}

void DetachedThread::dealloc(void)
{
	delete this;
}

void DetachedThread::cancel(void)
{
	pthread_cancel(tid);
}

void PooledThread::cancel(void)
{
	crit(0);
}

void PooledThread::release(void)
{
	lock();
	--poolused;
	unlock();
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
    if(over)
        pthread_exit(NULL);
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
	if(over)
		pthread_exit(NULL);
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

JoinableThread::~JoinableThread()
{
	release();
}

DetachedThread::~DetachedThread()
{
}
		
extern "C" {
	static void exit_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		th->dealloc();
	};

	static void *exec_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);
		pthread_cleanup_push(exit_thread, th);
		th->run();
		th->release();
		pthread_cleanup_pop(1);
		return NULL;
	};
}

void JoinableThread::pause(void)
{
	pthread_testcancel();
	cpr_yield();
}

void JoinableThread::pause(timeout_t timeout)
{
	int ctype;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &ctype);
	cpr_sleep(timeout);
	pthread_setcanceltype(ctype, NULL);
}

void DetachedThread::pause(void)
{
	cpr_yield();
}

void DetachedThread::pause(timeout_t timeout)
{
	cpr_sleep(timeout);
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
    if(over)
		pthread_exit(NULL);
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
	if(over)
		pthread_exit(NULL);
	else
		cpr_yield();
}

void JoinableThread::release(void)
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

	lock();
	while(!count) {
		if(!wait(timeout))
			break;
	}
	if(count)
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

	lock();
	while(count == limit)
		rtn = wait(timeout);
	if(rtn) {
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

extern "C" void cpr_cancel_async(cancellation *cancel)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &cancel->type);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &cancel->state);
}

extern "C" void cpr_cancel_suspend(cancellation *cancel)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel->state);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &cancel->type);
}

extern "C" void cpr_cancel_resume(cancellation *cancel)
{
	pthread_setcanceltype(cancel->type, NULL);
	pthread_setcancelstate(cancel->state, NULL);
	if(cancel->state == PTHREAD_CANCEL_ENABLE)
		pthread_testcancel();
}


