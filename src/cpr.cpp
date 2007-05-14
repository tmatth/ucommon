#include <config.h>
#include <ucommon/string.h>
#include <ucommon/service.h>
#include <ucommon/socket.h>
#include <errno.h>
#include <stdarg.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>

using namespace UCOMMON_NAMESPACE;

extern "C" uint16_t lsb_getshort(uint8_t *b)
{
	return (b[1] * 256) + b[0];
}
	
extern "C" uint32_t lsb_getlong(uint8_t *b)
{
	return (b[3] * 16777216l) + (b[2] * 65536l) + (b[1] * 256) + b[0];
}

extern "C" uint16_t msb_getshort(uint8_t *b)
{
	return (b[0] * 256) + b[1];
}
	
extern "C" uint32_t msb_getlong(uint8_t *b)
{
	return (b[0] * 16777216l) + (b[1] * 65536l) + (b[2] * 256) + b[3];
}

extern "C" void lsb_setshort(uint8_t *b, uint16_t v)
{
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
}

extern "C" void msb_setshort(uint8_t *b, uint16_t v)
{
	b[1] = v & 0x0ff;
	b[0] = (v / 256) & 0xff;
}

extern "C" void lsb_setlong(uint8_t *b, uint32_t v)
{
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
	b[2] = (v / 65536l) & 0xff;
	b[3] = (v / 16777216l) & 0xff;
}

extern "C" void msb_setling(uint8_t *b, uint32_t v)
{
	b[3] = v & 0x0ff;
	b[2] = (v / 256) & 0xff;
	b[1] = (v / 65536l) & 0xff;
	b[0] = (v / 16777216l) & 0xff;
}


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
