#include <private.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/linked.h>
#include <inc/memory.h>

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
	release();
}

void mempager::release()
{
	page_t *next;	
	while(page) {
		next = page->next;
		free(page);
		page = next;
	}
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

keyassoc::keydata::keydata(NamedObject **root, const char *id, unsigned max) :
NamedObject(root, id, max)
{
	data = NULL;
}

keyassoc::keyassoc(unsigned max, size_t paging) :
mempager(minsize(max, paging))
{
	root = static_cast<NamedObject **>(alloc(sizeof(NamedObject *) * max));
	memset(root, 0, sizeof(NamedObject *) * max);
};

keyassoc::~keyassoc()
{
	release();
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
		id = dup(id);
		kd = new(this) keydata(root, id, max);
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
	size_t min = (sizeof(NamedObject *) * max) + overhead();
	if(size < min)
		size = min;
	return size;
}

void *operator new(size_t size, mempager *pager)
{
	void *mem = pager->alloc(size);

	crit(mem != NULL);
	return mem;
}

void *operator new(size_t size, mempager *pager, size_t overdraft)
{
    void *mem = pager->alloc(size + overdraft);

    crit(mem != NULL);
    return mem;
}

void *operator new(size_t size, keyassoc *pager, const char *id)
{
	void *mem = pager->get(id);
	if(mem)
		return mem;

	mem = pager->alloc(size);
	crit(mem != NULL);

	pager->set(id, mem);
	return mem;
}

void *operator new[](size_t size, keyassoc *pager, const char *id)
{
    void *mem = pager->get(id);
    if(mem)
        return mem;

    mem = pager->alloc(size);
	crit(mem != NULL);

    pager->set(id, mem);
    return mem;
}

void *operator new[](size_t size, mempager *pager)
{
    void *mem = pager->alloc(size);

	crit(mem != NULL);
    return mem;
}

