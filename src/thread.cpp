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
#include <string.h>

#if	UCOMMON_THREADING > 0

using namespace UCOMMON_NAMESPACE;

const size_t Buffer::timeout = ((size_t)(-1));
#if	_POSIX_PRIORITY_SCHEDULING
int Thread::policy = SCHED_RR;
#else
int Thread::policy = 0;
#endif
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
	unsigned last = count;
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
	unsigned last = count;
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
	live = true;
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
	destroy();
}

void Conditional::destroy(void)
{
	if(live) {
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
	}
	live = false;
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

LockedPointer::LockedPointer()
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&mutex, &lock, sizeof(mutex));
	pointer = NULL;
}

void LockedPointer::set(Object *obj)
{
	pthread_mutex_lock(&mutex);
	obj->retain();
	if(pointer)
		pointer->release();
	pointer = obj;
	pthread_mutex_unlock(&mutex);
}

Object *LockedPointer::get(void)
{
	Object *temp;
	pthread_mutex_lock(&mutex);
	temp = pointer;
	if(temp)
		temp->retain();
	pthread_mutex_unlock(&mutex);
	return temp;
}

SharedPointer::SharedPointer()
{
	crit(pthread_rwlock_init(&lock, NULL) == 0);
	pointer = NULL;
}

SharedPointer::~SharedPointer()
{
	pthread_rwlock_destroy(&lock);
}

void SharedPointer::set(Object *ptr)
{
	pthread_rwlock_wrlock(&lock);
	ptr->retain();
	if(pointer)
		pointer->release();
	pointer = ptr;
	pthread_rwlock_unlock(&lock);
}

Object *SharedPointer::get(void)
{
	pthread_rwlock_rdlock(&lock);
	if(pointer)
		pointer->retain();
	return pointer;
}

void SharedPointer::release(void)
{
	if(pointer)
		pointer->release();
	pthread_rwlock_unlock(&lock);
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

Thread::Thread(unsigned p, size_t size)
{
	priority = p;
	stack = size;
}

Thread::~Thread()
{
	Thread::release();
}

#ifdef	_POSIX_PRIORITY_SCHEDULING
unsigned Thread::maxPriority(void)
{
	int min = sched_get_priority_min(policy);
	int max = sched_get_priority_max(policy);
	return max - min;
}

void Thread::setPriority(unsigned pri)
{
	struct sched_param sparam;
	bool reset = false;
	int pval = (int)pri, rc;

	if(!priority)
		return;

	if(!pri)
		reset = true;
		
	pval += priority;

	int min = sched_get_priority_min(policy);
    int max = sched_get_priority_max(policy);

	if(min == max)
		return;
	
	pval += min;
	if(pval > max)
		pval = max;

	if(tid != 0) {
		sparam.sched_priority = pval;
		if(reset)
			rc = pthread_setschedparam(tid, policy, &sparam);		
		else
			rc = pthread_setschedprio(tid, pval);
		assert(rc == 0);
	}
}
#else
unsigned Thread::maxPriority(void)
{
	return 0;
}

void Thread::setPriority(unsigned pri)
{
}	
#endif
		
extern "C" {
	static void *exec_thread(void *obj)
	{
		Thread *th = static_cast<Thread *>(obj);
		th->setPriority(0);
		th->run();
		th->release();
		return NULL;
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
	if(stack && stack < PTHREAD_STACK_MIN)
		stack = PTHREAD_STACK_MIN;
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

Queue::~Queue()
{
	Conditional::destroy();
}

bool Queue::remove(Object *o)
{
	bool rtn = false;
	member *node;
	Exlock();
	node = static_cast<member*>(head);
	while(node) {
		if(node->object == o)
			break;
		node = static_cast<member*>(node->getNext());
	}
	if(node) {
		--used;
		rtn = true;
		node->object->release();
		node->delist(this);
		node->LinkedObject::enlist(&freelist);			
	}
	Unlock();
	return rtn;
}

Object *Queue::lifo(timeout_t timeout)
{
    member *member;
    Object *obj = NULL;
    Exlock();
    if(!Conditional::wait(timeout)) {
        Unlock();
        return NULL;
    }
    if(tail) {
        --used;
        member = static_cast<Queue::member *>(head);
        obj = member->object;
		member->delist(this);
        member->LinkedObject::enlist(&freelist);
    }
    Conditional::signal(false);
    Unlock();
    return obj;
}

Object *Queue::fifo(timeout_t timeout)
{
	member *member;
	Object *obj = NULL;
	Exlock();
	if(!Conditional::wait(timeout)) {
		Unlock();
		return NULL;
	}
	if(head) {
		--used;
		member = static_cast<Queue::member *>(head);
		obj = member->object;
		head = static_cast<Queue::member*>(head->getNext());
		if(!head)
			tail = NULL;
		member->LinkedObject::enlist(&freelist);
	}
	Conditional::signal(false);
	Unlock();
	return obj;
}

bool Queue::post(Object *object, timeout_t timeout)
{
	member *node;
	LinkedObject *mem;
	Exlock();
	while(limit && used == limit) {
		if(!Conditional::wait(timeout)) {
			Unlock();
			return false;	
		}
	}
	++used;
	if(freelist) {
		mem = freelist;
		freelist = freelist->getNext();
		node = new((caddr_t)mem) member(this, object);
	}
	else
		node = new(pager) member(this, object);
	Conditional::signal(false);
	Unlock();
	return true;
}

size_t Queue::getCount(void)
{
	size_t count;
	Exlock();
	count = used;
	Unlock();
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

Stack::~Stack()
{
	Conditional::destroy();
}

bool Stack::remove(Object *o)
{
	bool rtn = false;
	member *node;
	Exlock();
	node = static_cast<member*>(usedlist);
	while(node) {
		if(node->object == o)
			break;
		node = static_cast<member*>(node->getNext());
	}
	if(node) {
		--used;
		rtn = true;
		node->object->release();
		node->delist(&usedlist);
		node->enlist(&freelist);			
	}
	Unlock();
	return rtn;
}

Object *Stack::pull(timeout_t timeout)
{
    member *member;
    Object *obj = NULL;
    Exlock();
    if(!Conditional::wait(timeout)) {
        Unlock();
        return NULL;
    }
    if(usedlist) {
        member = static_cast<Stack::member *>(usedlist);
        obj = member->object;
		usedlist = member->getNext();
        member->enlist(&freelist);
    }
    Conditional::signal(false);
    Unlock();
    return obj;
}

bool Stack::push(Object *object, timeout_t timeout)
{
	member *node;
	LinkedObject *mem;
	Exlock();
	while(limit && used == limit) {
		if(!Conditional::wait(timeout)) {
			Unlock();
			return false;	
		}
	}
	++used;
	if(freelist) {
		mem = freelist;
		freelist = freelist->getNext();
		node = new((caddr_t)mem) member(this, object);
	}
	else
		node = new(pager) member(this, object);
	Conditional::signal(false);
	Unlock();
	return true;
}

size_t Stack::getCount(void)
{
	size_t count;
	Exlock();
	count = used;
	Unlock();
	return count;
}

Buffer::Buffer(size_t capacity, size_t osize) : Conditional()
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
	Conditional::destroy();
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
	
	Exlock();
	if(!Conditional::wait(timeout)) {
		Unlock();
		return Buffer::timeout;
	}
	rc = onWait(buf);
	--used;
	Conditional::signal(false);
	Unlock();
	return rc;
}

size_t Buffer::pull(void *buf, timeout_t timeout)
{
    size_t rc = 0;

    Exlock();
    if(!Conditional::wait(timeout)) {
        Unlock();
        return Buffer::timeout;
    }
    rc = onPull(buf);
    --used;
    Conditional::signal(false);
    Unlock();
    return rc;
}

size_t Buffer::post(void *buf, timeout_t timeout)
{
	size_t rc = 0;
	Exlock();
	while(used == size) {
		if(!Conditional::wait(timeout)) {
			Unlock();
			return Buffer::timeout;
		}
	}
	rc = onPost(buf);
	++used;
	Conditional::signal(false);
	Unlock();
	return rc;
}

size_t Buffer::peek(void *buf)
{
	size_t rc = 0;

	Lock();
	if(!used) {
		Unlock();
		return 0;
	}
	rc = onPeek(buf);
	Unlock();
	return rc;
}

bool Buffer::operator!()
{
	if(head && tail)
		return false;

	return true;
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
	object = p.get();
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
	object = p.get();
	return *this;
}

shared_release::shared_release(const shared_release &copy)
{
	ptr = copy.ptr;
	if(ptr)
		object = ptr->get();
	else
		object = NULL;
}

shared_release::shared_release()
{
	ptr = NULL;
	object = NULL;
}

shared_release::shared_release(SharedPointer &p)
{
	ptr = &p;
	object = p.get();
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
	object = NULL;
}

shared_release &shared_release::operator=(SharedPointer &p)
{
	release();
	ptr = &p;
	object = p.get();
	return *this;
}

#endif
