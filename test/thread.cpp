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

#define	DEBUG
#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

class testThread : public JoinableThread
{
public:
	int count;

	testThread() : JoinableThread() {
		count = 0;};

	void run(void) {
		++count;
		sleep(10000);
	};
};

extern "C" int main()
{
	time_t now, later;
	testThread *thr;

	time(&now);
	thr = new testThread();
	start(thr);
	Thread::sleep(1000);
	assert(thr->count == 1);
	cancel(thr);
	delete thr;
	time(&later);
	assert(later == now + 1);
};

