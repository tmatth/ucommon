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

struct cpr_mq;

typedef	void (*sighandler_t)(int);

#define	CPR_PRIORITY_LOWEST 0
#define	CPR_PRIORITY_LOW 1
#define	CPR_PRIORITY_NORMAL 2
#define	CPR_PRIORITY_HIGH 3

#define	CPR_SPAWN_WAIT		0
#define	CPR_SPAWN_NOWAIT	1
#define	CPR_SPAWN_DETACH	2

NAMESPACE_UCOMMON

class __EXPORT MessageQueue
{
private:
	struct ipc;

	ipc *mq;

public:
	MessageQueue(const char *name, size_t objsize, unsigned count);
	MessageQueue(const char *name, bool blocking = true);
	~MessageQueue();

	bool get(void *data, size_t len);
	bool put(void *data, size_t len);
	bool puts(char *data);
	bool gets(char *data);

	void release(void);

	inline operator bool() const
		{return mq != NULL;};
		
	inline bool operator!() const
		{return mq == NULL;};

	inline bool isPending(void) const
		{return getPending() > 0;};

	unsigned getPending(void) const;
	unsigned getLimit(void) const;
	size_t getSize(void) const;
};

class __EXPORT envpager : public mempager
{
protected:
	LinkedObject *root;

public:
	typedef named_value<const char *> member;

	envpager(size_t paging);
	~envpager();

	void dup(const char *id, const char *val);
	void set(char *id, const char *val);
	const char *get(const char *id);

	inline member *find(const char *id)
		{return static_cast<member*>(NamedObject::find(static_cast<NamedObject*>(root), id));};

	inline member *begin(void)
		{return static_cast<member*>(root);};

#ifdef	_MSWINDOWS_
	char **getEnviron(void);
#endif

};

template <class T>
class mqueue : private MessageQueue
{
protected:
	unsigned getPending(void)
		{return MessageQueue::getPending();};

public:
	inline mqueue(const char *name) :
		MessageQueue(name) {};
	
	inline mqueue(const char *name, unsigned count) :
		MessageQueue(name, sizeof(T), count) {};

	inline ~mqueue() {release();};

	inline operator bool() const
		{return mq != NULL;};

	inline bool operator!() const
		{return mq == NULL;};

	inline bool get(T *buf)
		{return MessageQueue::get(buf);};
	
	inline bool put(T *buf)
		{return MessageQueue::put(buf);};
};

END_NAMESPACE

extern "C" {

	__EXPORT size_t cpr_pagesize(void);
	__EXPORT int cpr_scheduler(int policy, unsigned priority = CPR_PRIORITY_NORMAL);
	__EXPORT void cpr_pattach(const char *path);
	__EXPORT void cpr_pdetach(void);
	__EXPORT int cpr_spawn(const char *fn, char **args, int mode, pid_t *pid, fd_t *iov = NULL, ucc::envpager *env = NULL);
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
	__EXPORT void cpr_sleep(timeout_t timeout);
	__EXPORT void cpr_yield(void);
	__EXPORT int cpr_priority(unsigned priority);
	__EXPORT void cpr_memlock(void *addr, size_t len);
	__EXPORT void cpr_memunlock(void *addr, size_t len);
};

#endif

