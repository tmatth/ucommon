#include <private.h>
#include <ucommon/memory.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

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

void *mempager::alloc(size_t size)
{
	caddr_t mem;
	page_t *p = page;

	crit(size <= (pagesize - sizeof(page_t)));

	while(size % sizeof(void *))
		++size;

	pthread_mutex_lock(&mutex);
	while(p) {
		if(size <= pagesize - p->used)	
			break;
		p = p->next;
	}
	if(!p)
		p = pager();

	mem = ((caddr_t)(p)) + p->used;	
	p->used += size;
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

keyassoc::keydata::keydata(NamedObject **root, const char *id, unsigned max) :
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
	keydata *kd = static_cast<keydata *>(NamedObject::map(root, id, max));
	if(!kd)
		return NULL;

	return kd->data;
}

void keyassoc::set(const char *id, void *d)
{
    keydata *kd = static_cast<keydata *>(NamedObject::map(root, id, max));
	if(!kd) {
		if(pager) {
			id = pager->dup(id);
			caddr_t ptr = (caddr_t)pager->alloc(sizeof(keydata));
			kd = new(ptr) keydata(root, id, max);
		}	
		else {
			id = strdup(id);
			kd = new keydata(root, id, max);
		}
	}
	kd->data = d;
}

void keyassoc::clear(const char *id)
{
    keydata *kd = static_cast<keydata *>(NamedObject::map(root, id, max));
	if(!kd)
		return;
	kd->data = NULL;
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
