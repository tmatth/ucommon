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

static unsigned reused = 0;

class myobject : public ReusableObject
{
public:
	unsigned count;

	myobject()
		{count = ++reused;};
};

mempager pool;
static paged_reuse<myobject> myobjects(&pool, 100);
static queueof<myobject, &pool> mycache(10);

extern "C" int main()
{
	myobject *x;
	for(unsigned i = 0; i < 10; ++i) {
		x = init<myobject>(myobjects.get());
		mycache.post(x);
	}
	assert(x->count == 10);

	for(unsigned i = 0; i < 3; ++i) {
		x = mycache.lifo();
		assert(x != NULL);
	}
	assert(x->count == 8);

	init<myobject>(x);
	assert(x->count == 11);
	return 0;
}

