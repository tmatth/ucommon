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
 * Stream related formatting classes.
 * @file ucommon/format.h
 * @author David Sugar <dyfet@gnutelephony.org>
 */

#ifndef _UCOMMON_FORMAT_H_
#define _UCOMMON_FORMAT_H_

#ifndef _UCOMMON_CPR_H_
#include <ucommon/cpr.h>
#endif

NAMESPACE_UCOMMON

/**
 * Used for forming stream output.  We would create a derived class who's
 * constructor creates an internal string object, and a single method to
 * extract that string.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT PrintFormat
{
public:
    /**
     * Extract formatted string for object.
     */
    virtual const char *get(void) const = 0;
};

/**
 * Used for processing input.  We create a derived class that processes a
 * single character of input, and returns a status value.  EOF means it
 * accepts no more input and any value other than 0 is a character to also
 * unget.  Otherwise 0 is good to accept more input.  The constructor is
 * used to reference a final destination object in the derived class.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT InputFormat
{
public:
    /**
     * Extract formatted string for object.
     * @param character code we are pushing.
     * @return 0 to keep processing, EOF if done, or char to unget.
     */
    virtual int put(char code) = 0;
};


END_NAMESPACE

#endif
