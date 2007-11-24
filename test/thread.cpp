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

static unsigned count = 0;

class testThread : public JoinableThread
{
public:
	testThread() : JoinableThread() {};

	void run(void) {
		++count;
		sleep(1000);
	};
};

extern "C" int main()
{
	time_t now, later;
	testThread *thr;

	time(&now);
	thr = new testThread();
	start(thr);
	Thread::sleep(10);
	delete thr;
	assert(count == 1);
	time(&later);
	assert(later >= now + 1);
};

