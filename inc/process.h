#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef	_UCOMMON_FILE_H_
#include <ucommon/file.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT keypair
{
private:
	friend class __EXPORT keyconfig;

	class __EXPORT keydata : public NamedObject
	{
	public:
		keydata(keydata **root, const char *kid, const char *value = NULL);

#pragma pack(1)
		const char *data;
		char key[1];
#pragma pack()
	};

	keydata *keypairs;
	mempager *pager;

	void update(keydata *key, const char *value);

	keydata *create(const char *key, const char *data = NULL);
	const char *alloc(const char *data);
	void dealloc(const char *data);

public:
	typedef struct {
		const char *key;
		const char *data;
	} define;

	keypair(define *defaults = NULL, const char *path = NULL, mempager *mem = NULL);

	void set(const char *id, const char *value);
	const char *get(const char *id);
	void load(const char *path);
	void load(define *defaults);

	inline const char *operator()(const char *id)
		{return get(id);};
};

class __EXPORT keyconfig : protected mempager, public CountedObject
{
public:
	class __EXPORT pointer 
	{
	public:
		pointer();

	private:
		friend class keyconfig;
		keyconfig *config;
		static_mutex_t mutex;
	};

	class __EXPORT instance 
	{
	private:
		keyconfig *object;

	public:
		instance(pointer &p);
		~instance();

		inline keyconfig *operator*()
			{return object;};

		keypair *operator[](unsigned idx);
	};

private:
	keypair *keypairs;
	size_t size;

	void dealloc(void);

public:
	keyconfig(unsigned members, size_t pagesize);

	inline static keyconfig *getInstance(pointer &i)
		{return static_cast<keyconfig *>(Object::get(i.config, &i.mutex));};

	inline void commit(pointer &i)
		{Object::set(i.config, this, &i.mutex);};

	inline void release(void)
		{CountedObject::release();};

	keypair *operator[](unsigned idx);
};		

__EXPORT void suspend(void);
__EXPORT void suspend(timeout_t timeout);

END_NAMESPACE

extern "C" {

	__EXPORT void cpr_closeall(void);
	__EXPORT void cpr_cancel(pid_t pid);
	__EXPORT void cpr_hangup(pid_t pid);
	__EXPORT pid_t cpr_wait(pid_t pid = 0, int *status = NULL);
	__EXPORT pid_t cpr_create(const char *path, char **args, fd_t *iov);

	inline void cpr_sleep(timeout_t timeout)
		{ucc::suspend(timeout);};

	inline void cpr_yield(void)
		{ucc::suspend();};
};

#endif

