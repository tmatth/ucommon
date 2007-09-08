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

/**
 * Private heaps, pools, and associations.
 * Private heaps often can reduce locking contention in threaded applications
 * since they do not require using the global "malloc" function.  Private
 * heaps also can be used as auto-release heaps, where all memory allocated
 * and parsled out for small objects can be automatically released all at once.
 * Pager pools are used to optimize system allocation around page boundries.
 * Associations allow private memory to be tagged and found by string
 * identifiers.
 * @file ucommon/memory.h
 */

#ifndef	_UCOMMON_MEMORY_H_
#define	_UCOMMON_MEMORY_H_

#ifndef	 _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

NAMESPACE_UCOMMON

class PagerPool;

class __EXPORT mempager
{
private:
	size_t pagesize, align;
	pthread_mutex_t mutex;
	unsigned count;

	typedef struct mempage {
		struct mempage *next;
		union {
			void *memalign;
			unsigned used;		
		};
	}	page_t;

	page_t *page;

protected:
	unsigned limit;

	inline size_t getOverhead(void)
		{return sizeof(page_t);};

	page_t *pager(void);

public:
	virtual void *alloc_locked(size_t size);
	
	char *dup_locked(const char *cp);

	inline void lock(void)
		{pthread_mutex_lock(&mutex);};

	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};
	
	mempager(size_t ps = 0);
	virtual ~mempager();

	inline unsigned overhead(void)
		{return sizeof(page_t);};

	inline unsigned getPages(void)
		{return count;};

	inline unsigned getLimit(void)
		{return limit;};

	inline unsigned getAlloc(void)
		{return pagesize;};

	unsigned utilization(void);

	void purge(void);
	virtual void dealloc(void *mem);
	void *alloc(size_t size);
	char *dup(const char *cp);
	void *dup(void *mem, size_t size);
};

class __EXPORT autorelease
{
private:
	LinkedObject *pool;

public:
	autorelease();
	~autorelease();

	void release(void);

	void operator+=(LinkedObject *obj);
};

class __EXPORT PagerObject : public LinkedObject, public CountedObject
{
protected:
	friend class PagerPool;

	PagerPool *pager;

	PagerObject();

	void release(void);

	void dealloc(void);
};	

class __EXPORT PagerPool 
{
private:
	mempager *pager;
	LinkedObject *freelist;
	pthread_mutex_t mutex;

protected:
	PagerPool(mempager *pager);
	~PagerPool();

	PagerObject *get(size_t size);

public:
	void put(PagerObject *obj);
};

class __EXPORT keyassoc : protected mempager
{
private:
	class __LOCAL keydata : public NamedObject
	{
	public:
		void *data;
		char text[8];

		keydata(keyassoc *assoc, char *id, unsigned max);
	};

	unsigned count;
	unsigned paths;
	size_t keysize;
	NamedObject **root;
	LinkedObject **list;

public:
	keyassoc(unsigned indexing = 177, size_t keymax = 0, size_t ps = 0);
	~keyassoc();

	inline unsigned getCount(void)
		{return count;};

	inline void *operator()(const char *id)
		{return locate(id);};

	void purge(void);
	void *locate(const char *id);
	bool assign(char *id, void *data);
	bool create(char *id, void *data);
	void *remove(const char *id);
};

template <class T, unsigned I = 177, size_t M = 0, size_t P = 0>
class assoc_pointer : private keyassoc
{
public:
	inline assoc_pointer() : keyassoc(I, M, P) {};

	inline unsigned getCount(void)
		{return keyassoc::getCount();};

	inline void purge(void)
		{keyassoc::purge();};

	inline T *locate(const char *id)
		{return static_cast<T*>(keyassoc::locate(id));};

	inline T *operator()(const char *id)
		{return locate(id);};

	inline bool assign(char *id, T *data)
		{return keyassoc::assign(id, data);};

	inline bool create(char *id, T *data)
		{return keyassoc::create(id, data);};

	inline void remove(char *id)
		{keyassoc::remove(id);};

	inline unsigned utilization(void)
		{return mempager::utilization();};

	inline unsigned getPages(void)
		{return mempager::getPages();};

	inline void lock(void)
		{mempager::lock();};

	inline void unlock(void)
		{mempager::unlock();};
};

template <class T>
class pager : private PagerPool
{
public:
	inline pager(mempager *P = NULL) : PagerPool(P) {};

	inline ~pager()
		{mempager::purge();};

	inline T *operator()(void)
		{return new(get(sizeof(T))) T;};

	inline T *operator*()
		{return new(get(sizeof(T))) T;};
};

END_NAMESPACE

#endif
