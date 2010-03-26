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
 * Support classes for manipulation of numbers as strings.  This is
 * used for things which parse numbers out of strings, such as in the
 * date and time classes.  Other useful math related functions, templates,
 * and macros may also be found here.
 * @file ucommon/numbers.h
 */

#ifndef _UCOMMON_NUMBERS_H_
#define	_UCOMMON_NUMBERS_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

/**
 * A number manipulation class.  This is used to extract, convert,
 * and manage simple numbers that are represented in C ascii strings
 * in a very quick and optimal way.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short number manipulation.
 */
class __EXPORT Number
{
protected:
	char *buffer;
	unsigned size;

public:
	/**
	 * Create an instance of a number.
 	 * @param buffer or NULL if created internally.
	 * @param size use - values for zero filled.
	 */
	Number(char *buffer, unsigned size);

	void set(long value);

	inline const char *c_str() const
		{return buffer;};

	long get() const;

	inline long operator()()
		{return get();};

	inline operator long()
		{return get();};

	inline operator char*()
		{return buffer;};

	long operator=(const long value);
	long operator+=(const long value);
	long operator-=(const long value);
	long operator--();
	long operator++();
	int operator==(const Number &num);
	int operator!=(const Number &num);
	int operator<(const Number &num);
	int operator<=(const Number &num);
	int operator>(const Number &num);
	int operator>=(const Number &num);

	friend long operator+(const Number &num, const long val);
	friend long operator+(const long val, const Number &num);
	friend long operator-(const Number &num, long val);
	friend long operator-(const long val, const Number &num);
};

class __EXPORT ZNumber : public Number
{
public:
	ZNumber(char *buf, unsigned size);
	void set(long value);
	long operator=(long value);
};

typedef	Number	number_t;
typedef	ZNumber znumber_t;

/**
 * Template for absolute value of a type.
 * @param value to check
 * @return absolute value
 */
template<typename T>
inline const T& abs(const T& v)
{
    if(v < (T)0)
        return -v;
    return v;
}


/**
 * Template for min value of a type.
 * @param v1 value to check
 * @param v2 value to check
 * @return v1 if < v2, else v2
 */
template<typename T>
inline const T& min(const T& v1, const T& v2)
{
	return ((v1 < v2) ? v1 : v2);
}

/**
 * Template for max value of a type.
 * @param v1 value to check
 * @param v2 value to check
 * @return v1 if > v2, else v2
 */
template<typename T>
inline const T& max(const T& v1, const T& v2)
{
	return ((v1 > v2) ? v1 : v2);
}

END_NAMESPACE

#endif
