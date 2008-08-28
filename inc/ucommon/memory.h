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

/**
 * An alternate memory pager private heap manager.  This is used to allocate
 * in an optimized manner, as it assumes no mutex locks are held or used as
 * part of it's own internal processing.  It also is designed for optimized
 * performance.
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
class __EXPORT memalloc
{
private:
	size_t pagesize, align;
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

	/**
	 * Acquire a new page from the heap.  This is mostly used internally.
	 * @return page structure of the newly aquired memory page.
	 */
	page_t *pager(void);

public:
	/**
	 * Construct a memory pager.
	 * @param page size to use or 0 for OS allocation size.
	 */
	memalloc(size_t page = 0);

	/**
	 * Destroy a memory pager.  Release all pages back to the heap at once.
	 */
	virtual ~memalloc();

	/**
	 * Get the number of pages that have been allocated from the real heap.
	 * @return pages allocated from heap.
	 */
	inline unsigned getPages(void)
		{return count;};

	/**
	 * Get the maximum number of pages that are permitted.  One can use a
	 * derived class to set and enforce a maximum limit to the number of
	 * pages that will be allocated from the real heap.  This is often used
	 * to detect and bring down apps that are leaking.
	 * @return page allocation limit.
	 */
	inline unsigned getLimit(void)
		{return limit;};

	/**
	 * Get the size of a memory page.
	 * @return size of each pager heap allocation.
	 */
	inline unsigned getAlloc(void)
		{return pagesize;};

	/**
	 * Determine fragmentation level of acquired heap pages.  This is
	 * represented as an average % utilization (0-100) and represents the
	 * used portion of each allocated heap page vs the page size.  Since
	 * requests that cannot fit on an already allocated page are moved into
	 * a new page, there is some unusable space left over at the end of the
	 * page.  When utilization approaches 100, this is good.  A low utilization
	 * may suggest a larger page size should be used.
	 * @return pager utilization.
	 */
	unsigned utilization(void);

	/**
	 * Purge all allocated memory and heap pages immediately.
	 */
	void purge(void);

	/**
	 * Allocate memory from the pager heap.  The size of the request must be
	 * less than the size of the memory page used.
	 * @param size of memory request.
	 * @return allocated memory or NULL if not possible.
	 */
	virtual void *alloc(size_t size);

	/**
	 * Allocate memory from the pager heap.  The size of the request must be
	 * less than the size of the memory page used.  The memory is initialized
	 * to zero.
	 * @param size of memory request.
	 * @return allocated memory or NULL if not possible.
	 */
	void *zalloc(size_t size);

	/**
	 * Duplicate NULL terminated string into allocated memory.
	 * @param string to copy into memory.
	 * @return allocated memory with copy of string or NULL if cannot allocate.
	 */
	char *dup(const char *string);

	/**
	 * Duplicate existing memory block into allocated memory.
	 * @param memory to data copy from.
	 * @param size of memory to allocate.
	 * @return allocated memory with copy or NULL if cannot allocate.
	 */
	void *dup(void *memory, size_t size);
};

/**
 * A managed private heap for small allocations.  This is used to allocate
 * a large number of small objects from a paged heap as needed and to then
 * release them together all at once.  This pattern has significiently less
 * overhead than using malloc and offers less locking contention since the 
 * memory pager can also have it's own mutex.  Pager pool allocated memory
 * is always aligned to the optimal data size for the cpu bus and pages are
 * themselves created from memory aligned allocations.  A page size for a
 * memory pager should be some multiple of the OS paging size.
 *
 * The mempager uses a strategy of allocating fixed size pages as needed
 * from the real heap and allocating objects from these pages as needed.
 * A new page is allocated from the real heap when there is insufficient
 * space in the existing page to complete a request.  The largest single
 * memory allocation one can make is restricted by the page size used, and
 * it is best to allocate objects a significent fraction smaller than the
 * page size, as fragmentation occurs at the end of pages when there is
 * insufficent space in the current page to complete a request. 
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
class __EXPORT mempager : public memalloc
{
private:
	pthread_mutex_t mutex;

public:
	/**
	 * Construct a memory pager.
	 * @param page size to use or 0 for OS allocation size.
	 */
	mempager(size_t page = 0);

	/**
	 * Destroy a memory pager.  Release all pages back to the heap at once.
	 */
	virtual ~mempager();

	/**
	 * Lock the memory pager mutex.  It will be more efficient to lock
	 * the pager and then call the locked allocator than using alloc which
	 * seperately locks and unlocks for each request when a large number of
	 * allocation requests are being batched together.
	 */
	inline void lock(void)
		{pthread_mutex_lock(&mutex);};

	/**
	 * Unlock the memory pager mutex.
	 */
	inline void unlock(void)
		{pthread_mutex_unlock(&mutex);};

	/**
	 * Determine fragmentation level of acquired heap pages.  This is
	 * represented as an average % utilization (0-100) and represents the
	 * used portion of each allocated heap page vs the page size.  Since
	 * requests that cannot fit on an already allocated page are moved into
	 * a new page, there is some unusable space left over at the end of the
	 * page.  When utilization approaches 100, this is good.  A low utilization
	 * may suggest a larger page size should be used.
	 * @return pager utilization.
	 */
	unsigned utilization(void);

	/**
	 * Purge all allocated memory and heap pages immediately.
	 */
	void purge(void);

	/**
	 * Return memory back to pager heap.  This actually does nothing, but
	 * might be used in a derived class to create a memory heap that can
	 * also receive (free) memory allocated from our heap and reuse it,
	 * for example in a full private malloc implimentation in a derived class.
	 * @param memory to free back to private heap.
	 */
	virtual void dealloc(void *memory);

	/**
	 * Allocate memory from the pager heap.  The size of the request must be
	 * less than the size of the memory page used.  The memory pager mutex
	 * is locked during this operation and then released.
	 * @param size of memory request.
	 * @return allocated memory or NULL if not possible.
	 */
	void *alloc(size_t size);

	/**
	 * Allocate memory from the pager heap.  The size of the request must be
	 * less than the size of the memory page used.  The memory pager mutex
	 * is locked during this operation and then released.  This version
	 * zeros memory after the mutex lock has been released.
	 * @param size of memory request.
	 * @return allocated memory or NULL if not possible.
	 */
	void *zalloc(size_t size);

	/**
	 * Duplicate NULL terminated string into allocated memory.  The mutex
	 * lock is acquired to perform this operation and then released.
	 * @param string to copy into memory.
	 * @return allocated memory with copy of string or NULL if cannot allocate.
	 */
	char *dup(const char *string);

	/**
	 * Duplicate existing memory block into allocated memory.  The mutex
	 * lock is acquired to perform this operation and then released.
	 * @param memory to data copy from.
	 * @param size of memory to allocate.
	 * @return allocated memory with copy or NULL if cannot allocate.
	 */
	void *dup(void *memory, size_t size);
};

/**
 * Create a linked list of auto-releasable objects.  LinkedObject derived
 * objects can be created that are assigned to an autorelease object list.
 * When the autorelease object falls out of scope, all the objects listed'
 * with it are automatically deleted.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT autorelease
{
private:
	LinkedObject *pool;

public:
	/**
	 * Create an initially empty autorelease pool.
	 */
	autorelease();

	/**
	 * Destroy an autorelease pool and delete member objects.
	 */
	~autorelease();

	/**
	 * Destroy an autorelease pool and delete member objects.  This may be
	 * used to release an existing pool programatically when desired rather
	 * than requiring the object to fall out of scope.
	 */
	void release(void);

	/**
	 * Add a linked object to the autorelease pool.
	 * @param object to add to pool.
	 */
	void operator+=(LinkedObject *object);
};

/**
 * This is a base class for objects that may be created in pager pools.
 * This is also used to create objects which can be maintained as managed
 * memory and returned to a pool.  The linked list is used when freeing
 * and re-allocating the object.  These objects are reference counted
 * so that they are returned to the pool they come from automatically
 * when falling out of scope.  This can be used to create automatic
 * garbage collection pools.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT PagerObject : public LinkedObject, public CountedObject
{
protected:
	friend class PagerPool;

	PagerPool *pager;

	/**
	 * Create a pager object.  This is object is constructed by a PagerPool.
	 */
	PagerObject();

	/**
	 * Release a pager object reference.
	 */
	void release(void);

	/**
	 * Return the pager object back to it's originating pool.
	 */
	void dealloc(void);
};	

/**
 * Pager pool base class for managed memory pools.  This is a helper base
 * class for the pager template and generally is not used by itself.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
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
	/**
	 * Return a pager object back to our free list.
	 * @param object to return to pool.
	 */
	void put(PagerObject *object);
};

/**
 * A class to hold memory pointers referenced by string names.  This is
 * used to form a typeless data pointer that can be associated and
 * referenced by string/logical name.  The memory used for forming
 * the string names can itself be managed in reusable memory pools and
 * the entire structure uses it's own private pager heap.  This allows
 * new string named pointers to be added and deleted at runtime in a thread-
 * safe manner.  This might typically be used as a session id manager or for
 * symbol tables.
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
class __EXPORT keyassoc : protected mempager
{
private:
	/**
	 * Internal paged memory residing data class for name associated pointers.
	 */
	class __LOCAL keydata : public NamedObject
	{
	public:
		void *data;
		char text[8];

		keydata(keyassoc *assoc, char *id, unsigned max, unsigned bufsize);
	};

	friend class keydata;

	unsigned count;
	unsigned paths;
	size_t keysize;
	NamedObject **root;
	LinkedObject **list;

public:
	/**
	 * Create a key associated memory pointer table.
	 * @param indexing size for hash map.
	 * @param max size of a string name if names are in reusable managed memory.
	 * @param page size of memory pager.
	 */
	keyassoc(unsigned indexing = 177, size_t max = 0, size_t page = 0);

	/**
	 * Destroy association object.  Release all pages back to the heap.
	 */
	~keyassoc();

	/**
	 * Get the number of associations we have in our object.
	 * @return number of associations stored.
	 */
	inline unsigned getCount(void)
		{return count;};

	/**
	 * Lookup the data pointer of a string by direct operation.
	 * @param name to lookup.
	 * @return pointer to data or NULL if not found.
	 */
	inline void *operator()(const char *name)
		{return locate(name);};

	/**
	 * Purge all associations and return allocated pages to heap.
	 */
	void purge(void);

	/**
	 * Lookup the data pointer by the string name given.
	 * @param name to lookup.
	 * @return pointer to data or NULL if not found.
	 */
	void *locate(const char *name);

	/**
	 * Assign a name to a data pointer.  If the name exists, it is re-assigned
	 * with the new pointer value, otherwise it is created.
	 * @param name to assign.
	 * @param pointer value to assign with name.
	 * @return false if failed because name is too long for managed table.
	 */
	bool assign(char *name, void *pointer);

	/**
	 * Create a new name in the association table and assign it's value.
	 * @param name to create.
	 * @param pointer value to assign with name.
	 * @return false if already exists or name is too long for managed table.
	 */
	bool create(char *name, void *pointer);

	/**
	 * Remove a name and pointer association.  If managed key names are used
	 * then the memory allocated for the name will be re-used.
	 * @param name to remove.
	 * @return pointer value of the name or NULL if not found.
	 */
	void *remove(const char *name);
};

/**
 * A typed template for using a key association with typed objects.
 * This essentially forms a form of "smart pointer" that is a reference
 * to specific typed objects by symbolic name.  This is commonly used as
 * for associated indexing of typed objects.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template <class T, unsigned I = 177, size_t M = 0, size_t P = 0>
class assoc_pointer : private keyassoc
{
public:
	/**
	 * Construct an associated pointer hash map based on the class template.
	 */
	inline assoc_pointer() : keyassoc(I, M, P) {};

	/**
	 * Get the count of typed objects stored in our hash map.
	 * @return typed objects in map.
	 */
	inline unsigned getCount(void)
		{return keyassoc::getCount();};

	/**
	 * Purge the hash map of typed objects.
	 */
	inline void purge(void)
		{keyassoc::purge();};

	/**
	 * Lookup a typed object by name.
	 * @param name of typed object to locate.
	 * @return typed object pointer or NULL if not found.
	 */
	inline T *locate(const char *name)
		{return static_cast<T*>(keyassoc::locate(name));};

	/**
	 * Reference a typed object directly by name.
	 * @param name of typed object to locate.
	 * @return typed object pointer or NULL if not found.
	 */
	inline T *operator()(const char *name)
		{return locate(name);};

	/**
	 * Assign a name for a pointer to a typed object.  If the name exists, 
	 * it is re-assigned with the new pointer value, otherwise it is created.
	 * @param name to assign.
	 * @param pointer of typed object to assign with name.
	 * @return false if failed because name is too long for managed table.
	 */
	inline bool assign(char *name, T *pointer)
		{return keyassoc::assign(name, pointer);};

	/**
	 * Create a new name in the association table and assign typed object.
	 * @param name to create.
	 * @param pointer of typed object to assign with name.
	 * @return false if already exists or name is too long for managed table.
	 */
	inline bool create(char *name, T *pointer)
		{return keyassoc::create(name, pointer);};

	/**
	 * Remove a name and typed pointer association.  If managed key names are 
	 * used then the memory allocated for the name will be re-used.
	 * @param name to remove.
	 */
	inline void remove(char *name)
		{keyassoc::remove(name);};

	/**
	 * Access to pager utilization stats.  This is needed because we
	 * inherit keyassoc privately.
	 * @return pager utilization, 0-100.
	 */
	inline unsigned utilization(void)
		{return mempager::utilization();};

	/**
	 * Access to number of pages allocated from heap for our associated
	 * index pointer.  This is needed because we inherit keyassoc
	 * privately.
	 * @return count of heap pages used.
	 */
	inline unsigned getPages(void)
		{return mempager::getPages();};
};

/**
 * Mempager managed type factory for pager pool objects.  This is used to
 * construct a type factory that creates and manages typed objects derived
 * from PagerObject which can be managed through a private heap.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template <class T>
class pager : private PagerPool
{
public:
	/**
	 * Construct a pager and optionally assign a private pager heap.
	 * @param heap pager to use.  If NULL, uses global heap.
	 */
	inline pager(mempager *heap = NULL) : PagerPool(heap) {};

	/**
	 * Purge managed objects.
	 */
	inline ~pager()
		{mempager::purge();};

	/**
	 * Create a managed object by casting reference.
	 * @return pointer to typed managed pager pool object.
	 */
	inline T *operator()(void)
		{return new(get(sizeof(T))) T;};

	/**
	 * Create a managed object by pointer reference.
	 * @return pointer to typed managed pager pool object.
	 */
	inline T *operator*()
		{return new(get(sizeof(T))) T;};
};

END_NAMESPACE

#endif
