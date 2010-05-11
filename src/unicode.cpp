// Copyright (C) 2009-2010 David Sugar, Tycho Softworks.
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

#include <config.h>
#include <ucommon/string.h>
#include <ucommon/unicode.h>
#include <ucommon/thread.h>
#include <ucommon/socket.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef	HAVE_WCHAR_H
#include <wchar.h>
#else
typedef	ucs4_t	wchar_t;
#endif

using namespace UCOMMON_NAMESPACE;

const char *utf8::nil = NULL;

unsigned utf8::size(const char *string)
{
	unsigned char v = (unsigned char)(*string);

	if(v < 0x80)
		return 1;

	if((v & 0xe0) == 0xc0)
		return 2;

	if((v & 0xf0) == 0xe0)
		return 3;

	if((v & 0xf8) == 0xf0)
		return 4;

	if((v & 0xfc) == 0xf8)
		return 5;

	if((v & 0xfe) == 0xfc)
		return 6;

	return 0;
}

ucs4_t utf8::codepoint(const char *string)
{
	unsigned codesize = size(string);
	unsigned char encoded = (unsigned char)(*(string++));
	ucs4_t code = 0;

	if(encoded == 0)
		return 0;

	if(!codesize)
		return -1;

	switch(codesize)
	{
	case 1:
		return (ucs4_t)encoded;
	case 2:
		code = encoded & 0x1f;
		break;
	case 3:
		code = encoded & 0x0f;
		break;
	case 4:
		code = encoded & 0x07;
		break;
	case 5:
		code = encoded & 0x03;
		break;
	case 6:
		code = encoded & 0x01;
		break;
	}

	while(--codesize) {
		encoded = (unsigned char)(*(string++));
		// validity check...
		if((encoded & 0xc0) != 0x80)
			return 0;
		code = (code << 6) | (encoded & 0x3f);
	}
	return code;
}

size_t utf8::count(const char *string)
{
	size_t pos = 0;
	unsigned codesize;

	if(!string)
		return 0;

	while(*string && (codesize = size(string) != 0)) {
		pos += codesize;
		string += codesize;
	}

	return pos;
}

char *utf8::offset(char *string, ssize_t pos)
{
	if(!string)
		return NULL;

	ssize_t codepoints = count(string);
	if(pos > codepoints)
		return NULL;

	if(pos == 0)
		return string;

	if(pos < 0) {
		pos = -pos;
		if(pos > codepoints)
			return NULL;

		pos = codepoints - pos;
	}

	while(pos--) {
		unsigned codesize = size(string);
		if(!codesize)
			return NULL;

		string += codesize;
	}
	return string;
}

size_t utf8::chars(const unicode_t str)
{
	size_t ccount = 0;
	const wchar_t *string = (const wchar_t *)str;

	if(!string)
		return 0;

	while(*string != 0l) {
		ucs4_t chr = (ucs4_t)(*(string++));
		ccount += chars(chr);
	}
	return ccount;
}

size_t utf8::chars(ucs4_t code)
{
	if(code <= 0x80)
		return 1;
	if(code <= 0x000007ff)
		return 2;
	if(code <= 0x0000ffff)
		return 3;
	if(code <= 0x001fffff)
		return 4;
	if(code <= 0x03ffffff)
		return 5;

	return 6;
}

size_t utf8::extract(const char *text, unicode_t buffer, size_t len)
{
	size_t used = 0;
	wchar_t *target = (wchar_t *)buffer;

	assert(len > 0);

	if(!text) {
		*target = 0;
		return 0;
	}

	while(--len) {
		ucs4_t code = codepoint(text);
		if(code < 1)
			break;
		text += size(text);
		*(target++) = (wchar_t)code;
		++used;
	}

	*target = 0;
	return used;
}

size_t utf8::convert(const unicode_t str, char *buffer, size_t size)
{
	unsigned used = 0;
	unsigned points = 0;
	ucs4_t code;
	const wchar_t *string = (const wchar_t *)str;
	unsigned cs;
	if(!str) {
		*buffer = 0;
		return 0;
	}

	while(0 != (code = (*(string++)))) {
		cs = chars(code);
		if(cs + used > (size - 1))
			break;
		++points;
		if(code < 0x80) {
			buffer[used++] = code & 0x7f;
			continue;
		}
		if(code < 0x000007ff) {
			buffer[used++] = (code >> 6) | 0xc0;
			buffer[used++] = (code & 0x3f) | 0x80;
			continue;
		}
		if(code <= 0x0000ffff) {
			buffer[used++] = (code >> 12) | 0xe0;
			buffer[used++] = (code >> 6 & 0x3f) | 0x80;
			buffer[used++] = (code & 0x3f) | 0x80;
			continue;
		}
		if(code <= 0x001fffff) {
			buffer[used++] = (code >> 18) | 0xf0;
			buffer[used++] = (code >> 12 & 0x3f) | 0x80;
			buffer[used++] = (code >> 6  & 0x3f) | 0x80;
			buffer[used++] = (code & 0x3f) | 0x80;
			continue;
		}
		if(code <= 0x03ffffff) {
			buffer[used++] = (code >> 24) | 0xf8;
			buffer[used++] = (code >> 18 & 0x3f) | 0x80;
			buffer[used++] = (code >> 12 & 0x3f) | 0x80;
			buffer[used++] = (code >> 6  & 0x3f) | 0x80;
			buffer[used++] = (code & 0x3f) | 0x80;
		}
		buffer[used++] = (code >> 30) | 0xfc;
		buffer[used++] = (code >> 24 & 0x3f) | 0x80;
		buffer[used++] = (code >> 18 & 0x3f) | 0x80;
		buffer[used++] = (code >> 12 & 0x3f) | 0x80;
		buffer[used++] = (code >> 6  & 0x3f) | 0x80;
		buffer[used++] = (code & 0x3f) | 0x80;
	}
	buffer[used++] = 0;
	return points;
}

unsigned utf8::ccount(const char *cp, ucs4_t code)
{
	unsigned total = 0;
	ucs4_t ch;
	unsigned cs;

	if(!cp)
		return 0;

	while(*cp) {
		ch = utf8::codepoint(cp);
		cs = utf8::size(cp);
		if(!cs || ch == -1)
			break;
		if(ch == code)
			++total;
		cp += cs;
	}
	return total;
}	

const char *utf8::find(const char *cp, ucs4_t code, size_t pos)
{
	ucs4_t ch;
	unsigned cs;
	size_t cpos = 0;

	if(!cp)
		return NULL;

	while(*cp) {
		ch = utf8::codepoint(cp);
		cs = utf8::size(cp);
		if(pos && ++cpos > pos)
			return NULL;
		if(!cs || ch == -1)
			return NULL;
		if(ch == code)
			return cp;
		cp += cs;
	}
	return NULL;
}

const char *utf8::rfind(const char *cp, ucs4_t code, size_t pos)
{
	const char *result = NULL;
	ucs4_t ch;
	unsigned cs;
	size_t cpos = 0;

	if(!cp)
		return NULL;
		
	while(*cp) {
		ch = utf8::codepoint(cp);
		cs = utf8::size(cp);

		if(!cs || ch == -1)
			break;

		if(ch == code)
			result = cp;

		if(++cpos > pos) 
			break;

		cp += cs;
	}
	return result;
}

size_t Unicode::len(unicode_t text)
{
	wchar_t *cp = (wchar_t *)text;
	if(!text)
		return 0;

	size_t count = 0;
	while(*(cp++))
		++count;

	return count;
}

unicode_t Unicode::dup(unicode_t source)
{
	if(!source)
		return NULL;

	unicode_t ptr;
	size_t size = Unicode::len(source);
	size = ++size * sizeof(wchar_t);
	
	ptr = (unicode_t)malloc(size);
	if(ptr)
		memcpy(ptr, source, size);
	return ptr;
}

unicode_t Unicode::set(unicode_t buf, size_t size, unicode_t src)
{
	assert(buf != NULL);
	assert(size != 0);

	wchar_t *tp = (wchar_t *)buf;
	wchar_t *sp = (wchar_t *)src;
	size_t count = 1;

	while(count++ < size && *sp && sp) {
		*(tp++) = *(tp++);
	}

	*tp = 0;
	return buf;
}

unicode_t Unicode::add(unicode_t buf, size_t size, unicode_t src)
{
	assert(buf != NULL);
	assert(size != 0);

	wchar_t *tp = (wchar_t *)buf;
	wchar_t *sp = (wchar_t *)src;
	size_t count = Unicode::len(buf);

	tp += count;
	++count;

	while(count++ < size && *sp && sp) {
		*(tp++) = *(tp++);
	}

	*tp = 0;
	return buf;
}

unicode_t Unicode::find(unicode_t str, ucs4_t code, size_t pos)
{
	ucs4_t ch;
	wchar_t *cp = (wchar_t *)str;
	size_t cpos = 0;

	if(!cp)
		return NULL;

	while(*cp) {
		ch = *(cp++);
		if(pos && ++cpos > pos)
			return NULL;
		if(ch == code)
			return (unicode_t)cp;
	}
	return NULL;
}

unicode_t Unicode::rfind(unicode_t str, ucs4_t code, size_t pos)
{
	wchar_t *cp = (wchar_t*)str;
	unicode_t result = NULL;
	ucs4_t ch;
	size_t cpos = 0;

	if(!cp)
		return NULL;
		
	while(*cp) {
		ch = *(cp++);

		if(ch == code)
			result = cp;

		if(++cpos > pos) 
			break;
	}
	return result;
}

unicode_t Unicode::token(unicode_t *ptr, unicode_t delim, unicode_t quote, unicode_t end)
{
	if(!ptr || *ptr == NULL)
		return NULL;

	bool ending = false;
	wchar_t qf = 0;
	wchar_t *cp = (wchar_t *)(*ptr);
	unicode_t result;
	while(!ending && *cp && Unicode::find(delim, (ucs4_t)(*cp))) {
		if(end && Unicode::find(end, (ucs4_t)(*cp)))
			ending = true;
		else
			++cp;
	}

	wchar_t *scan = (wchar_t *)quote;
	while(scan && !qf && *scan) {
		if(*cp == *(scan++)) {
			++cp;
			qf = *(scan++);
		}
		else if(*scan)
			++scan;
	}

	*ptr = result = (unicode_t)cp;

	while(!ending && *cp && !Unicode::find(delim, (ucs4_t)(*cp))) {
		if(qf == *cp) {
			qf = 0;
			break;
		}
		if(end && Unicode::find(end, (ucs4_t)(*cp)))
			ending = true;
		else
			++cp;
	}

	if(ending)
		*cp = 0;
	else if(*cp)
		*(cp++) = 0;

	*ptr = (unicode_t *)cp;
	return result;
}
	
UString::UString() 
{
	str = NULL;
}

UString::~UString() {}

UString::UString(strsize_t size)
{
	str = create(size);
	str->retain();
}

UString::UString(const char *text, strsize_t size) 
{
	str = NULL;
	String::set(0, text, size);
}

UString::UString(const unicode_t text) 
{
	str = NULL;
	set(text);
}

UString::UString(const UString& copy) 
{
	str = NULL;
	if(copy.str)
		String::set(copy.str->text);
}

void UString::set(const unicode_t text)
{
	strsize_t size = utf8::chars(text); 
	str = NULL;
	str = create(size);
	str->retain();
	utf8::convert(text, str->text, str->max);
	str->len = size;
	str->fix();
}

void UString::add(const unicode_t text)
{
	strsize_t alloc, size;
	
	size = alloc = utf8::chars(text); 
	if(str)
		alloc += str->len;

	if(!resize(alloc))
		return;

	utf8::convert(text, str->text + str->len, size + 1);
	str->fix();
}

UString UString::get(strsize_t pos, strsize_t size) const
{
	if(!str)
		return UString("", 0);

	char *substr = utf8::offset(str->text, (ssize_t)pos);	
	if(!substr)
		return UString("", 0);

	if(!size)
		return UString(substr, 0);

	const char *end = utf8::offset(substr, size);
	if(!end)
		return UString(substr);

	pos = (end - substr + 1);
	return UString(substr, pos);
}	

ucs4_t UString::at(int offset) const
{
	const char *cp;

	if(!str)
		return -1;

	cp = utf8::offset(str->text, offset);

	if(!cp)
		return -1;

	return utf8::codepoint(cp);
}

const char *UString::find(ucs4_t code, strsize_t pos) const
{
	if(!str)
		return NULL;

	return utf8::find(str->text, code, (size_t)pos);
}

const char *UString::rfind(ucs4_t code, strsize_t pos) const
{
	if(!str)
		return NULL;

	return utf8::rfind(str->text, code, (size_t)pos);
}
		
unsigned UString::ccount(ucs4_t code) const
{
	if(!str)
		return 0;

	return utf8::ccount(str->text, code);
}	

UString UString::operator()(int codepoint, strsize_t size) const
{
	return UString::get(codepoint, size);
}

const char *UString::operator()(int offset) const
{
	if(!str)
		return NULL;

	return utf8::offset(str->text, offset);
}

utf8_pointer::utf8_pointer()
{
	text = NULL;
}

utf8_pointer::utf8_pointer(const char *str)
{
	text = (uint8_t*)str;
}

utf8_pointer::utf8_pointer(const utf8_pointer& copy)
{
	text = copy.text;
}

void utf8_pointer::inc(void)
{
	if(!text)
		return;

	if(*text < 0x80) {
		++text;
		return;
	}

	if((*text & 0xc0) == 0xc0)
		++text;

	while((*text & 0xc0) == 0x80)
		++text;
}

void utf8_pointer::dec(void)
{
	if(!text)
		return;

	while((*(--text) & 0xc0) == 0x80)
		;
}

utf8_pointer& utf8_pointer::operator++()
{
	inc();
	return *this;
}

utf8_pointer& utf8_pointer::operator--()
{
	dec();
	return *this;
}

utf8_pointer& utf8_pointer::operator+=(long offset)
{
	if(!text || !offset)
		return *this;

	if(offset > 0) {
		while(offset--)
			inc();
	}
	else {
		while(offset++)
			dec();
	}
	return *this;
}

utf8_pointer& utf8_pointer::operator-=(long offset)
{
	if(!text || !offset)
		return *this;

	if(offset > 0) {
		while(offset--)
			dec();
	}
	else {
		while(offset++)
			inc();
	}
	return *this;
}

utf8_pointer utf8_pointer::operator+(long offset) const
{
	utf8_pointer nsp(c_str());
	nsp += offset;
	return nsp;
}

utf8_pointer utf8_pointer::operator-(long offset) const
{
	utf8_pointer nsp(c_str());
	nsp -= offset;
	return nsp;
}

utf8_pointer& utf8_pointer::operator=(const char *str)
{
	text = (uint8_t *)str;
	return *this;
}

ucs4_t utf8_pointer::operator[](long offset) const
{
	utf8_pointer ncp(c_str());

	if(!text)
		return 0l;

	if(!offset)
		return utf8::codepoint((const char*)text);

	if(offset > 0) {
		while(offset--)
			ncp.inc();
	}
	else {
		while(offset++)
			ncp.dec();
	}
	return *ncp;
}

unicode_pointer::unicode_pointer()
{
	text = NULL;
}

unicode_pointer::unicode_pointer(unicode_t str)
{
	text = str;
}

unicode_pointer::unicode_pointer(const unicode_pointer& copy)
{
	text = copy.text;
}

unicode_pointer& unicode_pointer::operator ++()
{
	wchar_t *cp = (wchar_t *)text;

	if(cp != NULL)
		++cp;
	text = (unicode_t)cp;
}

unicode_pointer& unicode_pointer::operator --()
{
	wchar_t *cp = (wchar_t *)text;

	if(cp != NULL)
		--cp;
	text = (unicode_t)cp;
}

unicode_pointer& unicode_pointer::operator+=(long offset)
{
	if(!text)
		return *this;

	wchar_t *cp = (wchar_t *)text;
	while(offset > 0) {
		--offset;
		++cp;
	}
	while(offset < 0) {
		++offset;
		--cp;
	}
	text = (unicode_t)cp;
	return *this;
}

unicode_pointer& unicode_pointer::operator-=(long offset)
{
	if(!text)
		return *this;

	wchar_t *cp = (wchar_t *)text;
	while(offset > 0) {
		--offset;
		--cp;
	}
	while(offset < 0) {
		++offset;
		++cp;
	}
	text = (unicode_t)cp;
	return *this;
}

unicode_pointer unicode_pointer::operator+(long offset) const
{
	unicode_pointer ncp(text);

	return (ncp += offset);
}

unicode_pointer unicode_pointer::operator-(long offset) const
{
	unicode_pointer ncp(text);

	return (ncp -= offset);
} 

ucs4_t unicode_pointer::operator[](long offset) const
{
	wchar_t *cp = (wchar_t *)text;

	if(!text)
		return 0;

	return (ucs4_t)cp[offset];
}

unicode_pointer& unicode_pointer::operator=(unicode_t data)
{
	text = data;
	return *this;
}

ucs4_t unicode_pointer::operator*() const
{
	wchar_t *cp = (wchar_t *)text;
	return (ucs4_t)(*cp);
}

