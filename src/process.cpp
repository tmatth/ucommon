#include <config.h>
#include <ucommon/process.h>
#include <ucommon/string.h>
#include <ucommon/misc.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_MQUEUE_H
#include <mqueue.h>
#endif

#if defined(HAVE_SIGACTION) && defined(HAVE_BSD_SIGNAL_H)
#undef	HAVE_BSD_SIGNAL_H
#endif

#if defined(HAVE_BSD_SIGNAL_H)
#include <bsd/signal.h>
#define	HAVE_SIGNALS
#elif defined(HAVE_SIGNAL_H)
#include <signal.h>
#define	HAVE_SIGNALS
#endif

#ifdef	HAVE_SIGNALS
#ifndef	SA_ONESHOT
#define	SA_ONESHOT	SA_RESETHAND
#endif
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>

#ifdef  SIGTSTP
#include <sys/file.h>
#include <sys/ioctl.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) ((unsigned)(status) >> 8)
#endif

#ifndef	_PATH_TTY
#define	_PATH_TTY	"/dev/tty"
#endif

using namespace UCOMMON_NAMESPACE;

#ifdef	HAVE_MQUEUE_H
struct MessageQueue::ipc
{
	mqd_t mqid;
	mq_attr attr;
};

MessageQueue::MessageQueue(const char *name, size_t msgsize, unsigned count)
{
	mq = (ipc *)malloc(sizeof(ipc));
	memset(&mq->attr, 0 , sizeof(mq_attr));
	mq->attr.mq_maxmsg = count;
	mq->attr.mq_msgsize = msgsize;
	mq_unlink(name);
	mq->mqid = mq_open(name, O_CREAT | O_RDWR | O_NONBLOCK, 0660, &mq->attr);
	if(mq->mqid == (mqd_t)(-1)) {
		free(mq);
		mq = NULL;
	}
}
	
MessageQueue::MessageQueue(const char *name, bool blocking)
{
	mq = (ipc *)malloc(sizeof(ipc));
	int mode = O_WRONLY;

	if(!blocking)
		mode |= O_NONBLOCK;
	mq->mqid = mq_open(name, mode);
	if(mq->mqid == (mqd_t)-1) {
		free(mq);
		return;
	}
	mq_getattr(mq->mqid, &mq->attr);
}

MessageQueue::~MessageQueue()
{
	release();
}

void MessageQueue::release(void)
{
	if(mq) {
		mq_close(mq->mqid);
		free(mq);
		mq = NULL;
	}
}

unsigned MessageQueue::getLimit(void) const
{
	if(!mq)
		return 0;

	return mq->attr.mq_maxmsg;
}

size_t MessageQueue::getSize(void) const
{
	if(!mq)
		return 0;
	return mq->attr.mq_msgsize;
}

unsigned MessageQueue::getPending(void) const
{
	mq_attr attr;
	if(!mq)
		return 0;

	if(mq_getattr(mq->mqid, &attr))
		return 0;

	return attr.mq_curmsgs;
}

bool MessageQueue::puts(char *buf)
{
	size_t len = cpr_strlen(buf);
	if(!mq)
		return false;

	if(len >= getSize())
		return false;
	
	return put(buf, len);
}

bool MessageQueue::put(void *buf, size_t len)
{
	if(!mq)
		return false;

	if(!len)
		len = getSize();

	if(!len)
		return false;

	if(mq_send(mq->mqid, (const char *)buf, len, 0) < 0)
		return false;

	return true;
}

bool MessageQueue::gets(char *buf)
{
	unsigned int pri;
	ssize_t len = getSize();
	if(!len)
		return false;

	len = mq_receive(mq->mqid, buf, (size_t)len, &pri);
	if(len < 1)
		return false;
	
	buf[len] = 0;
	return true;
}	

bool MessageQueue::get(void *buf, size_t len)
{
	unsigned int pri;

	if(!mq)
		return false;
	
	if(!len)
		len = getSize();

	if(!len)
		return false;

	if(mq_receive(mq->mqid, (char *)buf, len, &pri) < 0)
		return false;
	
	return true;
}

#endif

envpager::envpager(size_t ps) :
mempager(ps)
{
	root = NULL;
}

envpager::~envpager()
{
	purge();
}

const char *envpager::get(const char *id)
{
	member *key = find(id);
	if(!key)
		return NULL;

	return key->value;
}

#ifdef	_MSWINDOWS_
char **envpager::getEnviron(void)
{
	char buf[1024 - 64];
	linked_pointer<member> env;
	unsigned idx = 0;
	unsigned count = LinkedObject::count(root) + 1;
	caddr_t mp = (caddr_t)alloc(count * sizeof(char *));
	char **envp = new(mp) char *[count];

	env = root;
	while(env) {
		snprintf(buf, sizeof(buf), "%s=%s", env->getId(), env->value);
		envp[idx++] = mempager::dup(buf);
	}
	envp[idx] = NULL;
	return envp;
}
#endif

void envpager::dup(const char *id, const char *value)
{
	member *env = find(id);

	if(!env) {
		caddr_t mp = (caddr_t)alloc(sizeof(member));
		env = new(mp) member(&root, mempager::dup(id));
	}
	env->value = mempager::dup(value);
};

void envpager::set(char *id, const char *value)
{
    member *env = find(id);
	
	if(!env) {
	    caddr_t mp = (caddr_t)alloc(sizeof(member));
	    env = new(mp) member(&root, id);
	}

    env->value = value;
};

extern "C" void cpr_sleep(timeout_t timeout)
{
#ifdef	_MSWINDOWS_
	SleepEx(timeout, FALSE);
#else
	timespec ts;
	ts.tv_sec = timeout / 1000l;
	ts.tv_nsec = (timeout % 1000l) * 1000000l;
#ifdef	HAVE_PTHREAD_DELAY
	pthread_delay(&ts);
#else
	nanosleep(&ts, NULL);
#endif
#endif
}

extern "C" void cpr_yield(void)
{
	sched_yield();
}

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

#ifndef	_MSWINDOWS_
extern "C" void cpr_pdetach(void)
{
	if(getppid() == 1)
		return;
	cpr_pattach("/dev/null");
}

extern "C" void cpr_pattach(const char *dev)
{
	pid_t pid;
	int fd;

	close(0);
	close(1);
	close(2);
#ifdef	SIGTTOU
	cpr_signal(SIGTTOU, SIG_IGN);
#endif

#ifdef	SIGTTIN
	cpr_signal(SIGTTIN, SIG_IGN);
#endif

#ifdef	SIGTSTP
	cpr_signal(SIGTSTP, SIG_IGN);
#endif
	pid = fork();
	if(pid > 0)
		exit(0);
	crit(pid == 0);

#if defined(SIGTSTP) && defined(TIOCNOTTY)
	crit(setpgid(0, getpid()) == 0);
	if((fd = open(_PATH_TTY, O_RDWR)) >= 0) {
		ioctl(fd, TIOCNOTTY, NULL);
		close(fd);
	}
#else

#ifdef HAVE_SETPGRP
	crit(setpgrp() == 0);
#else
	crit(setpgid(0, getpid()) == 0);
#endif
	cpr_signal(SIGHUP, SIG_IGN);
	pid = fork();
	if(pid > 0)
		exit(0);
	crit(pid == 0);
#endif
	if(dev && *dev) {
		fd = open(dev, O_RDWR);
		if(fd > 0)
			dup2(fd, 0);
		if(fd != 1)
			dup2(fd, 1);
		if(fd != 2)
			dup2(fd, 2);
		if(fd > 2)
			close(fd);
	}
}

static void setenv(envpager *ep)
{
	linked_pointer<envpager::member> env;

	if(!ep)
		return;

	env = ep->begin();
	while(env) {
		setenv(env->getId(), env->value, 1);
		env.next();
	}
}

extern "C" int cpr_spawn(const char *fn, char **args, int mode, pid_t *pid, fd_t *iov, envpager *env)
{
	unsigned max = OPEN_MAX, idx = 0;
	int status;

	*pid = fork();
	if(*pid < 0)
		return -1;

	if(*pid) {
		switch(mode) {
		case CPR_SPAWN_DETACH:
		case CPR_SPAWN_NOWAIT:
			return 0;
		case CPR_SPAWN_WAIT:
			waitpid(*pid, &status, 0);
			return WEXITSTATUS(status);
		}
	}

	while(iov && *iov > -1)
		dup2(*(iov++), idx++);

	while(idx < 3)
		++idx;
	
#if defined(HAVE_SYSCONF)
	max = sysconf(_SC_OPEN_MAX);
#endif
#if defined(HAVE_SYS_RESOURCE_H)
	struct rlimit rl;
	if(!getrlimit(RLIMIT_NOFILE, &rl))
		max = rl.rlim_cur;
#endif

	while(idx < max)
		close(idx++);

	if(mode == CPR_SPAWN_DETACH)
		cpr_pdetach();

	setenv(env);		
	execvp(fn, args);
	exit(-1);
}

extern "C" void cpr_closeall(void)
{
	unsigned max = OPEN_MAX;
#if defined(HAVE_SYSCONF)
	max = sysconf(_SC_OPEN_MAX);
#endif
#if defined(HAVE_SYS_RESOURCE_H)
	struct rlimit rl;
	if(!getrlimit(RLIMIT_NOFILE, &rl))
		max = rl.rlim_cur;
#endif
	for(unsigned fd = 3; fd < max; ++fd)
		::close(fd);
}

extern "C" void cpr_cancel(pid_t pid)
{
#ifdef	HAVE_SIGNAL_H
	kill(pid, SIGTERM);
#endif
}

extern "C" void cpr_hangup(pid_t pid)
{
#ifdef  HAVE_SIGNAL_H
    kill(pid, SIGHUP);
#endif
}

extern "C" sighandler_t cpr_intsignal(int signo, sighandler_t func)
{
	struct	sigaction	sig_act, old_act;

	memset(&sig_act, 0, sizeof(sig_act));
	sig_act.sa_handler = func;
	sigemptyset(&sig_act.sa_mask);
	if(signo != SIGALRM)
		sigaddset(&sig_act.sa_mask, SIGALRM);

	sig_act.sa_flags = 0;
#ifdef	SA_INTERRUPT
	sig_act.sa_flags |= SA_INTERRUPT;
#endif
	if(sigaction(signo, &sig_act, &old_act) < 0)
		return SIG_ERR;

	return old_act.sa_handler;
}

extern "C" sighandler_t cpr_signal(int signo, sighandler_t func)
{
	struct sigaction sig_act, old_act;

	memset(&sig_act, 0, sizeof(sig_act));
	sig_act.sa_handler = func;
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = 0;
	if(signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		sig_act.sa_flags |= SA_INTERRUPT;
#endif
	}
	else {
		sigaddset(&sig_act.sa_mask, SIGALRM);
#ifdef	SA_RESTART
		sig_act.sa_flags |= SA_RESTART;
#endif
	}
	if(sigaction(signo, &sig_act, &old_act))
		return SIG_ERR;
	return old_act.sa_handler;
}

extern "C" int cpr_sigwait(sigset_t *set)
{
#ifdef	HAVE_SIGWAIT2
	int status;
	crit(sigwait(set, &status) == 0);
	return status;
#else
	return sigwait(set);
#endif
}

extern "C" pid_t cpr_wait(pid_t pid, int *status)
{
	if(pid) {
		if(waitpid(pid, status, 0))
			return -1;
		return pid;
	}
	else
		pid = wait(status);

	if(status)
		*status = WEXITSTATUS(*status);
	return pid;
}

#else
extern "C" void cpr_closeall(void)
{
}
#endif

extern "C" size_t cpr_pagesize(void)
{
#ifdef  HAVE_SYSCONF
    return sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
    return PAGESIZE;
#elif defined(PAGE_SIZE)
    return PAGE_SIZE;
#else
    return 1024;
#endif
}

extern "C" int cpr_scheduler(int policy, unsigned priority)
{
#if _POSIX_PRIORITY_SCHEDULING > 0
	struct sched_param sparam;
    int min = sched_get_priority_min(policy);
    int max = sched_get_priority_max(policy);
	int pri = (int)priority;

	if(min == max)
		pri = min;
	else 
		pri += min;
	if(pri > max)
		pri = max;

	if(policy == SCHED_OTHER) 
		setpriority(PRIO_PROCESS, 0, -(priority - CPR_PRIORITY_NORMAL));
	else
		setpriority(PRIO_PROCESS, 0, -(priority - CPR_PRIORITY_LOW));

	memset(&sparam, 0, sizeof(sparam));
	sparam.sched_priority = pri;
	return sched_setscheduler(0, policy, &sparam);	
#elif defined(_MSWINDOWS_)
	return cpr_priority(priority);
#endif
}

extern "C" int cpr_priority(unsigned priority)
{
#if defined(_POSIX_PRIORITY_SCHEDULING)
	struct sched_param sparam;
	pthread_t tid = pthread_self();
    int min, max, policy;
	int pri = (int)priority;

	if(pthread_getschedparam(tid, &policy, &sparam))
		return -1;

	min = sched_get_priority_min(policy);
    max = sched_get_priority_max(policy);

	if(min == max)
		return 0;

	pri += min;
	if(pri > max)
		pri = max;
	sparam.sched_priority = pri;
	return pthread_setschedparam(tid, policy, &sparam);
#elif defined(_MSWINDOWS_)
	int pri = THREAD_PRIORITY_ABOVE_NORMAL;
	switch(priority) {
	case CPR_PRIORITY_LOWEST:
	case CPR_PRIORITY_LOW:
		pri = THREAD_PRIORITY_LOWEST;
		break;
	case CPR_PRIORITY_NORMAL:
		pri = THREAD_PRIORITY_NORMAL;
		break;
	}
	SetThreadPriority(GetCurrentThread(), priority);
	return 0;
#endif
}

#if defined(HAVE_MKFIFO)

static void fifo_name(const char *name, char *buf, size_t max)
{
	if(*name == '/')
		++name;

	snprintf(buf, max, "/var/run/%s", name);
	if(cpr_isdir(buf)) {
		snprintf(buf, max, "/var/run/%s/%s.ctrl");
		return;
	}

	snprintf(buf, max, "/tmp/.%s.ctrl", name);
}

void cpr_unlinkctrl(const char *name)
{
	char buf[65];

	fifo_name(name, buf, sizeof(buf));
	remove(buf);
}

fd_t cpr_createctrl(const char *name)
{
	char buf;

	fifo_name(name, buf, sizeof(buf));
	remove(buf);
	if(mkfifo(buf, 0660))
		return -1;

	return open(name, O_RDWR);
}

fd_t cpr_openctrl(const char *name)
{
	char buf[65];

	fifo_name(name, buf, sizeof(buf));
	return open(name, O_WRONLY);
}

#elif defined(_MSWINDOWS_)

void cpr_unlinkctrl(const char *name)
{
}

fd_t cpr_createctrl(const char *name)
{
	return INVALID_HANDLE_VALUE;
}

fd_t cpr_openctrl(const char *name)
{
	return INVALID_HANDLE_VALUE;
}

#else

void cpr_unlinkctrl(const char *name)
{
}

fd_t cpr_createctrl(const char *name)
{
	return -1;
}

fd_t cpr_openctrl(const char *name)
{
	return -1;
}

#endif

#ifdef	_MSWINDOWS_

void cpr_memlock(void *addr, size_t len)
{
	VirtualLock(addr, len);
}

void cpr_memunlock(void *addr, size_t len)
{
	VirtualUnlock(addr, len);
}

#else

void cpr_memlock(void *addr, size_t len)
{
#if _POSIX_MEMLOCK_RANGE > 0
	mlock(addr, len);
#endif
}

void cpr_memunlock(void *addr, size_t len)
{
#if _POSIX_MEMLOCK_RANGE > 0
	munlock(addr, len);
#endif
}

#endif


