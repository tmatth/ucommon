#ifndef _UCOMMON_PROCESS_H_
#define	_UCOMMON_PROCESS_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef	_UCOMMON_FILE_H_
#include <ucommon/file.h>
#endif

#ifndef _UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

typedef	void (*sighandler_t)(int);

#define	CPR_PRIORITY_LOWEST 0
#define	CPR_PRIORITY_LOW 1
#define	CPR_PRIORITY_NORMAL 2
#define	CPR_PRIORITY_HIGH 3

NAMESPACE_UCOMMON

class __EXPORT keypair : public SharedObject
{
public:
	class __EXPORT callback : public LinkedObject
	{
	friend class keypair;
	protected:
		callback();
		virtual ~callback();
		
		void release(void);

		virtual void notify(SharedPointer *ptr, keypair *keys) = 0;
	};

	class __EXPORT keydata : public NamedObject
	{
	friend class keypair;

	public:
		keydata(keydata **root, char *kid, const char *value = NULL);

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
	friend class callback;

	static linked_pointer<callback> callbacks;

	keydata *keypairs;
	mempager *pager;
	unsigned count;
	keydata **index;

	void update(keydata *key, const char *value);

	keydata *create(char *key, const char *data = NULL);
	const char *alloc(const char *data);
	void dealloc(const char *data);
	void dealloc(void);
	void commit(SharedPointer *ptr);

public:
	typedef struct {
		char *key;
		const char *data;
	} define;

	typedef shared_pointer<keypair> pointer;
	typedef shared_instance<keypair> instance;

	keypair(define *defaults = NULL, mempager *mem = NULL);
	~keypair();

	void set(char *id, const char *value);
	const char *get(const char *id);
	void load(FILE *fp, const char *key = NULL);
	void load(define *defaults);
	void section(FILE *fp, char *buf, size_t size);
	void validate(unsigned count, const char *lastkey);

	inline keydata *begin(void)
		{return keypairs;};

	inline const char *operator()(const char *id)
		{return get(id);};

	inline unsigned getCount(void)
		{return count;};

	const char *operator[](unsigned idx);

	inline const char *operator()(unsigned idx)
		{return operator[](idx);};
};

END_NAMESPACE

#define	SPAWN_WAIT		0
#define	SPAWN_NOWAIT	1
#define	SPAWN_DETACH	2

extern "C" {

	__EXPORT size_t cpr_pagesize(void);
	__EXPORT int cpr_scheduler(int policy, unsigned priority = CPR_PRIORITY_NORMAL);
	__EXPORT void cpr_pattach(const char *path);
	__EXPORT void cpr_pdetach(void);
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
	__EXPORT void cpr_sleep(timeout_t timeout);
	__EXPORT void cpr_yield(void);
	__EXPORT int cpr_priority(unsigned priority);
};

#endif

