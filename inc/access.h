#ifndef _UCOMMON_ACCESS_H_
#define	_UCOMMON_ACCESS_H_

#ifndef _UCOMMON_CONFIG_H_
#include ucommon/config.h
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

class __EXPORT auto_exclusive
{
private:
	Exclusive *lock;

public:
	auto_exclusive(Exclusive *l);
	~auto_exclusive();
	
	void release(void);
};

class __EXPORT auto_shared
{
private:
    Shared *lock;

public:
    auto_shared(Shared *l);
    ~auto_shared();

    void release(void);
};

#define	__access__name(x)	x##__LINE__
	
#define	access_exclusive(x)	auto_exclusive __access__name(_ex_) \
	(static_cast<Exclusive*>(&x))

#define	access_shared(x)	auto_shared __access__name(_ex_) \
	(static_cast<Shared*>(&x))

END_NAMESPACE

#endif
