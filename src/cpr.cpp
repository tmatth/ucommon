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

#include <config.h>

#ifdef	HAVE_STDEXCEPT
#include <stdexcept>
#endif

#include <ucommon/string.h>
#include <ucommon/socket.h>
#include <errno.h>
#include <stdarg.h>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

using namespace UCOMMON_NAMESPACE;

void cpr_runtime_error(const char *str)
{
	assert(str != NULL);

#ifdef	HAVE_STDEXCEPT
	throw std::runtime_error(str);
#endif
	abort();
}

extern "C" uint16_t lsb_getshort(uint8_t *b)
{
	assert(b != NULL);
	return (b[1] * 256) + b[0];
}
	
extern "C" uint32_t lsb_getlong(uint8_t *b)
{
	assert(b != NULL);
	return (b[3] * 16777216l) + (b[2] * 65536l) + (b[1] * 256) + b[0];
}

extern "C" uint16_t msb_getshort(uint8_t *b)
{
	assert(b != NULL);
	return (b[0] * 256) + b[1];
}
	
extern "C" uint32_t msb_getlong(uint8_t *b)
{
	assert(b != NULL);
	return (b[0] * 16777216l) + (b[1] * 65536l) + (b[2] * 256) + b[3];
}

extern "C" void lsb_setshort(uint8_t *b, uint16_t v)
{
	assert(b != NULL);
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
}

extern "C" void msb_setshort(uint8_t *b, uint16_t v)
{
	assert(b != NULL);
	b[1] = v & 0x0ff;
	b[0] = (v / 256) & 0xff;
}

extern "C" void lsb_setlong(uint8_t *b, uint32_t v)
{
	assert(b != NULL);
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
	b[2] = (v / 65536l) & 0xff;
	b[3] = (v / 16777216l) & 0xff;
}

extern "C" void msb_setling(uint8_t *b, uint32_t v)
{
	assert(b != NULL);
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
	crit(mem != NULL, "memory alloc failed");
	return mem;
}

extern "C" void *cpr_memassign(size_t size, caddr_t addr, size_t max)
{
	assert(addr);
	crit((size <= max), "memory assign failed");
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
