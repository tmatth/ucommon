#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_FILE_H_
#include <ucommon/file.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT keypair
{
protected:
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

	void update(keydata *key, const char *value);

	virtual keydata *create(const char *key, const char *data = NULL);
	virtual const char *alloc(const char *data);
	virtual void dealloc(const char *data);

public:
	typedef struct {
		const char *key;
		const char *data;
	} define;
	void set(const char *id, const char *value);
	const char *get(const char *id);
	void load(const char *path);
	void load(define *defaults);
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

