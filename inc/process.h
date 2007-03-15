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

typedef	RETSIGTYPE (*sighandler_t)(int);

NAMESPACE_UCOMMON

class __EXPORT keypair
{
private:
	friend class __EXPORT keyconfig;

public:
	class __EXPORT keydata : public NamedObject
	{
	friend class keypair;

	public:
		keydata(keydata **root, const char *kid, const char *value = NULL);

	private:
#pragma pack(1)
		const char *data;
		char key[1];
#pragma pack()

	public:
		inline const char *getValue(void)
			{return data;};
	};

private:
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

	keypair(define *defaults = NULL, mempager *mem = NULL);

	void set(const char *id, const char *value);
	const char *get(const char *id);
	void load(FILE *fp, const char *key = NULL);
	void load(define *defaults);
	void section(FILE *fp, char *buf, size_t size);

	inline keydata *begin(void)
		{return keypairs;};

	inline const char *operator()(const char *id)
		{return get(id);};
};

class __EXPORT keyconfig : public CountedObject, protected mempager
{
public:
	typedef	locked_pointer<keyconfig> pointer;

	class __EXPORT instance : public locked_release
	{
	public:
		instance(pointer &p);

		inline keyconfig &operator*()
			{return *(static_cast<keyconfig *>(object));};

		inline keyconfig *operator->()
			{return static_cast<keyconfig *>(object);};

		const char *operator()(unsigned idx, const char *key);
		keypair *operator[](unsigned idx);
	};

private:
	keypair *keypairs;
	size_t size;

	void dealloc(void);

public:
	keyconfig(unsigned members = 1, size_t pagesize = 0);

	inline void release(void)
		{CountedObject::release();};

	keypair *operator[](unsigned idx);
};		

__EXPORT void suspend(void);
__EXPORT void suspend(timeout_t timeout);

END_NAMESPACE

#define	SPAWN_WAIT		0
#define	SPAWN_NOWAIT	1
#define	SPAWN_DETACH	2

extern "C" {

	__EXPORT size_t cpr_pagesize(void);
	__EXPORT void cpr_attach(const char *path);
	__EXPORT void cpr_detach(void);
	__EXPORT int cpr_spawn(const char *fn, char **args, int mode);
	__EXPORT void cpr_closeall(void);
	__EXPORT void cpr_cancel(pid_t pid);
#ifndef	_MSWINDOWS_
	__EXPORT sighandler_t cpr_intsignal(int sig, sighandler_t handler);
	__EXPORT sighandler_t cpr_signal(int sig, sighandler_t handler);
	__EXPORT void cpr_hangup(pid_t pid);
	__EXPORT int cpr_sigwait(sigset_t *set);
#else
	#define cpr_signal(sig, handler) signal(sig, handler)
#endif
	__EXPORT pid_t cpr_wait(pid_t pid = 0, int *status = NULL);
	__EXPORT pid_t cpr_create(const char *path, char **args, fd_t *iov);

	inline void cpr_sleep(timeout_t timeout)
		{ucc::suspend(timeout);};

	inline void cpr_yield(void)
		{ucc::suspend();};
};

#endif

