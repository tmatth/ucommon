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

static void cpr_sleep(timeout_t timeout)
{
#ifdef	_MSWINDOWS_
	SleepEx(timeout, FALSE);
#else
	timespec ts;
	ts.tv_sec = timeout / 1000l;
	ts.tv_nsec = (timeout % 1000l) * 1000000l;
#ifdef	HAVE_PTHREAD_DELAY
	pthread_delay(&ts);
#else
	usleep(timeout * 1000);
#endif
#endif
}

recursive_mutex::attribute recursive_mutex::attr;
Conditional::attribute Conditional::attr;

recursive_mutex::attribute::attribute()
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

	obj->retain();
	obj->release();

	lock();
	obj->enlist(ru);
	if(waiting)
		signal();
	unlock();
}

Completion::Completion() :
Conditional()
{
	count = 0;
	signalled = false;
}

unsigned Completion::request(void)
{
	unsigned id;
	lock();
	id = ++count;
	signalled = false;
	signal();
	unlock();
	return id;
}

bool Completion::accepts(unsigned id)
{
	lock();
	if(id != count) {
		unlock();
		return false;
	}

	signal();
	++signalled;
	return true;
}

bool Completion::wait(timeout_t timeout)
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
	if(last != count)
		notimeout = false;
	else
		++count;
	unlock();
	return notimeout;
}

bool Completion::wait(void)
{
	bool rtn = true;
	lock();
	unsigned last = count;
	while(!signalled && count == last)
		Conditional::wait();
	if(last != count)
		rtn = false;
	else
		++count;
	unlock();
	return rtn;
}

semaphore::semaphore(unsigned limit) : 
Conditional()
{
	count = limit;
	waits = 0;
	used = 0;
}

void semaphore::Shlock(void)
{
	wait();
}

void semaphore::Unlock(void)
{
	release();
}

unsigned semaphore::getUsed(void)
{
	unsigned rtn;
	lock();
	rtn = used;
	unlock();
	return rtn;
}

unsigned semaphore::getCount(void)
{
    unsigned rtn;
	lock();
    rtn = count;
	unlock();
    return rtn;
}

bool semaphore::wait(timeout_t timeout)
{
	return request(1, timeout);
}

bool semaphore::request(unsigned size, timeout_t timeout)
{
	bool result = true;
	Timer expires;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	if(used + size > count) {
		++waits;
		while(used + size > count && result) {
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
		used += size;
	unlock();
	return result;
}

void semaphore::wait(void)
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

void semaphore::request(unsigned size)
{
	lock();
	if(used + size > count) {
		++waits;
		while(used + size > count)
			Conditional::wait();
		--waits;
	}
	used += size;
	unlock();
}

void semaphore::release(unsigned size)
{
	while(size--)
		release();
}

void semaphore::release(void)
{
	lock();
	if(used)
		--used;
	if(waits)
		signal();
	unlock();
}

void semaphore::set(unsigned value)
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

rwlock::rwlock() :
Conditional()
{
	writers = false;
	reading = 0;
	waiting = 0;
}

void rwlock::Exlock(void)
{
	exclusive();
}

void rwlock::Shlock(void)
{
	shared();
}

void rwlock::Unlock(void)
{
	release();
}

bool rwlock::exclusive(timeout_t timeout)
{
	Timer expires;
	bool rtn = true;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);
	
	lock();
	while(reading && rtn) {
		++waiting;
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
		--waiting;
	}
	if(rtn)
		writers = true;
	unlock();
	return rtn;
}

bool rwlock::shared(timeout_t timeout)
{
	Timer expires;
	bool rtn = true;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);	

	lock();
	while(writers && rtn) {
		++waiting;
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
		--waiting;
	}
	if(rtn)
		++reading;
	unlock();
	return rtn;
}

void rwlock::release(void)
{
	lock();
	if(writers) {
		writers = false;
		if(waiting)
			broadcast();
		unlock();
		return;
	}
	if(reading) {
		--reading;
		if(!reading && waiting)
			signal();
	}
	unlock();
}

recursive_mutex::recursive_mutex()
{
	crit(pthread_mutex_init(&mutex, &attr.attr) == 0);
}

recursive_mutex::~recursive_mutex()
{
	pthread_mutex_destroy(&mutex);
}

void recursive_mutex::Exlock(void)
{
	pthread_mutex_lock(&mutex);
}

void recursive_mutex::Unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

mutex::mutex()
{
	crit(pthread_mutex_init(&mlock, NULL) == 0);
}

mutex::~mutex()
{
	pthread_mutex_destroy(&mlock);
}

void mutex::Exlock(void)
{
	pthread_mutex_lock(&mlock);
}

void mutex::Unlock(void)
{
	pthread_mutex_unlock(&mlock);
}

ConditionalLock::ConditionalLock() :
Conditional()
{
	waits = 0;
	reads = 0;
}

void ConditionalLock::Shlock(void)
{
	shared();
}

void ConditionalLock::Unlock(void)
{
	release();
}

void ConditionalLock::exclusive(void)
{
	Conditional::lock();
	while(reads) {
		++waits;
		Conditional::wait();
		--waits;
	}
}

void ConditionalLock::release(void)
{
	if(reads) {
		Conditional::lock();
		--reads;
		if(!reads && waits)
			Conditional::broadcast();
	}
	Conditional::unlock();
}

void ConditionalLock::shared(void)
{
	Conditional::lock();
	++reads;
	Conditional::unlock();
}

barrier::barrier(unsigned limit) :
Conditional()
{
	count = limit;
	waits = 0;
}

barrier::~barrier()
{
	for(;;)
	{
		lock();
		if(waits)
			broadcast();
		unlock();
	}
}

void barrier::set(unsigned limit)
{
	lock();
	count = limit;
	if(count <= waits) {
		waits = 0;
		broadcast();
	}
	unlock();
}

void barrier::wait(void)
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
OrderedIndex()
{
	pthread_mutex_init(&mutex, NULL);
}

void LockedIndex::lock_index(void)
{
	pthread_mutex_lock(&mutex);
}

void LockedIndex::unlock_index(void)
{
	pthread_mutex_unlock(&mutex);
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
ConditionalLock()
{
	pointer = NULL;
}

SharedPointer::~SharedPointer()
{
}

void SharedPointer::replace(SharedObject *ptr)
{
	ConditionalLock::exclusive();
	if(pointer)
		delete pointer;
	pointer = ptr;
	if(ptr)
		ptr->commit(this);
	ConditionalLock::unlock();
}

SharedObject *SharedPointer::share(void)
{
	ConditionalLock::shared();
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

void Thread::sleep(timeout_t timeout)
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
		sleep(Timer::inf);
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

queue::member::member(queue *q, Object *o) :
OrderedObject(q)
{
	o->retain();
	object = o;
}

queue::queue(mempager *p, size_t size) :
OrderedIndex(), Conditional()
{
	pager = p;
	freelist = NULL;
	used = 0;
	limit = size;
}

bool queue::remove(Object *o)
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

Object *queue::lifo(timeout_t timeout)
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
        member = static_cast<queue::member *>(head);
        obj = member->object;
		member->delist(this);
        member->LinkedObject::enlist(&freelist);
    }
	if(rtn)
		signal();
	unlock();
    return obj;
}

Object *queue::fifo(timeout_t timeout)
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

bool queue::post(Object *object, timeout_t timeout)
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

size_t queue::getCount(void)
{
	size_t count;
	lock();
	count = used;
	unlock();
	return count;
}


stack::member::member(stack *S, Object *o) :
LinkedObject((&S->usedlist))
{
	o->retain();
	object = o;
}

stack::stack(mempager *p, size_t size) :
Conditional()
{
	pager = p;
	freelist = usedlist = NULL;
	limit = size;
	used = 0;
}

bool stack::remove(Object *o)
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

Object *stack::pull(timeout_t timeout)
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
        member = static_cast<stack::member *>(usedlist);
        obj = member->object;
		usedlist = member->getNext();
        member->enlist(&freelist);
	}
	if(rtn)
		signal();
	unlock();
    return obj;
}

bool stack::push(Object *object, timeout_t timeout)
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

size_t stack::getCount(void)
{
	size_t count;
	lock();
	count = used;
	unlock();
	return count;
}

Buffer::Buffer(size_t osize, size_t c) : 
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
	unsigned bcount = 0;

	lock();
	if(tail > head) 
		bcount = (unsigned)((size_t)(tail - head) / objsize);
	else if(head > tail)
		bcount = (unsigned)((((buf + size) - head) + (tail - buf)) / objsize);
	unlock();
	return bcount;
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


