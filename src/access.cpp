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

#include <config.h>
#include <ucommon/access.h>

using namespace UCOMMON_NAMESPACE;

Shared::~Shared()
{
}

Exclusive::~Exclusive()
{
}

void Shared::Exclusive(void)
{
}

void Shared::Share(void)
{
}

shared_lock::shared_lock(Shared *obj)
{
	lock = obj;
	modify = false;
	lock->Shlock();

};

exclusive_lock::exclusive_lock(Exclusive *obj)
{
	lock = obj;
	lock->Exlock();
}

shared_lock::~shared_lock()
{
	if(lock) {
		if(modify)
			lock->Share();
		lock->Unlock();
		lock = NULL;
		modify = false;
	}
}

exclusive_lock::~exclusive_lock()
{
	if(lock) {
		lock->Unlock();
		lock = NULL;
	}
}

void shared_lock::release()
{
	if(lock) {
		if(modify)
			lock->Share();
		lock->Unlock();
		lock = NULL;
		modify = false;
	}
}

void exclusive_lock::release()
{
    if(lock) {
        lock->Unlock();
        lock = NULL;
    }
}

void shared_lock::exclusive(void)
{
	if(lock && !modify) {
		lock->Exclusive();
		modify = true;
	}
}

void shared_lock::share(void)
{
	if(lock && modify) {
		lock->Share();
		modify = false;
	}
}

