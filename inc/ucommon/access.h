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

/**
 * Locking classes for member function automatic operations.
 * This header covers ucommon access related classes.  These are used to
 * provide automatic management of locks and sychronization objects through
 * common virtual base classes which can be used with automatic objects. This
 * allows member functions to aquire a lock when entered and automatically
 * release the lock when the member function returns.
 * @file ucommon/access.h
 */

#ifndef _UCOMMON_ACCESS_H_
#define	_UCOMMON_ACCESS_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT Exclusive
{
protected:
	virtual ~Exclusive();

public:
	virtual void Exlock(void) = 0;
	virtual void Unlock(void) = 0;

	inline void Lock(void)
		{return Exlock();};
};

class __EXPORT Shared
{
protected:
	virtual ~Shared();

public:
	virtual void Shlock(void) = 0;
	virtual void Unlock(void) = 0;

	virtual void Share(void);
	virtual void Exclusive(void);

	inline void Lock(void)
		{return Shlock();};
};

class __EXPORT exclusive_lock
{
private:
	Exclusive *lock;
	int state;

public:
	exclusive_lock(Exclusive *l);
	~exclusive_lock();

	bool operator!()
		{return lock == NULL;};

	operator bool()
		{return lock != NULL;};
	
	void release(void);
};

class __EXPORT shared_lock
{
private:
    Shared *lock;
	int state;
	bool modify;

public:
    shared_lock(Shared *l);
    ~shared_lock();

	bool operator!()
		{return lock == NULL;};

	operator bool()
		{return lock != NULL;};

    void release(void);
	void exclusive(void);
	void share(void);
};

inline void lock(Exclusive *ex)
	{ex->Exlock();};

inline void unlock(Exclusive *ex)
	{ex->Unlock();};

inline void access(Shared *sh)
	{sh->Shlock();};

inline void release(Shared *sh)
	{sh->Unlock();};

inline void exclusive(Shared *sh)
	{sh->Exclusive();};

inline void share(Shared *sh)
	{sh->Share();};

typedef	exclusive_lock exlock_t;
typedef	shared_lock shlock_t;

inline void release(exlock_t &ex)
	{ex.release();};

inline void release(shlock_t &sh)
	{sh.release();};

#define	exclusive_object()	exlock_t __autolock__ = this
#define	protected_object()	shlock_t __autolock__ = this
#define	exclusive_access(x)	exlock_t __autolock__ = &x
#define	protected_access(x) shlock_t __autolock__ = &x

END_NAMESPACE

#endif
