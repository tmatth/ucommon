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

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/access.h>

using namespace UCOMMON_NAMESPACE;

SharedProtocol::~SharedProtocol()
{
}

ExclusiveProtocol::~ExclusiveProtocol()
{
}

void SharedProtocol::exclusive(void)
{
}

void SharedProtocol::share(void)
{
}

shared_lock::shared_lock(SharedProtocol *obj)
{
    assert(obj != NULL);
    lock = obj;
    modify = false;
    lock->shared_lock();
}

exclusive_lock::exclusive_lock(ExclusiveProtocol *obj)
{
    assert(obj != NULL);
    lock = obj;
    lock->exclusive_lock();
}

shared_lock::~shared_lock()
{
    if(lock) {
        if(modify)
            lock->share();
        lock->release_share();
        lock = NULL;
        modify = false;
    }
}

exclusive_lock::~exclusive_lock()
{
    if(lock) {
        lock->release_exclusive();
        lock = NULL;
    }
}

void shared_lock::release()
{
    if(lock) {
        if(modify)
            lock->share();
        lock->release_share();
        lock = NULL;
        modify = false;
    }
}

void exclusive_lock::release()
{
    if(lock) {
        lock->release_exclusive();
        lock = NULL;
    }
}

void shared_lock::exclusive(void)
{
    if(lock && !modify) {
        lock->exclusive();
        modify = true;
    }
}

void shared_lock::share(void)
{
    if(lock && modify) {
        lock->share();
        modify = false;
    }
}

