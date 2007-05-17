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

class __EXPORT keyassoc : mempager
{
private:
	class __LOCAL keydata : public NamedObject
	{
	public:
		void *data;
		char text[8];

		keydata(keyassoc *assoc, char *id, unsigned max);
	};

	unsigned paths;
	size_t keysize;
	NamedObject **root;
	LinkedObject **list;

public:
	keyassoc(unsigned indexing = 177, size_t keymax = 0, size_t ps = 0);
	~keyassoc();

	void purge(void);
	void *locate(const char *id);
	void assign(char *id, void *data);
	void create(char *id, void *data);
	void *remove(const char *id);
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
