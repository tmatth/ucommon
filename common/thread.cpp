// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/timers.h>
#include <ucommon/linked.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#if _POSIX_PRIORITY_SCHEDULING > 0
#include <sched.h>
static int realtime_policy = SCHED_FIFO;
#endif

#undef	_POSIX_SPIN_LOCKS

static unsigned max_sharing = 0;

using namespace UCOMMON_NAMESPACE;

struct mutex_entry
{
	pthread_mutex_t mutex;
	struct mutex_entry *next;
	void *pointer;
	unsigned count;
};

class __LOCAL rwlock_entry : public rwlock
{
public:
	rwlock_entry();
	rwlock_entry *next;
	void *object;
	unsigned count;
};
	
class __LOCAL mutex_index : public mutex
{
public:
	struct mutex_entry *list;

	mutex_index();
};

class __LOCAL rwlock_index : public mutex
{
public:
	rwlock_entry *list;

	rwlock_index();
};

static rwlock_index single_rwlock;
static rwlock_index *rwlock_table = &single_rwlock;
static mutex_index single_table;
static mutex_index *mutex_table = &single_table;
static unsigned mutex_indexing = 1;
static unsigned rwlock_indexing = 1;

mutex_index::mutex_index() : mutex()
{
	list = NULL;
}

rwlock_index::rwlock_index() : mutex()
{
	list = NULL;
}

rwlock_entry::rwlock_entry() : rwlock()
{
	count = 0;
}

#if !defined(_MSWINDOWS_) && !defined(__PTH__)
Conditional::attribute Conditional::attr;
#endif

#ifdef	__PTH__

static int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	static pth_key_t ev_key = PTH_KEY_INIT;
	pth_event_t ev = pth_event(PTH_EVENT_TIME|PTH_MODE_STATIC, &ev_key,
		pth_time(abstime->tv_sec, (abstime->tv_nsec) / 1000));
	
	if(!pth_cond_await(cond, mutex, ev))
		return errno;
	return 0;
}

static void pthread_shutdown(void)
{
	pth_kill();
}

inline pthread_t pthread_self(void)
	{return pth_self();};

#endif

static unsigned hash_address(void *ptr, unsigned indexing)
{
	assert(ptr != NULL);

	unsigned key = 0;
	unsigned count = 0;
	unsigned char *addr = (unsigned char *)(&ptr);

	if(indexing < 2)
		return 0;

	// skip lead zeros if little endian...
	while(count < sizeof(void *) && *addr == 0) {
		++count;
		++addr;
	}

	while(count++ < sizeof(void *) && *addr)
		key = (key << 1) ^ *(addr++);	

	return key % indexing;
}

ReusableAllocator::ReusableAllocator() :
Conditional()
{
	freelist = NULL;
	waiting = 0;
}

void ReusableAllocator::release(ReusableObject *obj)
{
	assert(obj != NULL);

	LinkedObject **ru = (LinkedObject **)&freelist;

	obj->retain();
	obj->release();

	lock();
	obj->enlist(ru);

	if(waiting)
		signal();

	unlock();
}

void ConditionalAccess::limit_sharing(unsigned max)
{
	max_sharing = max;
}

void Conditional::gettimeout(timeout_t msec, struct timespec *ts)
{
	assert(ts != NULL);

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

semaphore::semaphore(unsigned limit) : 
Conditional()
{
	assert(limit > 0);

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
	bool result = true;
	struct timespec ts;
	gettimeout(timeout, &ts);

	lock();
	while(used >= count && result) {
		++waits;
		result = Conditional::wait(&ts);
		--waits;
	}
	if(result)
		++used;
	unlock();
	return result;
}

void semaphore::wait(void)
{
	lock();
	if(used >= count) {
		++waits;
		Conditional::wait();
		--waits;
	}
	++used;
	unlock();
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
	assert(value > 0);

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

#ifdef	_MSWINDOWS_

bool Thread::equal(pthread_t t1, pthread_t t2)
{
	return t1 == t2;
}

void Conditional::wait(void)
{
	int result;

	EnterCriticalSection(&mlock);
	++waiting;
	LeaveCriticalSection(&mlock);
	LeaveCriticalSection(&mutex);
	result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
	EnterCriticalSection(&mlock);
	--waiting;
	result = ((result == WAIT_OBJECT_0 + BROADCAST) && (waiting == 0));
	LeaveCriticalSection(&mlock);
	if(result)
		ResetEvent(&events[BROADCAST]);
	EnterCriticalSection(&mutex);
}

void Conditional::signal(void)
{
	EnterCriticalSection(&mlock);
	if(waiting)
		SetEvent(&events[SIGNAL]);
	LeaveCriticalSection(&mlock);
}

void Conditional::broadcast(void)
{
	EnterCriticalSection(&mlock);
	if(waiting)
		SetEvent(&events[BROADCAST]);
	LeaveCriticalSection(&mlock);

}

Conditional::Conditional()
{
	waiting = 0;

	InitializeCriticalSection(&mutex);
	InitializeCriticalSection(&mlock);
	events[SIGNAL] = CreateEvent(NULL, FALSE, FALSE, NULL);
	events[BROADCAST] = CreateEvent(NULL, TRUE, FALSE, NULL);	
}

Conditional::~Conditional()
{
	DeleteCriticalSection(&mlock);
	DeleteCriticalSection(&mutex);
	CloseHandle(events[SIGNAL]);
	CloseHandle(events[BROADCAST]);
}

bool Conditional::wait(timeout_t timeout)
{
	int result;
	bool rtn = true;

	if(!timeout)
		return false;

	EnterCriticalSection(&mlock);
	++waiting;
	LeaveCriticalSection(&mlock);
	LeaveCriticalSection(&mutex);
	result = WaitForMultipleObjects(2, events, FALSE, timeout);
	EnterCriticalSection(&mlock);
	--waiting;
	if(result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + BROADCAST)
		rtn = true;
	result = ((result == WAIT_OBJECT_0 + BROADCAST) && (waiting == 0));
	LeaveCriticalSection(&mlock);
	if(result)
		ResetEvent(&events[BROADCAST]);
	EnterCriticalSection(&mutex);
	return rtn;
}

bool Conditional::wait(struct timespec *ts)
{
	assert(ts != NULL);

	return wait((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

#else

#include <stdio.h>
bool Thread::equal(pthread_t t1, pthread_t t2)
{
#ifdef	__PTH__
	return (t1 == t2);
#else
	return pthread_equal(t1, t2) != 0;
#endif
}

#ifndef	__PTH__
Conditional::attribute::attribute()
{
	Thread::init();
	pthread_condattr_init(&attr);
#if _POSIX_TIMERS > 0 && defined(HAVE_PTHREAD_CONDATTR_SETCLOCK)
#if defined(_POSIX_MONOTONIC_CLOCK)
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#else
	pthread_condattr_setclock(&attr, CLOCK_REALTIME);
#endif
#endif
}
#endif

Conditional::Conditional()
{
#ifdef	__PTH__
	Thread::init();
	pth_cond_init(&cond);
	pth_mutex_init(&mutex);
#else
	crit(pthread_cond_init(&cond, &attr.attr) == 0, "conditional init failed");
	crit(pthread_mutex_init(&mutex, NULL) == 0, "mutex init failed");
#endif
}

Conditional::~Conditional()
{
#ifndef	__PTH__
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
#endif
}

bool Conditional::wait(timeout_t timeout)
{
	struct timespec ts;
	gettimeout(timeout, &ts);
	return wait(&ts);
}

bool Conditional::wait(struct timespec *ts)
{
	assert(ts != NULL);

	if(pthread_cond_timedwait(&cond, &mutex, ts) == ETIMEDOUT)
		return false;

	return true;
}

#endif


#ifdef	_MSWINDOWS_

void ConditionalAccess::waitSignal(void)
{
	LeaveCriticalSection(&mutex);
	WaitForSingleObject(&events[SIGNAL], INFINITE);
	EnterCriticalSection(&mutex);
}

void ConditionalAccess::waitBroadcast(void)
{
	int result;

	EnterCriticalSection(&mlock);
	++waiting;
	LeaveCriticalSection(&mlock);
	LeaveCriticalSection(&mutex);
	result = WaitForSingleObject(&events[BROADCAST], INFINITE);
	EnterCriticalSection(&mlock);
	--waiting;
	result = ((result == WAIT_OBJECT_0) && (waiting == 0));
	LeaveCriticalSection(&mlock);
	if(result)
		ResetEvent(&events[BROADCAST]);
	EnterCriticalSection(&mutex);
}

ConditionalAccess::ConditionalAccess() : Conditional()
{
	pending = waiting = pending = 0;
}

ConditionalAccess::~ConditionalAccess()
{
}

bool ConditionalAccess::waitSignal(timeout_t timeout)
{
	int result;
	bool rtn = true;

	if(!timeout)
		return false;

	LeaveCriticalSection(&mutex);
	result = WaitForSingleObject(events[SIGNAL], timeout);
	if(result == WAIT_OBJECT_0)
		rtn = true;
	EnterCriticalSection(&mutex);
	return rtn;
}

bool ConditionalAccess::waitSignal(struct timespec *ts)
{
	assert(ts != NULL);

	return waitSignal((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}


bool ConditionalAccess::waitBroadcast(timeout_t timeout)
{
	int result;
	bool rtn = true;

	if(!timeout)
		return false;

	EnterCriticalSection(&mlock);
	++waiting;
	LeaveCriticalSection(&mlock);
	LeaveCriticalSection(&mutex);
	result = WaitForSingleObject(events[BROADCAST], timeout);
	EnterCriticalSection(&mlock);
	--waiting;
	if(result == WAIT_OBJECT_0)
		rtn = true;
	result = ((result == WAIT_OBJECT_0) && (waiting == 0));
	LeaveCriticalSection(&mlock);
	if(result)
		ResetEvent(&events[BROADCAST]);
	EnterCriticalSection(&mutex);
	return rtn;
}

bool ConditionalAccess::waitBroadcast(struct timespec *ts)
{
	assert(ts != NULL);

	return waitBroadcast((timeout_t)(ts->tv_sec * 1000 + (ts->tv_nsec / 1000000l)));
}

#else

ConditionalAccess::ConditionalAccess()
{
	waiting = pending = sharing = 0;
#ifdef	__PTH__
	pth_cond_init(&bcast);
#else
	crit(pthread_cond_init(&bcast, &attr.attr) == 0, "conditional init failed");
#endif
}

ConditionalAccess::~ConditionalAccess()
{
#ifndef	__PTH__
	pthread_cond_destroy(&bcast);
#endif
}

bool ConditionalAccess::waitSignal(timeout_t timeout)
{
	struct timespec ts;
	gettimeout(timeout, &ts);
	return waitSignal(&ts);
}

bool ConditionalAccess::waitBroadcast(struct timespec *ts)
{
	assert(ts != NULL);

	if(pthread_cond_timedwait(&bcast, &mutex, ts) == ETIMEDOUT)
		return false;

	return true;
}

bool ConditionalAccess::waitBroadcast(timeout_t timeout)
{
	struct timespec ts;
	gettimeout(timeout, &ts);
	return waitBroadcast(&ts);
}

bool ConditionalAccess::waitSignal(struct timespec *ts)
{
	assert(ts != NULL);

	if(pthread_cond_timedwait(&cond, &mutex, ts) == ETIMEDOUT)
		return false;

	return true;
}

#endif

void ConditionalAccess::modify(void)
{
	lock();
	while(sharing) {
		++pending;
		waitSignal();
		--pending;
	}
}

void ConditionalAccess::commit(void)
{
	if(pending)
		signal();
	else if(waiting)
		broadcast();
	unlock();
}

void ConditionalAccess::access(void)
{
	lock();
	assert(!max_sharing || sharing < max_sharing);
	while(pending) {
		++waiting;
		waitBroadcast();
		--waiting;
	}
	++sharing;
	unlock();
}

void ConditionalAccess::release(void)
{
   lock();
	assert(sharing);

    --sharing;
    if(pending && !sharing)
        signal();
    else if(waiting && !pending)
        broadcast();
    unlock();
}

rexlock::rexlock() :
Conditional()
{
	lockers = 0;
	waiting = 0;
}

unsigned rexlock::getWaiting(void)
{
	unsigned count;
	Conditional::lock();
	count = waiting;
	Conditional::unlock();
	return count;
}

unsigned rexlock::getLocking(void)
{
	unsigned count;
	Conditional::lock();
	count = lockers;
	Conditional::unlock();
	return count;
}

void rexlock::Exlock(void)
{
	lock();
}

void rexlock::Unlock(void)
{
	release();
}

void rexlock::lock(void)
{
	Conditional::lock();
	while(lockers) {
		if(Thread::equal(locker, pthread_self()))
			break;
		++waiting;
		Conditional::wait();
		--waiting;
	}
	if(!lockers)
		locker = pthread_self();
	++lockers;
	Conditional::unlock();
	return;
}

void rexlock::release(void)
{
	Conditional::lock();
	--lockers;
	if(!lockers && waiting)
		Conditional::signal();
	Conditional::unlock();
}

rwlock::rwlock() :
ConditionalAccess()
{
	writers = 0;
}

unsigned rwlock::getAccess(void)
{
	unsigned count;
	lock();
	count = sharing;
	unlock();
	return count;
}

unsigned rwlock::getModify(void)
{
	unsigned count;
	lock();
	count = writers;
	unlock();
	return count;
}

unsigned rwlock::getWaiting(void)
{
	unsigned count;
	lock();
	count = waiting + pending;
	unlock();
	return count;
}

void rwlock::Exlock(void)
{
	modify();
}

void rwlock::Shlock(void)
{
	access();
}

void rwlock::Unlock(void)
{
	release();
}

bool rwlock::modify(timeout_t timeout)
{
	bool rtn = true;
	struct timespec ts;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);
	
	lock();
	while((writers || sharing) && rtn) {
		if(writers && Thread::equal(writeid, pthread_self()))
			break;
		++pending;
		if(timeout == Timer::inf)
			waitSignal();
		else if(timeout)
			rtn = waitSignal(&ts);
		else
			rtn = false;
		--pending;
	}
	assert(!max_sharing || writers < max_sharing);
	if(rtn) {
		if(!writers)
			writeid = pthread_self();
		++writers;
	}
	unlock();
	return rtn;
}

bool rwlock::access(timeout_t timeout)
{
	struct timespec ts;
	bool rtn = true;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while((writers || pending) && rtn) {
		++waiting;
		if(timeout == Timer::inf)
			waitBroadcast();
		else if(timeout)
			rtn = waitBroadcast(&ts);
		else
			rtn = false;
		--waiting;
	}
	assert(!max_sharing || sharing < max_sharing);
	if(rtn)
		++sharing;
	unlock();
	return rtn;
}

void rwlock::release(void)
{
	lock();
	assert(sharing || writers);

	if(writers) {
		assert(!sharing);
		--writers;
		if(pending && !writers)
			signal();
		else if(waiting && !writers)
			broadcast();
		unlock();
		return;
	}
	if(sharing) {
		assert(!writers);
		--sharing;
		if(pending && !sharing)
			signal();
		else if(waiting && !pending)
			broadcast();
	}
	unlock();
}

auto_protect::auto_protect()
{
	object = NULL;
}

auto_protect::auto_protect(void *obj)
{
	object = obj;
	if(object)
		mutex::protect(obj);
}

auto_protect::~auto_protect()
{
	release();
}

void auto_protect::release()
{
	if(object) {
		mutex::release(object);
		object = NULL;
	}
}

void auto_protect::operator=(void *obj)
{
	if(obj == object)
		return;

	release();
	object = obj;
	if(object)
		mutex::protect(object);
}

mutex::gaurd::gaurd()
{
	object = NULL;
}

mutex::gaurd::gaurd(void *obj)
{
	object = obj;
	if(obj)
		mutex::protect(object);
}

mutex::gaurd::~gaurd()
{
	release();
}

void mutex::gaurd::set(void *obj)
{
	release();
	object = obj;
	if(obj)
		mutex::protect(object);
}

void mutex::gaurd::release(void)
{
	if(object) {
		mutex::release(object);
		object = NULL;
	}
}

mutex::mutex()
{
#ifdef	__PTH__
	pth_mutex_init(&mlock);
#else
	crit(pthread_mutex_init(&mlock, NULL) == 0, "mutex init failed");
#endif
}

mutex::~mutex()
{
	pthread_mutex_destroy(&mlock);
}

void mutex::indexing(unsigned index)
{
	if(index > 1) {
		mutex_table = new mutex_index[index];
		mutex_indexing = index;
	}
}

void rwlock::indexing(unsigned index)
{
	if(index > 1) {
		rwlock_table = new rwlock_index[index];
		rwlock_indexing = index;
	}
}

rwlock::gaurd_reader::gaurd_reader()
{
	object = NULL;
}

rwlock::gaurd_reader::gaurd_reader(void *obj)
{
	object = obj;
	if(obj)
		if(!rwlock::reader(object))
			object = NULL;
}

rwlock::gaurd_reader::~gaurd_reader()
{
	release();
}

void rwlock::gaurd_reader::set(void *obj)
{
	release();
	object = obj;
	if(obj)
		if(!rwlock::reader(object))
			object = NULL;
}

void rwlock::gaurd_reader::release(void)
{
	if(object) {
		rwlock::release(object);
		object = NULL;
	}
}

rwlock::gaurd_writer::gaurd_writer()
{
	object = NULL;
}

rwlock::gaurd_writer::gaurd_writer(void *obj)
{
	object = obj;
	if(obj)
		if(!rwlock::writer(object))
			object = NULL;
}

rwlock::gaurd_writer::~gaurd_writer()
{
	release();
}

void rwlock::gaurd_writer::set(void *obj)
{
	release();
	object = obj;
	if(obj)
		if(!rwlock::writer(object))
			object = NULL;
}

void rwlock::gaurd_writer::release(void)
{
	if(object) {
		rwlock::release(object);
		object = NULL;
	}
}

bool rwlock::reader(void *ptr, timeout_t timeout)
{
	rwlock_index *index = &rwlock_table[hash_address(ptr, rwlock_indexing)];
	rwlock_entry *entry, *empty = NULL;

	if(!ptr)
		return false;

	index->acquire();
	entry = index->list;
	while(entry) {
		if(entry->count && entry->object == ptr)
			break;
		if(!entry->count)
			empty = entry;
		entry = entry->next;
	}
	if(!entry) {
		if(empty)
			entry = empty;
		else {
			entry = new rwlock_entry;
			entry->next = index->list;
			index->list = entry;
		}
	}
	entry->object = ptr;
	++entry->count;
	index->release();
	if(entry->access(timeout))
		return true;
	index->acquire();
	--entry->count;
	index->release();
	return false;
}			

bool rwlock::writer(void *ptr, timeout_t timeout)
{
	rwlock_index *index = &rwlock_table[hash_address(ptr, rwlock_indexing)];
	rwlock_entry *entry, *empty = NULL;

	if(!ptr)
		return false;

	index->acquire();
	entry = index->list;
	while(entry) {
		if(entry->count && entry->object == ptr)
			break;
		if(!entry->count)
			empty = entry;
		entry = entry->next;
	}
	if(!entry) {
		if(empty)
			entry = empty;
		else {
			entry = new rwlock_entry;
			entry->next = index->list;
			index->list = entry;
		}
	}
	entry->object = ptr;
	++entry->count;
	index->release();
	if(entry->modify(timeout))
		return true;
	index->acquire();
	--entry->count;
	index->release();
	return false;
}			

void mutex::protect(void *ptr)
{
	mutex_index *index = &mutex_table[hash_address(ptr, mutex_indexing)];
	mutex_entry *entry, *empty = NULL;

	if(!ptr)
		return;

	index->acquire();
	entry = index->list;
	while(entry) {
		if(entry->count && entry->pointer == ptr)
			break;
		if(!entry->count)
			empty = entry;
		entry = entry->next;
	}
	if(!entry) {
		if(empty)
			entry = empty;
		else {
			entry = new struct mutex_entry;
			entry->count = 0;
			pthread_mutex_init(&entry->mutex, NULL);
			entry->next = index->list;
			index->list = entry;
		}
	}
	entry->pointer = ptr;
	++entry->count;
//	printf("ACQUIRE %p, THREAD %d, POINTER %p, COUNT %d\n", entry, Thread::self(), entry->pointer, entry->count);
	index->release();
	pthread_mutex_lock(&entry->mutex);
}	

void rwlock::release(void *ptr)
{
	rwlock_index *index = &rwlock_table[hash_address(ptr, rwlock_indexing)];
	rwlock_entry *entry;

	if(!ptr)
		return;

	index->acquire();
	entry = index->list;
	while(entry) {
		if(entry->count && entry->object == ptr)
			break;
		entry = entry->next;
	}

	assert(entry);
	if(entry) {
		entry->release();
		--entry->count;
	}
	index->release();
}	

void mutex::release(void *ptr)
{
	mutex_index *index = &mutex_table[hash_address(ptr, mutex_indexing)];
	mutex_entry *entry;

	if(!ptr)
		return;

	index->acquire();
	entry = index->list;
	while(entry) {
		if(entry->count && entry->pointer == ptr)
			break;
		entry = entry->next;
	}

	assert(entry);
	if(entry) {
//		printf("RELEASE %p, THREAD %d, POINTER %p COUNT %d\n", entry, Thread::self(), entry->pointer, entry->count);
		pthread_mutex_unlock(&entry->mutex);
		--entry->count;
	}
	index->release();
}	

void mutex::Exlock(void)
{
	pthread_mutex_lock(&mlock);
}

void mutex::Unlock(void)
{
	pthread_mutex_unlock(&mlock);
}

#ifdef	_MSWINDOWS_

TimedEvent::TimedEvent() : 
Timer()
{
	event = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&mutex);
	set();
}

TimedEvent::~TimedEvent()
{
	if(event != INVALID_HANDLE_VALUE) {
		CloseHandle(event);
		event = INVALID_HANDLE_VALUE;
	}
	DeleteCriticalSection(&mutex);
}

TimedEvent::TimedEvent(timeout_t timeout) :
Timer(timeout)
{
	event = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&mutex);
}

TimedEvent::TimedEvent(time_t timer) :
Timer(timer)
{
	event = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&mutex);
}

void TimedEvent::signal(void)
{
	SetEvent(event);
}

void TimedEvent::reset(void)
{
	set();
	ResetEvent(event);
}

bool TimedEvent::sync(void) 
{
	int result;
	timeout_t timeout;

	timeout = get();
	if(!timeout)
		return false;

	LeaveCriticalSection(&mutex);
	result = WaitForSingleObject(event, timeout);
	EnterCriticalSection(&mutex);
	if(result != WAIT_OBJECT_0)
		return true;
	return false;
}

bool TimedEvent::wait(timeout_t timer) 
{
	int result;
	timeout_t timeout;

	if(timer)
		operator+=(timer);

	timeout = get();
	if(!timeout)
		return false;

	result = WaitForSingleObject(event, timeout);
	if(result == WAIT_OBJECT_0)
		return true;
	return false;
}

#else

TimedEvent::TimedEvent() : 
Timer()
{
	signalled = false;
#ifdef	__PTH__
	Thread::init();
	pth_cond_init(&cond);
	pth_mutex_init(&mutex);
#else
	crit(pthread_cond_init(&cond, Conditional::initializer()) == 0, "conditional init failed");
	crit(pthread_mutex_init(&mutex, NULL) == 0, "mutex init failed");
#endif
	set();
}

TimedEvent::TimedEvent(timeout_t timeout) :
Timer(timeout)
{
	signalled = false;
#ifdef	__PTH__
	Thread::init();
	pth_cond_init(&cond);
	pth_mutex_init(&mutex);
#else
	crit(pthread_cond_init(&cond, Conditional::initializer()) == 0, "conditional init failed");
	crit(pthread_mutex_init(&mutex, NULL) == 0, "mutex init failed");
#endif
}

TimedEvent::TimedEvent(time_t timer) :
Timer(timer)
{
	signalled = false;
#ifdef	__PTH__
	Thread::init();
	pth_cond_init(&cond);
	pth_mutex_init(&mutex);
#else
	crit(pthread_cond_init(&cond, Conditional::initializer()) == 0, "conditional init failed");
	crit(pthread_mutex_init(&mutex, NULL) == 0, "mutex init failed");
#endif
}

TimedEvent::~TimedEvent()
{
#ifndef	__PTH__
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
#endif
}

void TimedEvent::reset(void)
{
	pthread_mutex_lock(&mutex);
	signalled = false;
	set();
	pthread_mutex_unlock(&mutex);
}

void TimedEvent::signal(void)
{
	pthread_mutex_lock(&mutex);
	signalled = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

bool TimedEvent::sync(void) 
{
	timeout_t timeout = get();
	struct timespec ts;

	if(signalled) {
		signalled = false;
		return true;
	}

	if(!timeout)
		return false;

	Conditional::gettimeout(timeout, &ts);

	if(pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT)
		return false;

	signalled = false;
	return true;
}

bool TimedEvent::wait(timeout_t timeout) 
{
	bool result = true;

	pthread_mutex_lock(&mutex);
	operator+=(timeout);
	result = sync();
	pthread_mutex_unlock(&mutex);
	return result;
}
#endif

void TimedEvent::lock(void)
{
	pthread_mutex_lock(&mutex);
}

void TimedEvent::release(void)
{
	pthread_mutex_unlock(&mutex);
}

ConditionalLock::ConditionalLock() :
ConditionalAccess()
{
	contexts = NULL;
}

ConditionalLock::~ConditionalLock()
{
	linked_pointer<Context> cp = contexts, next;
	while(cp) {
		next = cp->getNext();
		delete *cp;
		cp = next;
	}
}

ConditionalLock::Context *ConditionalLock::getContext(void)
{
	Context *slot = NULL;
	pthread_t tid = Thread::self();
	linked_pointer<Context> cp = contexts;

	while(cp) {
		if(cp->count && Thread::equal(cp->thread, tid))
			return *cp;
		if(!cp->count)
			slot = *cp;
		cp.next();
	}
	if(!slot) {
		slot = new Context(&this->contexts);
		slot->count = 0;
	}
	slot->thread = tid;
	return slot;
}

unsigned ConditionalLock::getReaders(void)
{
	unsigned count;

	lock();
	count = sharing;
	unlock();
	return count;
}

unsigned ConditionalLock::getWaiters(void)
{
	unsigned count;

	lock();
	count = pending + waiting;
	unlock();
	return count;
}

void ConditionalLock::Shlock(void)
{
	access();
}

void ConditionalLock::Unlock(void)
{
	release();
}

void ConditionalLock::Exclusive(void)
{
	exclusive();
}

void ConditionalLock::Share(void)
{
	share();
}

void ConditionalLock::modify(void)
{
	Context *context;

	lock();
	context = getContext();

	assert(context && sharing >= context->count);

	sharing -= context->count;
	while(sharing) {
		++pending;
		waitSignal();
		--pending;
	}
	++context->count;
}

void ConditionalLock::commit(void)
{
	Context *context = getContext();
	--context->count;

	if(context->count) {
		sharing += context->count;
		unlock();
	}
	else
		ConditionalAccess::commit();
}

void ConditionalLock::release(void)
{
	Context *context;

	lock();
	context = getContext();
	assert(sharing && context && context->count > 0);
	--sharing;
	--context->count;
	if(pending && !sharing)
		signal();
	else if(waiting && !pending)
		broadcast();
	unlock();
}

void ConditionalLock::access(void)
{
	Context *context;
	lock();
	context = getContext();
	assert(context && (!max_sharing || sharing < max_sharing));

	// reschedule if pending exclusives to make sure modify threads are not
	// starved.

	++context->count;

	while(context->count < 2 && pending) {
		++waiting;
		waitBroadcast();
		--waiting;
	}
	++sharing;
	unlock();
}

void ConditionalLock::exclusive(void)
{
	Context *context;

	lock();
	context = getContext();
	assert(sharing && context && context->count > 0);
	sharing -= context->count;
	while(sharing) {
		++pending;
		waitSignal();
		--pending;
	}
}

void ConditionalLock::share(void)
{
	Context *context = getContext();
	assert(!sharing && context && context->count);
	sharing += context->count;
	unlock();
}

barrier::barrier(unsigned limit) :
Conditional()
{
	count = limit;
	waits = 0;
}

barrier::~barrier()
{
	lock();
	if(waits)
		broadcast();
	unlock();
}

void barrier::set(unsigned limit)
{
	assert(limit > 0);

	lock();
	count = limit;
	if(count <= waits) {
		waits = 0;
		broadcast();
	}
	unlock();
}

void barrier::inc(void)
{
	lock();
	count++;
	if(count <= waits) {
		waits = 0;
		broadcast();
	}
	unlock();
}

unsigned barrier::operator++(void)
{
	unsigned result;
	lock();
	count++;
	if(count <= waits) {
		waits = 0;
		broadcast();
	}
	result = count;
	unlock();
	return result;
}

bool barrier::wait(timeout_t timeout)
{
	bool result;

	Conditional::lock();
	if(!count) {
		Conditional::unlock();
		return true;
	}
	if(++waits >= count) {
		waits = 0;
		Conditional::broadcast();
		Conditional::unlock();
		return true;
	}
	result = Conditional::wait(timeout);
	Conditional::unlock();
	return result;
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
	
LockedPointer::LockedPointer()
{
#ifdef	_MSWINDOWS_
	InitializeCriticalSection(&mutex);
#else
#ifdef	__PTH__
	static pthread_mutex_t lock = PTH_MUTEX_INIT;
#else
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif
	memcpy(&mutex, &lock, sizeof(mutex));
#endif
	pointer = NULL;
}

void LockedPointer::replace(Object *obj)
{
	assert(obj != NULL);

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
ConditionalAccess()
{
	pointer = NULL;
}

SharedPointer::~SharedPointer()
{
}

void SharedPointer::replace(SharedObject *ptr)
{
	modify();
		
	if(pointer)
		delete pointer;
	pointer = ptr;
	if(ptr)
		ptr->commit(this);

	commit();
}

SharedObject *SharedPointer::share(void)
{
	access();
	return pointer;
}

Thread::Thread(size_t size)
{
	stack = size;
	priority = 0;
	init();
}

#if defined(_MSWINDOWS_)

void Thread::setPriority(void)
{
	HANDLE hThread = GetCurrentThread();
	priority += THREAD_PRIORITY_NORMAL;
	if(priority < THREAD_PRIORITY_LOWEST)
		priority = THREAD_PRIORITY_LOWEST;
	else if(priority > THREAD_PRIORITY_HIGHEST)
		priority = THREAD_PRIORITY_HIGHEST;

	SetThreadPriority(hThread, priority);
}
#elif _POSIX_PRIORITY_SCHEDULING > 0

void Thread::setPriority(void)
{
#ifndef	__PTH__
	int policy;
	struct sched_param sp;
	pthread_t ptid = pthread_self();
	int pri = 0;

	if(!priority)
		return;

	if(pthread_getschedparam(ptid, &policy, &sp))
		return;

	if(priority > 0) {
		policy = realtime_policy;
		if(realtime_policy == SCHED_OTHER)
			pri = sp.sched_priority + priority;
		else
			pri = sched_get_priority_min(policy) + priority;
		policy = realtime_policy;
		if(pri > sched_get_priority_max(policy))
			pri = sched_get_priority_max(policy);
	} else if(priority < 0) {
		pri = sp.sched_priority - priority;
		if(pri < sched_get_priority_min(policy))
			pri = sched_get_priority_min(policy);
	}

	sp.sched_priority = pri;
	pthread_setschedparam(ptid, policy, &sp);
#endif
}

#else
void Thread::setPriority(void) {};
#endif

void Thread::concurrency(int level)
{
#if defined(HAVE_PTHREAD_SETCONCURRENCY) && !defined(_MSWINDOWS_)
	pthread_setconcurrency(level);
#endif
}

void Thread::policy(int polid)
{
#if	_POSIX_PRIORITY_SCHEDULING > 0
	realtime_policy = polid;
#endif
}

JoinableThread::JoinableThread(size_t size)
{
#ifdef	_MSWINDOWS_
	joining = INVALID_HANDLE_VALUE;
#else
	running = false;
#endif
	stack = size;
}

DetachedThread::DetachedThread(size_t size)
{
	stack = size;
}

void Thread::sleep(timeout_t timeout)
{
	timespec ts;
	ts.tv_sec = timeout / 1000l;
	ts.tv_nsec = (timeout % 1000l) * 1000000l;
#if defined(__PTH__)
	pth_usleep(timeout * 1000);
#elif defined(HAVE_PTHREAD_DELAY)
	pthread_delay(&ts);
#elif defined(HAVE_PTHREAD_DELAY_NP)
	pthread_delay_np(&ts);
#elif defined(_MSWINDOWS_)
	::Sleep(timeout);
#else
	usleep(timeout * 1000);
#endif
}

void Thread::yield(void)
{
#if defined(_MSWINDOWS_)
	SwitchToThread();
#elif defined(__PTH__)
	pth_yield(NULL);
#elif defined(HAVE_PTHREAD_YIELD_NP)
	pthread_yield_np();
#elif defined(HAVE_PTHREAD_YIELD)
	pthread_yield();
#else
	sched_yield();
#endif
}

void DetachedThread::exit(void)
{
	delete this;
	pthread_exit(NULL);
}

Thread::~Thread()
{
}

JoinableThread::~JoinableThread()
{
	join();
}

DetachedThread::~DetachedThread()
{
}
		
extern "C" {
#ifdef	_MSWINDOWS_
	static unsigned __stdcall exec_thread(void *obj) {
		assert(obj != NULL);

		Thread *th = static_cast<Thread *>(obj);
		th->setPriority();
		th->run();
		th->exit();
		return 0;
	}
#else
	static void *exec_thread(void *obj)
	{
		assert(obj != NULL);

		Thread *th = static_cast<Thread *>(obj);
		th->setPriority();
		th->run();
		th->exit();
		return NULL;
	}
#endif
}

#ifdef	_MSWINDOWS_
void JoinableThread::start(int adj)
{
	if(joining != INVALID_HANDLE_VALUE)
		return;

	priority = adj;

	if(stack == 1)
		stack = 1024;

	joining = (HANDLE)_beginthreadex(NULL, stack, &exec_thread, this, 0, (unsigned int *)&tid);
	if(!joining)
		joining = INVALID_HANDLE_VALUE;
}

void DetachedThread::start(int adj)
{
	HANDLE hThread;;
	
	priority = adj;

	if(stack == 1)
		stack = 1024;

	hThread = (HANDLE)_beginthreadex(NULL, stack, &exec_thread, this, 0, (unsigned int *)&tid);
	CloseHandle(hThread);
}		

void JoinableThread::join(void)
{
	pthread_t self = pthread_self();
	int rc;

	if(joining && equal(tid, self))
		Thread::exit();

	// already joined, so we ignore...
	if(joining == INVALID_HANDLE_VALUE)
		return;

	rc = WaitForSingleObject(joining, INFINITE);
	if(rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED) {
		CloseHandle(joining);
		joining = INVALID_HANDLE_VALUE;
	}	
}

#else

void JoinableThread::start(int adj)
{
	int result;
	
	if(running)
		return;

	priority = adj;

#ifndef	__PTH__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 
	pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
#endif
// we typically use "stack 1" for min stack...
#ifdef	PTHREAD_STACK_MIN
	if(stack && stack < PTHREAD_STACK_MIN)
		stack = PTHREAD_STACK_MIN;
#else
	if(stack && stack < 2)
		stack = 0;
#endif
#ifdef	__PTH__
	pth_attr_t attr = PTH_ATTR_DEFAULT;
	pth_attr_set(attr, PTH_ATTR_JOINABLE);
	tid = pth_spawn(attr, &exec_thread, this);
#else
	if(stack)
		pthread_attr_setstacksize(&attr, stack);
	result = pthread_create(&tid, &attr, &exec_thread, this);
	pthread_attr_destroy(&attr);
	if(!result)
		running = true;
#endif
}

void DetachedThread::start(int adj)
{
	priority = adj;
#ifndef	__PTH__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
#endif
// we typically use "stack 1" for min stack...
#ifdef	PTHREAD_STACK_MIN
	if(stack && stack < PTHREAD_STACK_MIN)
		stack = PTHREAD_STACK_MIN;
#else
	if(stack && stack < 2)
		stack = 0;
#endif
#ifdef	__PTH__
	tid = pth_spawn(PTH_ATTR_DEFAULT, &exec_thread, this);
#else
	if(stack)
		pthread_attr_setstacksize(&attr, stack);
	pthread_create(&tid, &attr, &exec_thread, this);
	pthread_attr_destroy(&attr);
#endif
}

void JoinableThread::join(void)
{
	pthread_t self = pthread_self();

	if(running && equal(tid, self))
		Thread::exit();

	// already joined, so we ignore...
	if(!running)
		return;

#ifdef	__PTH__
	if(pth_join(tid, NULL))
		running = false;
#else
	if(!pthread_join(tid, NULL))
		running = false;
#endif
}

#endif

void Thread::exit(void)
{
	pthread_exit(NULL);
}

queue::member::member(queue *q, Object *o) :
OrderedObject(q)
{
	assert(o != NULL);
	
	o->retain();
	object = o;
}

queue::queue(mempager *p, size_t size) :
OrderedIndex(), Conditional()
{
	assert(size > 0);

	pager = p;
	freelist = NULL;
	used = 0;
	limit = size;
}

queue::~queue()
{
	linked_pointer<member> mp;
	OrderedObject *next;

	if(pager)
		return;
	
	mp = freelist;
	while(is(mp)) {
		next = mp->getNext();
		delete *mp;
		mp = next;
	}

	mp = head;
	while(is(mp)) {
		next = mp->getNext();
		delete *mp;
		mp = next;
	}
}

bool queue::remove(Object *o)
{
	assert(o != NULL);

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
	struct timespec ts;
	bool rtn = true;
    member *member;
    Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(!tail && rtn) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(&ts);
		else
			rtn = false;
	}
    if(rtn && tail) {
        --used;
        member = static_cast<queue::member *>(tail);
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
	struct timespec ts;
	linked_pointer<member> node;
	Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(rtn && !head) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(&ts);
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
	assert(object != NULL);

	bool rtn = true;
	struct timespec ts;
	member *node;
	LinkedObject *mem;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(rtn && limit && used == limit) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(&ts);
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
	assert(o != NULL);

	o->retain();
	object = o;
}

stack::stack(mempager *p, size_t size) :
Conditional()
{
	assert(size > 0);

	pager = p;
	freelist = usedlist = NULL;
	limit = size;
	used = 0;
}

stack::~stack()
{
	linked_pointer<member> mp;
	LinkedObject *next;

	if(pager)
		return;
	
	mp = freelist;
	while(is(mp)) {
		next = mp->getNext();
		delete *mp;
		mp = next;
	}

	mp = usedlist;
	while(is(mp)) {
		next = mp->getNext();
		delete *mp;
		mp = next;
	}
}

bool stack::remove(Object *o)
{
	assert(o != NULL);

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
	struct timespec ts;
    member *member;
    Object *obj = NULL;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(rtn && !usedlist) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(&ts);
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
	assert(object != NULL);

	bool rtn = true;
	struct timespec ts;
	member *node;
	LinkedObject *mem;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(rtn && limit && used == limit) {
		if(timeout == Timer::inf)
			Conditional::wait();
		else if(timeout)
			rtn = Conditional::wait(&ts);
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
	assert(osize > 0 && c > 0);

	size = osize * c;
	objsize = osize;
	count = 0;
	limit = c;

	if(osize) {
		buf = (char *)malloc(size);
		crit(buf != NULL, "buffer alloc failed");
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

void Buffer::copy(void *data)
{
	assert(data != NULL);

	void *ptr = get();
	memcpy(data, ptr, objsize);
	release();
}

bool Buffer::copy(void *data, timeout_t timeout)
{
	assert(data != NULL);

	void *ptr = get(timeout);
	if(!ptr)
		return false;

	memcpy(data, ptr, objsize);
	release();
	return true;
}

void *Buffer::get(timeout_t timeout)
{
	caddr_t dbuf = NULL;
	struct timespec ts;
	bool rtn = true;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(!count && rtn) {
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(&ts);
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
	assert(dbuf != NULL);

	lock();
	while(count == limit)
		wait();
	memcpy(tail, dbuf, objsize);
	tail += objsize;
	if(tail >= (buf + size))
		tail = buf;
	++count;
	signal();
	unlock();
}

bool Buffer::put(void *dbuf, timeout_t timeout)
{
	assert(dbuf != NULL);

	bool rtn = true;
	struct timespec ts;

	if(timeout && timeout != Timer::inf)
		gettimeout(timeout, &ts);

	lock();
	while(count == limit && rtn) {
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(&ts);
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

void SharedObject::commit(SharedPointer *spointer)
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

void Thread::init(void)
{
#ifdef	__PTH__
	static bool initialized = false;

	if(!initialized) {
		pth_init();
		atexit(pthread_shutdown);
		initialized = true;
	}
#endif
}



