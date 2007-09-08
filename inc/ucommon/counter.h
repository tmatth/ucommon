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
 * Support for various automatic counting objects.
 * This header defines templates for various kinds of automatic counting
 * and sequencing objects.  Templates are used to allow manipulation of
 * various numerical-like types.
 * @file ucommon/counter.h
 */

#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT counter
{
private:
	unsigned value, cycle;

public:
	counter();

	counter(unsigned limit);

	unsigned get(void);

	inline unsigned range(void)
		{return cycle;};

	inline unsigned operator*()
		{return get();};

	inline operator unsigned()
		{return get();};

	void operator=(unsigned v);
};

class __EXPORT SeqCounter : public counter
{
private:
	void *item;
	size_t offset;
	
protected:
	SeqCounter(void *start, size_t size, unsigned count);

	void *get(void);

	void *get(unsigned idx);

public:
	inline void operator=(unsigned v)
		{counter::operator=(v);};
};
	
class __EXPORT toggle
{
private:
	bool value;

public:
	inline toggle()
		{value = false;};

	bool get(void);

	inline bool operator*()
		{return get();};

	inline void operator=(bool v)
		{value = v;};

	inline operator bool()
		{return get();};

};

template <class T>
class sequence : public SeqCounter
{
protected:
	inline T *get(unsigned idx)
		{return static_cast<T *>(SeqCounter::get(idx));};

public:
	inline sequence(T *list, unsigned max) : 
		SeqCounter(list, sizeof(T), max) {};

	inline T* get(void)
		{return static_cast<T *>(SeqCounter::get());};

	inline T& operator*()
		{return *get();};

	inline operator T&()
		{return *get();};

	inline T& operator[](unsigned idx)
		{return *get(idx);};
};

typedef	counter counter_t;
typedef	toggle toggle_t;

END_NAMESPACE

#endif
