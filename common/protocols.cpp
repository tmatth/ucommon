// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/protocols.h>
#include <ucommon/string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace UCOMMON_NAMESPACE;

char *MemoryProtocol::dup(const char *str)
{
    if(!str)
        return NULL;
	size_t len = strlen(str) + 1;
    char *mem = static_cast<char *>(alloc(len));
    String::set(mem, len, str);
    return mem;
}

void *MemoryProtocol::dup(void *obj, size_t size)
{
	assert(obj != NULL);
	assert(size > 0);

    char *mem = static_cast<char *>(alloc(size));
	memcpy(mem, obj, size);
    return mem;
}

void *MemoryProtocol::zalloc(size_t size)
{
	void *mem = alloc(size);

	if(mem)
		memset(mem, 0, size);

	return mem;
}

MemoryRedirect::MemoryRedirect(MemoryProtocol *protocol)
{
	target = protocol;
}

void *MemoryRedirect::_alloc(size_t size)
{
	// a null redirect uses the heap...
	if(!target)
		return malloc(size);

	return target->_alloc(size);
}

