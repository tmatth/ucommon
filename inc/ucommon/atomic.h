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

/**
 * Atomic pointers and locks.  These are meant to use atomic CPU operations
 * and hence offer maximum performance.
 * @file ucommon/atomic.h
 * @author David Sugar <dyfet@gnutelephony.org>
 */

#ifndef _UCOMMON_ATOMIC_H_
#define	_UCOMMON_ATOMIC_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT atomic
{
public:
	class __EXPORT counter
	{
	private:
		volatile long value;
	
	public:
		counter(long initial = 0);

		long operator++();
		long operator--();
		long operator+=(long offset);
		long operator-=(long offset);

		inline operator long()
			{return (long)(value);};

		inline long operator*()
			{return value;};
	};

	class __EXPORT lock
	{
	private:
		volatile long value;

	public:
		lock();

		bool acquire(void);
		void release(void);
	};
};

END_NAMESPACE

#endif
