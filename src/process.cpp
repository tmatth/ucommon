#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/string.h>
#include <ucommon/process.h>
#include <errno.h>

#ifdef	HAVE_PWD_H
#include <pwd.h>
#include <grp.h>
#endif

#ifdef	HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_MQUEUE_H
#include <mqueue.h>
#endif

#if HAVE_FTOK
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

#if	defined(HAVE_FTOK) && (!defined(HAVE_MQUEUE_H) || !defined(HAVE_SHM_OPEN))

#include <sys/ipc.h>
#include <sys/shm.h>

static void ftok_name(const char *name, char *buf, size_t max)
{
	if(*name == '/')
		++name;

	if(cpr_isdir("/var/run/ipc"))
		snprintf(buf, sizeof(buf), "/var/run/ipc/%s", name);
	else
		snprintf(buf, sizeof(buf), "/tmp/.%s.ipc", name);
}

#if !defined(HAVE_MQUEUE_H)
static void unlinkipc(const char *name)
{
	char buf[65];

	ftok_name(name, buf, sizeof(buf));
	remove(buf);
}
#endif

static key_t createipc(const char *name, char mode)
{
	char buf[65];
	int fd;

	ftok_name(name, buf, sizeof(buf));
	fd = open(buf, O_CREAT | O_EXCL | O_WRONLY, 0660);
	if(fd > -1)
		close(fd);
	return ftok(buf, mode);
}

static key_t accessipc(const char *name, char mode)
{
	char buf[65];

	ftok_name(name, buf, sizeof(buf));
	return ftok(buf, mode);
}

#endif

using namespace UCOMMON_NAMESPACE;

#if defined(_MSWINDOWS_)

MappedMemory::MappedMemory(const char *fn, size_t len)
{
	int share = FILE_SHARE_READ;
	int prot = FILE_MAP_READ;
	int mode = GENERIC_READ;
	struct stat ino;

	size = 0;
	used = 0;
	map = NULL;

	if(len) {
		prot = FILE_MAP_WRITE;
		mode |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, len, fn + 1);

	}
	else
		fd = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fn + 1);
	
	if(fd == INVALID_HANDLE_VALUE || fd == NULL) 
		return;

	map = (caddr_t)MapViewOfFile(fd, FILE_MAP_ALL_ACCESS, 0, 0, len);
	if(map) {
		size = len;
		VirtualLock(map, size);
	}
}

MappedMemory::~MappedMemory()
{
	if(map) {
		VirtualUnlock(map, size);
		UnmapViewOfFile(fd);
		CloseHandle(fd);
		map = NULL;
		fd = INVALID_HANDLE_VALUE;
	}
}

#elif defined(HAVE_SHM_OPEN)

MappedMemory::MappedMemory(const char *fn, size_t len)
{
	int prot = PROT_READ;
	struct stat ino;

	size = 0;
	used = 0;
	
	if(len) {
		prot |= PROT_WRITE;
		shm_unlink(fn);
		fd = shm_open(fn, O_RDWR | O_CREAT, 0660);
		if(fd > -1)
			ftruncate(fd, len);
	}
	else {
		fd = shm_open(fn, O_RDONLY, 0660);
		if(fd > -1) {
			fstat(fd, &ino);
			len = ino.st_size;
		}
	}

	if(fd < 0)
		return;

	map = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	close(fd);
	if(map != (caddr_t)MAP_FAILED) {
		size = len;
		mlock(map, size);
	}
}

MappedMemory::~MappedMemory()
{
	if(size) {
		munlock(map, size);
		munmap(map, size);
		size = 0;
	}
}

#else

MappedMemory::MappedMemory(const char *name, size_t len)
{
	struct shmid_ds stat;
	size = 0;
	used = 0;
	key_t key;

	if(len) {
		key = createipc(name, 'S');
remake:
		fd = shmget(key, len, IPC_CREAT | IPC_EXCL | 0660);
		if(fd == -1 && errno == EEXIST) {
			fd = shmget(key, 0, 0);
			if(fd > -1) {
				shmctl(fd, IPC_RMID, NULL);
				goto remake;
			}
		}
	}
	else {
		key = accessipc(name, 'S');
		fd = shmget(key, 0, 0);
	}
	
	if(fd > -1) {
		if(len)
			size = len;
		else if(shmctl(fd, IPC_STAT, &stat) == 0)
			size = stat.shm_segsz;
		else
			fd = -1;
	}
	map = (caddr_t)shmat(fd, NULL, 0);
#ifdef	SHM_LOCK
	if(fd > -1)
		shmctl(fd, SHM_LOCK, NULL);
#endif
}

MappedMemory::~MappedMemory()
{
	if(size > 0) {
#ifdef	SHM_UNLOCK
		shmctl(fd, SHM_UNLOCK, NULL);
#endif
		shmdt(map);
		fd = -1;
		size = 0;
	}
}

#endif

void MappedMemory::fault(void) 
{
	abort();
}

void *MappedMemory::sbrk(size_t len)
{
	void *mp = (void *)(map + used);
	if(used + len > size)
		fault();
	used += len;
	return mp;
}
	
void *MappedMemory::get(size_t offset)
{
	if(offset >= size)
		fault();
	return (void *)(map + offset);
}

MappedReuse::MappedReuse(const char *name, size_t osize, unsigned count) :
ReusableAllocator(), MappedMemory(name,  osize * count)
{
	objsize = osize;
}

bool MappedReuse::avail(void)
{
	bool rtn = false;
	lock();
	if(freelist || used < size)
		rtn = true;
	unlock();
	return rtn;
}

ReusableObject *MappedReuse::request(void)
{
    ReusableObject *obj = NULL;

	lock();
	if(freelist) {
		obj = freelist;
		freelist = next(obj);
	} 
	else if(used + objsize <= size)
		obj = (ReusableObject *)sbrk(objsize);
	unlock();
	return obj;	
}

ReusableObject *MappedReuse::get(void)
{
	return get(Timer::inf);
}

ReusableObject *MappedReuse::get(timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
	ReusableObject *obj = NULL;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && !freelist && used >= size) {
		++waiting;
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
		--waiting;
	}
	if(!rtn) {
		unlock();
		return NULL;
	}
	if(freelist) {
		obj = freelist;
		freelist = next(obj);
	}
	else if(used + objsize <= size)
		obj = (ReusableObject *)sbrk(objsize);
	unlock();
	return obj;
}

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
	mq->mqid = (mqd_t)(-1);
	if(strrchr(name, '/') == name)
		mq->mqid = mq_open(name, O_CREAT | O_RDWR | O_NONBLOCK, 0660, &mq->attr);
	if(mq->mqid == (mqd_t)(-1)) {
		free(mq);
		mq = NULL;
	}
}
	
MessageQueue::MessageQueue(const char *name)
{
	mq = (ipc *)malloc(sizeof(ipc));

	mq->mqid = (mqd_t)(-1);
	if(strrchr(name, '/') == name)
		mq->mqid = mq_open(name, O_WRONLY | O_NONBLOCK);
	if(mq->mqid == (mqd_t)-1) {
		free(mq);
		mq = NULL;
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
	size_t len = string::count(buf);
	if(!mq)
		return false;

	if(len >= (size_t)(mq->attr.mq_msgsize))
		return false;
	
	return put(buf, len);
}

bool MessageQueue::put(void *buf, size_t len)
{
	if(!mq)
		return false;

	if(!len)
		len = mq->attr.mq_msgsize;

	if(!len)
		return false;

	if(mq_send(mq->mqid, (const char *)buf, len, 0) < 0)
		return false;

	return true;
}

bool MessageQueue::gets(char *buf)
{
	unsigned int pri;
	ssize_t len = mq->attr.mq_msgsize;
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
		len = mq->attr.mq_msgsize;

	if(!len)
		return false;

	if(mq_receive(mq->mqid, (char *)buf, len, &pri) < 0)
		return false;
	
	return true;
}

#elif defined(HAVE_FTOK)
#include <sys/msg.h>

/* struct msgbuf {
	long mtype;
	char mtext[1];
}; */

struct MessageQueue::ipc
{
	key_t key;
	bool creator;
	int fd;
	struct msqid_ds attr;
	struct msginfo info;
	struct msgbuf *mbuf;
};

MessageQueue::MessageQueue(const char *name)
{
	mq = (ipc *)malloc(sizeof(ipc));
	mq->key = accessipc(name, 'M');
	mq->creator = false;

	mq->fd = -1;
	if(strrchr(name, '/') == name) 
		mq->fd = msgget(mq->key, 0660);

	if(mq->fd == -1) {
fail:
		free(mq);
		mq = NULL;
		return;
	}

	if(msgctl(mq->fd, IPC_STAT, &mq->attr))
		goto fail;
	void *mi = (void *)&mq->info;
	if(msgctl(mq->fd, IPC_INFO, (struct msqid_ds *)(mi)))
		goto fail;
	mq->mbuf = (struct msgbuf *)malloc(sizeof(msgbuf) + mq->info.msgmax);
}

MessageQueue::MessageQueue(const char *name, size_t size, unsigned count)
{
	int tmp;

	mq = (ipc *)malloc(sizeof(ipc));
	mq->key = createipc(name, 'M');
	mq->creator = true;
	
	if(strrchr(name, '/') != name)
		goto fail;

remake:
	mq->fd = msgget(mq->key, IPC_CREAT | IPC_EXCL | 0660);

	if(mq->fd < 0 && errno == EEXIST) {
		tmp = msgget(mq->key, 0660);
		if(tmp > -1) {
			msgctl(tmp, IPC_RMID, NULL);
			goto remake;
		}
	}
	
    if(mq->fd == -1) {
fail:
        free(mq);
        mq = NULL;
		unlinkipc(name);
        return;
    }

	void *mi = (void *)&mq->info;
	if(msgctl(mq->fd, IPC_INFO, (struct msqid_ds *)mi)) {
		msgctl(mq->fd, IPC_RMID, NULL);
		goto fail;
	}

	if(msgctl(mq->fd, IPC_STAT, &mq->attr)) {
		msgctl(mq->fd, IPC_RMID, NULL);
		goto fail;
	}

	mq->attr.msg_qbytes = size * count;
	if(msgctl(mq->fd, IPC_SET, &mq->attr)) {
		msgctl(mq->fd, IPC_RMID, NULL);
		goto fail;
	}
	mq->mbuf = (struct msgbuf *)malloc(sizeof(struct msgbuf) + mq->info.msgmax);
}

MessageQueue::~MessageQueue()
{
	release();
}

void MessageQueue::release(void)
{
	if(mq) {
		free(mq->mbuf);
		if(mq->creator)
			msgctl(mq->fd, IPC_RMID, NULL);
		close(mq->fd);
		free(mq);
		mq = NULL;
	}
}

unsigned MessageQueue::getPending(void) const
{
	struct msqid_ds stat;

	if(!mq)
		return 0;

	if(msgctl(mq->fd, IPC_STAT, &stat))
		return 0;

	return stat.msg_qnum;
}

bool MessageQueue::puts(char *data)
{
	size_t len = string::count(data);

	if(len >= (size_t)(mq->info.msgmax))
		len = mq->info.msgmax - 1;

	return put(data, len);
}

bool MessageQueue::put(void *data, size_t len)
{
	if(!mq)
		return false;

	mq->mbuf->mtype = 1;

	if(len > (size_t)(mq->info.msgmax) || !len)
		len = mq->info.msgmax;

	memcpy(mq->mbuf->mtext, data, len);
	if(msgsnd(mq->fd, &mq->mbuf, len, IPC_NOWAIT) < 0)
		return false;
	return true;
}

bool MessageQueue::get(void *data, size_t len)
{
	if(!mq)
		return false;

	if(msgrcv(mq->fd, &mq->mbuf, len, 1, 0) < 0)
		return false;

	memcpy(data, mq->mbuf->mtext, len);
	return true;
}

bool MessageQueue::gets(char *data)
{
    if(!mq)
        return false;

    if(msgrcv(mq->fd, &mq->mbuf, mq->info.msgmax, 1, 0) < 0)
        return false;

	mq->mbuf->mtext[mq->info.msgmax - 1] = 0;
	strcpy(data, mq->mbuf->mtext);
    return true;
}


#endif

proc::proc(size_t ps) :
mempager(ps)
{
	root = NULL;
}

proc::~proc()
{
	purge();
}

void proc::setenv(proc *ep)
{
#ifndef	HAVE_SETENV
	char buf[128];
#endif

	linked_pointer<proc::member> env;

	if(!ep)
		return;

	env = ep->begin();
	while(env) {
#ifdef	HAVE_SETENV
		::setenv(env->getId(), env->value, 1);
#else
		snprintf(buf, sizeof(buf), "%s=%s", env->getID(), env->value);
		putenv(buf);
#endif
		env.next();
	}
}


const char *proc::get(const char *id)
{
	member *key = find(id);
	if(!key)
		return NULL;

	return key->value;
}

#ifdef	_MSWINDOWS_
char **proc::getEnviron(void)
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

void proc::dup(const char *id, const char *value)
{
	member *env = find(id);

	if(!env) {
		caddr_t mp = (caddr_t)alloc(sizeof(member));
		env = new(mp) member(&root, mempager::dup(id));
	}
	env->value = mempager::dup(value);
};

void proc::set(char *id, const char *value)
{
    member *env = find(id);
	
	if(!env) {
	    caddr_t mp = (caddr_t)alloc(sizeof(member));
	    env = new(mp) member(&root, id);
	}

    env->value = value;
};

#ifdef	_MSWINDOWS_

#else

bool proc::setuser(const char *uid)
{
	bool rtn = false;
	struct passwd *pwd;
	struct group *grp;

	if(!uid)
		return true;

	if(uid && *uid == '@') {
		grp = getgrnam(++uid);
		if(grp) {
			rtn = true;
			umask(002);
			setgid(grp->gr_gid);
		}
		endgrent();
	}
	else if(uid) {
		pwd = getpwnam(uid);
		if(pwd) {
			rtn = true;
			umask(022);
			::setenv("HOME", pwd->pw_dir, 1);
			::setenv("USER", pwd->pw_name, 1);
			::setenv("LOGNAME", pwd->pw_name, 1);
			::setenv("SHELL", pwd->pw_shell, 1);
			setgid(pwd->pw_gid);
			setuid(pwd->pw_uid);
		}
		endpwent();
	}
	return rtn;
}

bool proc::foreground(const char *id, const char *uid, int fac)
{
	bool rtn;

	if(!fac)
		fac = LOG_DAEMON;

	rtn = setuser(uid);
	setenv(this);
	
	if(id)
		openlog(id, LOG_PERROR, fac);

	purge();
	return rtn;
}

bool proc::background(const char *id, const char *uid, int fac)
{
	bool rtn;

	if(!fac)
		fac = LOG_DAEMON;

	cpr_pdetach();

	if(getppid() != 1)
		return false;

	rtn = setuser(uid);
	setenv(this);

	if(id)
		openlog(id, LOG_CONS, fac);

	purge();
	return rtn;
}

int proc::spawn(const char *fn, char **args, int mode, pid_t *pid, fd_t *iov, proc *env, const char *uid)
{
	unsigned max = OPEN_MAX, idx = 0;
	int status;

	*pid = fork();
	if(*pid < 0)
		return -1;

	if(*pid) {
		switch(mode) {
		case SPAWN_DETACH:
		case SPAWN_NOWAIT:
			return 0;
		case SPAWN_WAIT:
			cpr_waitpid(*pid, &status);
			return status;
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

	closelog();
	setuser(uid);

	while(idx < max)
		close(idx++);

	if(mode == SPAWN_DETACH)
		cpr_pdetach();

	setenv(env);		
	execvp(fn, args);
	exit(-1);
}
#endif
