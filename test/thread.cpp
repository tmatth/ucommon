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

class mythread : public JoinableThread
{
public:
	inline mythread() : JoinableThread() {};

	void run(void);
};

void mythread::run(void)
{
	printf("starting thread...\n");
	Thread::sleep(10000);
	printf("finishing thread\n");
};

extern "C" int main()
{
	mythread *thr = new mythread();

	printf("before\n");
	start(thr);
	printf("sleeping main...\n");
	Thread::sleep(1000);
	printf("wakeup main...\n");
	cancel(thr);
	printf("joining\n");
	delete thr;
	printf("ending\n");
};
