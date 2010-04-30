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
typedef	int32_t	ucs4_t;

/**
 * 16 bit unicode character code.  Java and some api's like these.
 */
typedef	int16_t	ucs2_t;

/**
 * Resolves issues where wchar_t is not defined.
 */
typedef	void *unicode_t;

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

	/**
	 * How many chars requires to encode a given wchar string.
	 * @param string of ucs4 data.
	 * @return number of chars required to encode given string.
	 */
	static size_t chars(const unicode_t string);

	/**
	 * How many chars requires to encode a given unicode character.
	 * @param character tyo encode.
	 * @return number of chars required to encode given character.
	 */
	static size_t chars(ucs4_t character);

	/**
	 * Convert a unicode string into utf8.
	 * @param string of unicode data.
	 * @param buffer to convert into.
	 * @param size of conversion buffer.
	 * @return number of code points converted.
	 */
	static size_t convert(const unicode_t string, char *buffer, size_t size);

	/**
	 * Convert a utf8 string into a unicode data buffer.
	 * @param utf8 string to copy.
	 * @param unicode data buffer.
	 * @param size of unicode data buffer in codepoints.
	 * @return number of code points converted.
	 */
	static size_t extract(const char *source, unicode_t unicode, size_t size);
};

/**
 * A copy-on-write utf8 string class that operates by reference count.  This
 * is derived from the classic uCommon String class by adding operations that
 * are utf8 encoding aware.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT UString : public String, public utf8
{
protected:
	/**
	 * Create a new empty utf8 aware string object.
	 */
	UString();

	/**
	 * Create an empty string with a buffer pre-allocated to a specified size.
	 * @param size of buffer to allocate.
	 */
	UString(strsize_t size);

	/**
	 * Create a utf8 aware string for a null terminated unicode string.
	 * @param text of ucs4 encoded data.
	 */
	UString(const unicode_t text);

	/**
	 * Create a string from null terminated text up to a maximum specified
	 * size.
	 * @param text to use for string.
	 * @param size limit of new string.
	 */ 
	UString(const char *text, strsize_t size);

	/**
	 * Create a string for a substring.  The end of the substring is a
	 * pointer within the substring itself.
	 * @param text to use for string.
	 * @param end of text in substring.
	 */
	UString(const unicode_t *text, const unicode_t *end);

	/**
	 * Construct a copy of a string object.  Our copy inherets the same 
	 * reference counted instance of cstring as in the original.
	 * @param existing string to copy from.
	 */
	UString(const UString& existing);

	/**
	 * Destroy string.  De-reference cstring.  If last reference to cstring, 
	 * then also remove cstring from heap.
	 */
	virtual ~UString();

	/**
	 * Get a new string object as a substring of the current object.
	 * @param codepoint offset of substring.
	 * @param size of substring in codepoints or 0 if to end.
	 * @return string object holding substring.
	 */
	UString get(strsize_t codepoint, strsize_t size = 0) const;

	/**
	 * Extract a unicode byte sequence from utf8 object.
	 * @param unicode data buffer.
	 * @param size of data buffer.
	 * @return codepoints copied.
	 */
	inline size_t get(unicode_t unicode, size_t size) const
		{return utf8::extract(str->text, unicode, size);};

	/**
	 * Set a utf8 encoded string based on unicode data.
	 * @param unicode text to set.
	 */
	void set(const unicode_t unicode);

	/**
	 * Extract a unicode byte sequence from utf8 object.
	 * @param unicode data buffer.
	 * @param size of data buffer.
	 * @return codepoints copied.
	 */
	inline size_t operator()(unicode_t unicode, size_t size) const
		{return utf8::extract(str->text, unicode, size);};

	/**
	 * Get a new substring through object expression.
	 * @param codepoint offset of substring.
	 * @param size of substring or 0 if to end.
	 * @return string object holding substring.
	 */
	UString operator()(int codepoint, strsize_t size) const;

	/**
	 * Reference a string in the object by codepoint offset.  Positive
	 * offsets are from the start of the string, negative from the
	 * end.
	 * @param offset to string position.
	 * @return pointer to string data or NULL if invalid offset.
	 */
	const char *operator()(int offset) const;

	/**
	 * Reference a unicode character in string object by array offset.
	 * @param offset to character.
	 * @return character value at offset.
	 */
	ucs4_t operator[](int offset) const;
};

/**
 * Convenience type for utf8 encoded strings.
 */
typedef	UString	ustring_t;

END_NAMESPACE

#endif
