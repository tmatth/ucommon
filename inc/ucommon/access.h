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
	
	void release(void);
};

class __EXPORT shared_lock
{
private:
    Shared *lock;
	int state;

public:
    shared_lock(Shared *l);
    ~shared_lock();

    void release(void);
};

inline void lock(Exclusive *ex)
	{ex->Exlock();};

inline void unlock(Exclusive *ex)
	{ex->Unlock();};

inline void share(Shared *sh)
	{sh->Shlock();};

inline void release(Shared *sh)
	{sh->Unlock();};

END_NAMESPACE

#endif
