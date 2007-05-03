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

	void purge(void);
	virtual void dealloc(void *mem);
	void *alloc(size_t size);
	char *dup(const char *cp);
	void *dup(void *mem, size_t size);
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

class __EXPORT keyassoc 
{
private:
	class __EXPORT keydata : public NamedObject
	{
	public:
		void *data;

		keydata(NamedObject **root, char *id, unsigned max);
	};

	unsigned max;
	NamedObject **root;
	mempager *pager;

	size_t minsize(unsigned max, size_t ps);

public:
	keyassoc(unsigned keysize = 177, mempager *pager = NULL);
	~keyassoc();

	void purge(void);
	void *get(const char *id);
	void set(char *id, void *data);
	void clear(const char *id);
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

template <class T>
class keymem
{
private:
	keyassoc *heap;

public:
	inline keymem(keyassoc *h)
		{heap = h;};

	inline T &get(const char *id)
		{return *(new(heap, id) T);};

	inline T *find(const char *id)
		{return static_cast<T*>(get(id));};

	inline void set(const char *id, const T& v)
		{*(new(heap, id) T) = v;};
};

END_NAMESPACE

#endif
