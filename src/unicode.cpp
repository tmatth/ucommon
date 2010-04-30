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

using namespace UCOMMON_NAMESPACE;

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

	if(!codesize)
		return 0;

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

UString::UString() :
string::string() {};

UString::~UString() {};

UString::UString(const StringFormat& format) :
string::string(format) {};

UString::UString(strsize_t size) :
string::string(size) {};

UString::UString(const char *text) :
string::string(text) {};

UString::UString(const char *text, strsize_t size) :
string::string(text, size) {};

UString::UString(const char *text, const char *end) :
string::string(text, end) {};

UString::UString(const UString& copy) :
string::string(copy) {};

UString UString::get(strsize_t pos, strsize_t size) const
{

	char *substr = utf8::offset(str->text, (ssize_t)pos);	
	if(!substr)
		return UString("");

	if(!size)
		return UString(substr);

	const char *end = utf8::offset(substr, size);
	if(!end)
		return UString(substr);

	pos = (end - substr + 1);
	return UString(substr, pos);
}	

