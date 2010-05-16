// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

#ifndef	DEBUG
#define	DEBUG
#endif

#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

typedef linked_value<int> ints;

static OrderedIndex list;

class member : public DLinkedObject
{
public:
	inline member(unsigned v) : DLinkedObject() {value = v;}

	unsigned value;
};

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

	member ov1 = 1, ov2 = 2, ov3 = 3;

	assert(ov2.value == 2);

	objstack_t st;
	push(st, &ov1);
	push(st, &ov2);
	push(st, &ov3);

	member *mv = (member *)pop(st);
	assert(mv->value == 3);
	pop(st);
	pop(st);
	assert(NULL == pop(st));

	objqueue<member> que;
	que.add(&ov1);
	que.add(&ov2);
	que.add(&ov3);
	mv = que.pop();
	assert(mv->value == 3);
	mv = que.pull();
	assert(mv != NULL);
//	assert(mv->value == 1);

	return 0;
}
