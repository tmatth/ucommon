#include <config.h>
#include <ucommon/ucommon.h>
#include <stdlib.h>

extern "C" void *cpr_memalloc(size_t size)
{
	void *mem;

	if(!size)
		++size;

	mem = malloc(size);
	crit(mem != NULL);
	return mem;
}

extern "C" void *cpr_memassign(size_t size, caddr_t addr, size_t max)
{
	if(!addr)
		addr = (caddr_t)malloc(size);
	crit((size <= max));
	return addr;
}

#ifdef	__GNUC__

extern "C" void __cxa_pure_virtual(void)
{
	abort();
}

#endif

// vim: set ts=4 sw=4:
// Local Variables:
// c-basic-offset: 4
// tab-width: 4
// End:

