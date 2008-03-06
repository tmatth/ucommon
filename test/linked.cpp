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

#ifndef	DEBUG
#define	DEBUG
#endif

#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

typedef linked_value<int> ints;

static OrderedIndex list;

extern "C" int main()
{
	linked_pointer<ints> ptr;
	unsigned count = 0;
	// Since value templates pass by reference, we must have referencable
	// objects or variables.  This avoids passing values by duplicating
	// objects onto the stack frame, though it causes problems for built-in
	// types...
	int xv = 3, xn = 5;
	ints v1(&list, xv);
	ints v2(&list);
	v2 = xn;

	ptr = &list;
	while(ptr) {
		switch(++count)
		{
		case 1:
			assert(ptr->value == 3);
			break;
		case 2:
			assert(ptr->value == 5);
		}
		++ptr;
	}
	assert(count == 2);
};
