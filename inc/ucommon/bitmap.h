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
 * A simple class to perform bitmap manipulation.
 * Bitmaps are used to manage bit-aligned objects, such as network cidr
 * addresses.  This header introduces a common bitmap management class
 * for the ucommon library.
 * @file ucommon/bitmap.h
 */

#ifndef _UCOMMON_BITMAP_H_
#define	_UCOMMON_BITMAP_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT bitmap
{
protected:
	size_t size;

	typedef union
	{
		void *a;
		uint8_t *b;
		uint16_t *w;
		uint32_t *l;
		uint64_t *d;
	}	addr_t;

	addr_t addr;

public:
	typedef enum {
		BMALLOC,
		B8,
		B16,
		B32,
		B64
	} bus_t;

protected:
	bus_t bus;

	unsigned memsize(void);

public:
	bitmap(void *addr, size_t size, bus_t access = B8); 
	bitmap(size_t size);
	~bitmap();

	void clear(void);

	bool get(size_t offset);
	void set(size_t offset, bool bit);
};

END_NAMESPACE

#endif
