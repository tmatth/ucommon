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

/**
 * Abstract interfaces and support.  This is a set of "protocols", a concept
 * borrowed from other object oriented languages, to define interfaces for
 * low level services.  By using a protocol base class which offers both
 * virtuals and support methods only, one can easily stack and share these
 * as common base classes without having to consider when the final derived
 * object implements them.  Core protocol methods always are tagged with a
 * _ prefix to make it easier to track their derivation.
 * @file ucommon/protocols.h
 * @author David Sugar <dyfet@gnutelephony.org>
 */

#ifndef _UCOMMON_PROTOCOLS_H_
#define	_UCOMMON_PROTOCOLS_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT MemoryProtocol
{
public:
    /**
     * Protocol to allocate memory from the pager heap.  The size of the 
	 * request must be less than the size of the memory page used.  The
	 * actual method is in a derived or stacked object.
     * @param size of memory request.
     * @return allocated memory or NULL if not possible.
     */
    virtual void *_alloc(size_t size) = 0;

	/**
	 * Convenience function.
	 * @param size of memory request.
	 * @return alocated memory or NULL if not possible.
	 */
	inline void *alloc(size_t size)
		{return _alloc(size);};

    /**
     * Allocate memory from the pager heap.  The size of the request must be
     * less than the size of the memory page used.  The memory is initialized
     * to zero.  This uses alloc.
     * @param size of memory request.
     * @return allocated memory or NULL if not possible.
     */
    void *zalloc(size_t size);

    /**
     * Duplicate NULL terminated string into allocated memory.  This uses
	 * alloc.
     * @param string to copy into memory.
     * @return allocated memory with copy of string or NULL if cannot allocate.
     */
    char *dup(const char *string);

    /**
     * Duplicate existing memory block into allocated memory.  This uses alloc.
     * @param memory to data copy from.
     * @param size of memory to allocate.
     * @return allocated memory with copy or NULL if cannot allocate.
     */
    void *dup(void *memory, size_t size);
};

/**
 * A redirection base class for the memory protocol.  This is used because
 * sometimes we choose a common memory pool to manage different objects.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT MemoryRedirect : public MemoryProtocol
{
private:
	MemoryProtocol *target;

public:
	MemoryRedirect(MemoryProtocol *protocol);

	virtual void *_alloc(size_t size);
};

END_NAMESPACE

#endif
