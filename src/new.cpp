#include <private.h>
#include <inc/config.h>
#include <stdlib.h>

void *operator new(size_t size, caddr_t addr, size_t max)
{
	crit((size <= max));
	return addr;
}

void *operator new[](size_t size, caddr_t addr, size_t max)
{
	crit((size <= max));
    return addr;
}

void *operator new(size_t size, size_t extra)
{
	size += extra;
	return operator new(size);
}

void *operator new(size_t size)
{
	if(!size)
		++size;

	void *mem = malloc(size);
	crit(mem != NULL);
	return mem;
}

void *operator new[](size_t size)
{
	if(!size)
		++size;

	void *mem = malloc(size);
	crit(mem != NULL);
	return mem;
}

void *operator new(size_t size, caddr_t place)
{
	crit(place != NULL);
	return place;
}

void *operator new[](size_t size, caddr_t place)
{
	crit(place != NULL);
    return place; 
}

void operator delete(void *mem)
{
	::free(mem);
}

void operator delete[](void *mem)
{
	::free(mem);
}

#ifdef	__GNUC__

extern "C" void __cxa_pure_virtual(void)
{
	abort();
}

#endif

// vim: set ts=4 sw=4:

