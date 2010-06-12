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
	char *tokens = NULL;
	unsigned count = 0;
	const char *tp;
	const char *array[5];

	String::fill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string_t)"hello" + (string_t)" this is a test";
	assert(ieq("hello this is a test", *mystr));
	assert(ieq("second test", *testing));
	assert(ieq(" is a test", mystr(-10)));
	mystr = "  abc 123 \n  ";
	assert(ieq("abc 123", String::strip(mystr.c_mem(), " \n")));
	String::set(buff, sizeof(buff), "this is \"a test\"");
	while(NULL != (tp = String::token(buff, &tokens, " ", "\"\"")) && count < 4)
		array[count++] = tp;
	assert(count == 3);
	assert(ieq(array[1], "is"));
	assert(ieq(array[2], "a test"));
}
