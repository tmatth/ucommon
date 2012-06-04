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

#include "../config.h"
#include <ucommon/export.h>
#include <ucommon/access.h>

using namespace UCOMMON_NAMESPACE;

SharedProtocol::~SharedProtocol()
{
}

ExclusiveProtocol::~ExclusiveProtocol()
{
}

void SharedProtocol::Exclusive(void)
{
}

void SharedProtocol::Share(void)
{
}

shared_lock::shared_lock(SharedProtocol *obj)
{
    assert(obj != NULL);
    lock = obj;
    modify = false;
    lock->Shlock();
}

exclusive_lock::exclusive_lock(ExclusiveProtocol *obj)
{
    assert(obj != NULL);
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

