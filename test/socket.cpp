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

static Socket::address localhost("127.0.0.1", 4444);

extern "C" int main()
{
	struct sockaddr_internet addr;

	char addrbuf[65];
	addrbuf[0] = 0;
	Socket::getaddress(localhost.getAddr(), addrbuf, sizeof(addrbuf));
	assert(0 == strcmp(addrbuf, "127.0.0.1"));
	Socket::getinterface((struct sockaddr *)&addr, localhost.getAddr());
	Socket::getaddress((struct sockaddr *)&addr, addrbuf, sizeof(addrbuf));
	assert(0 == strcmp(addrbuf, "127.0.0.1"));
	assert(Socket::equal((struct sockaddr *)&addr, localhost.getAddr()));
	assert(Socket::subnet((struct sockaddr *)&addr, localhost.getAddr()));
};
