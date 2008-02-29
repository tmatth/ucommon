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

#ifndef	DEBUG
#define	DEBUG
#endif

#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

static string_t testing("second test");

extern "C" int main()
{
	char buff[33];
	String::fill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string_t)"hello" + (string_t)" this is a test";
	assert(!stricmp("hello this is a test", *mystr));
	assert(!stricmp("second test", *testing));
	assert(!stricmp(" is a test", mystr(-10)));
	mystr = "  abc 123 \n  ";
	assert(!stricmp("abc 123", String::strip(mystr.c_mem(), " \n")));
};
