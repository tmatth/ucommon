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
 * Basic UCommon Unicode support.
 * This includes computing unicode transcoding and supporting a 
 * UTF8-aware string class (UString).  We may add support for a wchar_t 
 * aware string class as well, as some external api libraries may require 
 * ucs-2 or 4 encoded strings.
 * @file ucommon/unicode.h
 */

#ifndef _UCOMMON_UNICODE_H_
#define	_UCOMMON_UNICODE_H_

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

NAMESPACE_UCOMMON

/**
 * 32 bit unicode character code.  We may extract this from a ucs2 or utf8
 * string.
 */
typedef	uint32_t	ucs4_t;

/**
 * 16 bit unicode character code.  Java and some api's like these.
 */
typedef	uint16_t	ucs2_t;

/**
 * A core class of ut8 encoded string functions.  This is a foundation for 
 * all utf8 string processing.
 * @author David Sugar
 */
class __EXPORT utf8
{
public:
	/**
	 * Compute character size of utf8 string codepoint.
	 * @param codepoint in string.
	 * @return size of codepoint as utf8 encoded data, 0 if invalid.
	 */
	static unsigned size(const char *codepoint);

	/**
	 * Count ut8 encoded ucs4 codepoints in string.
	 * @param string of utf8 data.
	 * @return codepount count, 0 if empty or invalid.
	 */
	static size_t count(const char *string);

	/**
	 * Get codepoint offset in a string.
	 * @param string of utf8 data.
	 * @param position of codepoint in string, negative offsets are from tail.
	 * @return offset of codepoint or NULL if invalid.
	 */
	static char *offset(char *string, ssize_t position);

	/**
	 * Convert a utf8 encoded codepoint to a ucs4 character value.
	 * @param encoded utf8 codepoint.
	 * @return ucs4 string or 0 if invalid.
	 */
	static ucs4_t codepoint(const char *encoded);
};

END_NAMESPACE

#endif
