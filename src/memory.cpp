#include <config.h>
#include <ucommon/memory.h>
#include <ucommon/thread.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

static size_t cpr_pagesize(void)
{
#ifdef  HAVE_SYSCONF
    return sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
    return PAGESIZE;
#elif defined(PAGE_SIZE)
    return PAGE_SIZE;
#else
    return 1024;
#endif
}

mempager::mempager(size_t ps)
{
#ifdef	HAVE_SYSCONF
	size_t paging = sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
	size_t paging = PAGESIZE;
#elif defined(PAGE_SIZE)
	size_t paging = PAGE_SIZE;
#else
	size_t paging = 1024;
#endif

	pthread_mutex_init(&mutex, NULL);

	if(!ps)
		ps = paging;
	else if(ps > paging)
		ps = (((ps + paging - 1) / paging)) * paging;

#ifdef	HAVE_POSIX_MEMALIGN
	if(ps >= paging)
		align = sizeof(void *);
	else
		align = 0;

	switch(align)
	{
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		align = 0;
	}
#endif
	pagesize = ps;
	count = 0;
	limit = 0;
}

mempager::~mempager()
{
	mempager::purge();
	pthread_mutex_destroy(&mutex);
}

void mempager::purge(void)
{
	page_t *next;
	pthread_mutex_lock(&mutex);	
	while(page) {
		next = page->next;
		free(page);
		page = next;
	}
	pthread_mutex_unlock(&mutex);
	count = 0;
}

mempager::page_t *mempager::pager(void)
{
	page_t *npage = NULL;
	void *addr;

	crit(!limit || count < limit);

#ifdef	HAVE_POSIX_MEMALIGN
	if(align) {
		crit(posix_memalign(&addr, align, pagesize) == 0);
		npage = (page_t *)addr;
		goto use;
	}
#endif
	npage = (page_t *)malloc(pagesize);

use:
	crit(npage != NULL);

	++count;
	npage->used = sizeof(page_t);
	npage->next = page;
	page = npage;
	if((size_t)(npage) % sizeof(void *))
		npage->used += sizeof(void *) - ((size_t)(npage) % sizeof(void 
*));
	return npage;
}

void mempager::dealloc(void *mem)
{
}

void *mempager::alloc_locked(size_t size)
{
	caddr_t mem;
	page_t *p = page;

	crit(size <= (pagesize - sizeof(page_t)));

	while(size % sizeof(void *))
		++size;

	while(p) {
		if(size <= pagesize - p->used)	
			break;
		p = p->next;
	}
	if(!p)
		p = pager();

	mem = ((caddr_t)(p)) + p->used;	
	p->used += size;
	return mem;
}

void *mempager::alloc(size_t size)
{
	void *mem;

	crit(size <= (pagesize - sizeof(page_t)));

	while(size % sizeof(void *))
		++size;

	pthread_mutex_lock(&mutex);
	mem = alloc_locked(size);
	pthread_mutex_unlock(&mutex);
	return mem;
}

char *mempager::dup(const char *str)
{
	if(!str)
		return NULL;
	char *mem = static_cast<char *>(alloc(strlen(str) + 1));
	strcpy(mem, str);
	return mem;
}

char *mempager::dup_locked(const char *str)
{
    if(!str)
        return NULL;
    char *mem = static_cast<char *>(alloc_locked(strlen(str) + 1));
    strcpy(mem, str);
    return mem;
}

void *mempager::dup(void *obj, size_t size)
{
	void *mem = alloc(size);
	memcpy(mem, obj, size);
	return mem;
}

PagerObject::PagerObject() :
LinkedObject(NULL), CountedObject()
{
}

void PagerObject::dealloc(void)
{
	pager->put(this);
}

void PagerObject::release(void)
{
	CountedObject::release();
}

PagerPool::PagerPool(mempager *p) 
{
	pager = p;
	freelist = NULL;
	pthread_mutex_init(&mutex, NULL);
}

PagerPool::~PagerPool()
{
	pthread_mutex_destroy(&mutex);
}

void PagerPool::put(PagerObject *ptr)
{
    pthread_mutex_lock(&mutex);
	ptr->enlist(&freelist);
    pthread_mutex_unlock(&mutex);
}

PagerObject *PagerPool::get(size_t size)
{
	PagerObject *ptr;
	pthread_mutex_lock(&mutex);
	ptr = static_cast<PagerObject *>(freelist);
	if(ptr) 
		freelist = ptr->next;

    pthread_mutex_unlock(&mutex);

	if(!ptr && pager)
		ptr = new((caddr_t)(pager->alloc(size))) PagerObject;
	else if(!ptr && !pager)
		ptr = new(size - sizeof(PagerObject)) PagerObject;
	memset(ptr, 0, size);
	ptr->pager = this;
	return ptr;
}

keyassoc::keydata::keydata(NamedObject **root, char *id, unsigned max) :
NamedObject(root, id, max)
{
	data = NULL;
}

keyassoc::keyassoc(unsigned max, mempager *mem) 
{
	pager = mem;
	if(mem)
		root = (NamedObject **)mem->alloc(sizeof(NamedObject *) * max);
	else
		root = new NamedObject *[max];
	memset(root, 0, sizeof(NamedObject *) * max);
};

keyassoc::~keyassoc()
{
	purge();
}

void keyassoc::purge(void)
{
	if(!root || pager)
		return;

	delete[] root;
	root = NULL;
}

void *keyassoc::get(const char *id)
{
	keydata *kd;

	if(pager)
		pager->lock();
	kd = static_cast<keydata *>(NamedObject::map(root, id, max));

	if(pager)
		pager->unlock();
	if(!kd)
		return NULL;

	return kd->data;
}

void keyassoc::set(char *id, void *d)
{
    keydata *kd;

	if(pager)
		pager->lock();
	kd = static_cast<keydata *>(NamedObject::map(root, id, max));
	if(!kd) {
		if(pager) {
			id = pager->dup_locked(id);
			caddr_t ptr = (caddr_t)pager->alloc_locked(sizeof(keydata));
			kd = new(ptr) keydata(root, id, max);
		}	
		else {
			id = strdup(id);
			kd = new keydata(root, id, max);
		}
	}
	kd->data = d;
	if(pager)
		pager->unlock();
}

void keyassoc::clear(const char *id)
{
    keydata *kd;

	if(pager)
		pager->lock();
	kd = static_cast<keydata *>(NamedObject::map(root, id, max));
	if(kd)
		kd->data = NULL;
	if(pager)
		pager->unlock();
}

size_t keyassoc::minsize(unsigned max, size_t size)
{
	size_t min;

	if(pager)
		min = (sizeof(NamedObject *) * max) + pager->overhead();
	else
		min = size;
	
	if(size < min)
		size = min;
	return size;
}

