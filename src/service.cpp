#include <config.h>
#include <ucommon/string.h>
#include <ucommon/service.h>
#include <ucommon/socket.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#if HAVE_ENDIAN_H
#include <endian.h>
#else
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER 1234
#endif
#endif

#ifdef	HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#include <sys/types.h>
#endif

#if HAVE_FTOK
#include <sys/ipc.h>
#include <sys/shm.h>
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

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

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

#if defined(_MSWINDOWS_)

static fd_t duplocal(fd_t fd)
{
	fd_t nh;
	DuplicateHandle(GetCurrentProcess(), fd, GetCurrentProcess(), &nh, 0, 0, DUPLICATE_SAME_ACCESS);
	CloseHandle(fd);
	return nh;
}

#else

static fd_t duplocal(fd_t fd)
{
	return fd;
}

#endif

#if defined(_MSWINDOWS_)

static bool createpipe(fd_t *fd, size_t size)
{
	if(CreatePipe(&fd[0], &fd[1], NULL, size)) {
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}
	return true;
}

#elif defined(HAVE_SOCKETPAIR) && defined(AF_UNIX) && defined(SO_RCVBUF)

static bool createpipe(fd_t *fd, size_t size)
{
	if(!size) {
		if(pipe(fd) == 0)
			return true;
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, fd)) {
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}

	shutdown(fd[1], SHUT_RD);
	shutdown(fd[0], SHUT_WR);
	setsockopt(fd[0], SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	return true;
}

#else

static bool createpipe(fd_t *fd, size_t size)
{
	return pipe(fd) == 0;
}
#endif

#if	defined(HAVE_FTOK) && !defined(HAVE_SHM_OPEN)

#include <sys/ipc.h>
#include <sys/shm.h>

static void ftok_name(const char *name, char *buf, size_t max)
{
	struct stat ino;
	if(*name == '/')
		++name;

	if(!stat("/var/run/ipc", &ino) && S_ISDIR(ino.st_mode))
		snprintf(buf, sizeof(buf), "/var/run/ipc/%s", name);
	else
		snprintf(buf, sizeof(buf), "/tmp/.%s.ipc", name);
}

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

OrderedIndex config::callback::list;
SharedLock config::lock;
config *config::cfg = NULL;

config::callback::callback() :
OrderedObject(&list)
{
}

config::callback::~callback()
{
	release();
}

void config::callback::release(void)
{
	delist(&list);
}

void config::callback::reload(config *keys)
{
}

void config::callback::commit(config *keys)
{
}

config::instance::instance()
{
	config::protect();
}

config::instance::~instance()
{
	config::release();
}

config::config(char *name, size_t s) :
mempager(s), root()
{
	root.setId(name);
}

config::~config()
{
	mempager::purge();
}

config::keynode *config::getPath(const char *id)
{
	const char *np;
	char buf[65];
	char *ep;
	keynode *node = &root, *child;
	caddr_t mp;

	while(id && *id && node) {
		string::set(buf, sizeof(buf), id);
		ep = strchr(buf, '.');
		if(ep)
			*ep = 0;
		np = strchr(id, '.');
		if(np)
			id = ++np;
		else
			id = NULL;
		child = node->getChild(buf);
		if(!child) {
			mp = (caddr_t)alloc(sizeof(keynode));
			child = new(mp) keynode(node, dup(buf));
		}
		node = child;			
	}
	return node;
}

config::keynode *config::addNode(keynode *base, const char *id, const char *value)
{
	caddr_t mp;
	keynode *node;

	mp = (caddr_t)alloc(sizeof(keynode));
	node = new(mp) keynode(base, dup(id));
	if(value)
		node->setPointer(dup(value));
	return node;
}

const char *config::getValue(keynode *node, const char *id, keynode *alt)
{
	node = node->getChild(id);
	if(!node && alt)
		node = alt->getChild(id);

	if(!node)
		return NULL;

	return node->getPointer();
}

config::keynode *config::addNode(keynode *base, define *defs)
{
	keynode *node = getNode(base, defs->key, defs->value);
	if(!node)
		node = addNode(base, defs->key, defs->value);

	for(;;) {
		++defs;
		if(!defs->key)
			return base;
		if(node->getChild(defs->key))
			continue;
		addNode(node, defs->key, defs->value);
	}
	return node;
}


config::keynode *config::getNode(keynode *base, const char *id, const char *text)
{
	linked_pointer<keynode> node = base->getFirst();
	char *cp;
	
	while(node) {
		if(!strcmp(id, node->getId())) {
			cp = node->getData();
			if(cp && !stricmp(cp, text))
				return *node;
		}
		node.next();
	}
	return NULL;
} 

bool config::loadconf(const char *fn, keynode *node, char *gid, keynode *entry)
{
	FILE *fp = fopen(fn, "r");
	bool rtn = false;
	keynode *data;
	caddr_t mp;
	const char *cp;
	char *value;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	while(!feof(fp)) {
		buffer << fp;
		buffer.strip(" \t\r\n");
		if(buffer[0] == '[') {
			if(!buffer.unquote("[]"))
				goto exit;
			value = mempager::dup(*buffer);
			if(gid)
				entry = getNode(node, gid, value);
			else
				entry = node->getChild(value);
			if(!entry) {
				mp = (caddr_t)alloc(sizeof(keynode));
				if(gid) {
					entry = new(mp) keynode(node, gid);
					entry->setPointer(value);
				}
				else					
					entry = new(mp) keynode(node, value);
			}
		}
		if(!buffer[0] || !isalnum(buffer[0]))
			continue;	
		if(!entry)
			continue;
		cp = buffer.chr('=');
		if(!cp)
			continue;
		buffer.split(cp++);
		buffer.chop(" \t=");
		while(isspace(*cp))
			++cp;
		data = entry->getChild(buffer.c_mem());
		if(!data) {
			mp = (caddr_t)alloc(sizeof(keynode));
			data = new(mp) keynode(entry, mempager::dup(*buffer));
		}
		data->setPointer(mempager::dup(cp));
	}

	rtn = true;
exit:
	fclose(fp);
	return rtn;
}

bool config::loadxml(const char *fn, keynode *node)
{
	FILE *fp = fopen(fn, "r");
	char *cp, *ep, *bp, *id;
	ssize_t len = 0;
	bool rtn = false;
	bool document = false, empty;
	keynode *match;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	buffer = "";

	while(node) {
		cp = buffer.c_mem() + buffer.len();
		if(buffer.len() < 1024 - 5) {
			len = fread(cp, 1, 1024 - buffer.len() - 1, fp);
		}
		else
			len = 0;

		if(len < 0)
			goto exit;
		cp[len] = 0;
		if(!buffer.chr('<'))
			goto exit;
		buffer = buffer.c_str();
		cp = buffer.c_mem();

		while(node && cp && *cp)
		{
			cp = string::trim(cp, " \t\r\n");

			if(cp && *cp && !node)
				goto exit;

			bp = strchr(cp, '<');
			ep = strchr(cp, '>');
			if(!ep && bp == cp)
				break;
			if(!bp ) {
				cp = cp + strlen(cp);
				break;
			}
			if(bp > cp) {
				if(node->getData() != NULL)
					goto exit;
				*bp = 0;
				cp = string::chop(cp, " \r\n\t");
				len = strlen(cp);
				ep = (char *)alloc(len + 1);
				string::xmldecode(ep, len, cp);
				node->setPointer(ep);
				*bp = '<';
				cp = bp;
				continue;
			}

			empty = false;	
			*ep = 0;
			if(*(ep - 1) == '/') {
				*(ep - 1) = 0;
				empty = true;
			}
			cp = ++ep;

			if(!strncmp(bp, "</", 2)) {
				if(strcmp(bp + 2, node->getId()))
					goto exit;

				node = node->getParent();
				continue;
			}		

			++bp;

			// if comment/control field...
			if(!isalnum(*bp))
				continue;

			ep = bp;
			while(isalnum(*ep))
				++ep;

			id = NULL;
			if(isspace(*ep)) {
				*(ep++) = 0;
				id = strchr(ep, '\"');
			}
			else if(*ep)
				goto exit;

			if(id) {
				ep = strchr(id + 1, *id);
				if(!ep)
					goto exit;
				*ep = 0;
				++id;
			}

			if(!document) {
				if(strcmp(node->getId(), bp))
					goto exit;
				document = true;
				continue;
			}

			if(id)
				match = getNode(node, bp, id);
			else
				match = node->getChild(bp);
			if(match) {
				if(!id)
					match->setPointer(NULL);
				node = match;
			}
			else 
				node = addNode(node, bp, id);

			if(empty)
				node = node->getParent();
		}
		buffer = cp;
	}
	if(!node && root.getId())
		rtn = true;
exit:
	fclose(fp);
	return rtn;
}

void config::commit(void)
{
	cb = callback::list.begin();
	while(cb) {
		cb->reload(this);
		cb.next();
	}
	lock.lock();

	cb = callback::list.begin();
	while(cb) {
		cb->commit(this);
		cb.next();
	}
	cfg = this;
	lock.unlock();
}

void config::update(void)
{
	lock.lock();
	if(cb)
		cb.next();

	while(cb) {
		cb->reload(this);
		cb->commit(this);
		cb.next();
	}
	lock.unlock();
}

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

	if(*fn == '/')
		++fn;

	if(len) {
		prot = FILE_MAP_WRITE;
		mode |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, len, fn);
	}
	else
		fd = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fn);
	
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
	char fbuf[65];

	size = 0;
	used = 0;

	if(*fn != '/') {
		snprintf(fbuf, sizeof(fbuf), "/%s", fn);
		fn = fbuf;
	}
	
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

service::service(size_t ps, define *def) :
mempager(ps)
{
	root = NULL;

	while(def && def->name) {
		set(def->name, def->value);
		++def;
	}
}

service::~service()
{
	purge();
}

void service::setenv(define *def)
{
	while(def && def->name) {
		setenv(def->name, def->value);
		++def;
	}
}

void service::setenv(const char *id, const char *value)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);
#ifdef	HAVE_SETENV
	::setenv(id, value, 1);
#else
	char buf[128];
	snprintf(buf, sizeof(buf), "%s=%s", id, value);
	::putenv(buf);
#endif
	pthread_mutex_unlock(&mutex);
}

const char *service::get(const char *id)
{
	member *key = find(id);
	if(!key)
		return NULL;

	return key->value;
}

#ifdef	_MSWINDOWS_
char **service::getEnviron(void)
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

bool service::reload(const char *id)
{
	return control(id, "%s\n", "reload");
}

bool service::shutdown(const char *id)
{
	return control(id, "%s\n", "down");
}

bool service::terminate(const char *id)
{
	return control(id, "%s\n", "quit");
}

#else

void service::block(int signo)
{
	sigset_t set;
	
	sigemptyset(&set);
	sigaddset(&set, signo);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
	
void service::setEnviron(void)
{
	const char *id;

	linked_pointer<service::member> env = begin();

	while(env) {
#ifdef	HAVE_SETENV
		::setenv(env->getId(), env->value, 1);
#else
		char buf[128];
		snprintf(buf, sizeof(buf), "%s=%s", env->getId(), env->value);
		putenv(buf);
#endif
		env.next();
	}

	if(getuid() == 0 && getppid() < 2)
		umask(002);

	id = get("UID");
	if(id && getuid() == 0) {
		if(getppid() > 1)
			umask(022);
		setuid(atoi(id));
	}

	id = get("HOME");
	if(id)
		chdir(id);

	id = get("PWD");
	if(id)
		chdir(id);	
}

pid_t service::pidfile(const char *id, pid_t pid)
{
	char buf[128];
	pid_t opid;
	struct stat ino;
	fd_t fd;

	snprintf(buf, sizeof(buf), "/var/run/%s", id);
	mkdir(buf, 0775);
	if(!stat(buf, &ino) && S_ISDIR(ino.st_mode)) 
		snprintf(buf, sizeof(buf), "/var/run/%s/%s.pid", id, id);
	else
		snprintf(buf, sizeof(buf), "/tmp/%s.pid", id);

retry:
	fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0755);
	if(fd < 0) {
		opid = pidfile(id);
		if(!opid || opid == 1 && pid > 1) {
			remove(buf);
			goto retry;
		}
		return opid;
	}

	if(pid > 1) {
		snprintf(buf, sizeof(buf), "%d\n", pid);
		write(fd, buf, strlen(buf));
	}
	close(fd);
	return 0;
}

pid_t service::pidfile(const char *id)
{
	struct stat ino;
	time_t now;
	char buf[128];
	fd_t fd;
	pid_t pid;

	snprintf(buf, sizeof(buf), "/var/run/%s", id);
	if(!stat(buf, &ino) && S_ISDIR(ino.st_mode))
		snprintf(buf, sizeof(buf), "/var/run/%s/%s.pid", id, id);
	else
		snprintf(buf, sizeof(buf), "/tmp/%s.pid", id);

	fd = open(buf, O_RDONLY);
	if(fd < 0 && errno == EPERM)
		return 1;

	if(fd < 0)
		return 0;

	if(read(fd, buf, 16) < 1) {
		goto bydate;
	}
	buf[16] = 0;
	pid = atoi(buf);
	if(pid == 1)
		goto bydate;

	close(fd);
	if(running(pid))
		return 0;

	return pid;

bydate:
	time(&now);
	fstat(fd, &ino);
	close(fd);
	if(ino.st_mtime + 30 < now)
		return 0;
	return 1;
}

bool service::reload(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGHUP);
	return true;
}

bool service::shutdown(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGINT);
	return true;
}

bool service::terminate(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGTERM);
	return true;
}

void service::detach(void)
{
	if(getppid() == 1)
		return;
	service::attach("/dev/null");
}

void service::attach(const char *dev)
{
	pid_t pid;
	int fd;

	close(0);
	close(1);
	close(2);
#ifdef	SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif

#ifdef	SIGTTIN
	signal(SIGTTIN, SIG_IGN);
#endif

#ifdef	SIGTSTP
	signal(SIGTSTP, SIG_IGN);
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
	signal(SIGHUP, SIG_IGN);
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

#endif

void service::dup(const char *id, const char *value)
{
	member *env = find(id);

	if(!env) {
		caddr_t mp = (caddr_t)alloc(sizeof(member));
		env = new(mp) member(&root, mempager::dup(id));
	}
	env->value = mempager::dup(value);
};

void service::set(char *id, const char *value)
{
    member *env = find(id);
	
	if(!env) {
	    caddr_t mp = (caddr_t)alloc(sizeof(member));
	    env = new(mp) member(&root, id);
	}

    env->value = value;
};

#ifdef _MSWINDOWS_

int service::spawn(const char *fn, char **args, spawn_t mode, pid_t *pid, fd_t *iov, service *env)
{
	char buf[32000];
	STARTUPINFO start;
	PROCESS_INFORMATION ps;
	BOOL result;
	unsigned pos;
	DWORD cflags = 0;
	char **envp = NULL;
	char *pwd = NULL;
	DWORD status = 0;

	if(env) {
		pwd = const_cast<char *>(env->get("PWD"));
		envp = env->getEnviron();
	}

	snprintf(buf, sizeof(buf), "%s", fn);
	pos = strlen(buf);
	while(pos < sizeof(buf) - 2 && args && *args) {
		snprintf(buf + pos, sizeof(buf) - pos - 2, " %s", *(args++));
		pos = strlen(buf);
	}

	buf[pos] = 0;
	
	memset(&ps, 0, sizeof(ps));
	memset(&start, 0, sizeof(start));
	start.dwFlags = 0x101;
	start.wShowWindow = 0;

	if(iov && iov[0] != INVALID_HANDLE_VALUE) {
		start.hStdInput = iov[0];
		if(iov[1] != INVALID_HANDLE_VALUE) {
			start.hStdOutput = iov[1];
			if(iov[2] != INVALID_HANDLE_VALUE) {
				start.hStdError = iov[2];
			}
		}
	}

	switch(mode) {
	SPAWN_DETACH:
		cflags = CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;
		break;
	SPAWN_NOWAIT:
	SPAWN_WAIT:
		break;
	}

	result = CreateProcess(NULL, buf, NULL, NULL, TRUE, cflags, env, pwd, &start, &ps);
	if(!result)
		return -1;

	*pid = ps.dwProcessId;
	if(ps.hProcess == NULL || ps.hProcess == INVALID_HANDLE_VALUE)
		return -1;

	if(mode == SPAWN_WAIT) {
		WaitForSingleObject(ps.hProcess, INFINITE);
		GetExitCodeProcess(ps.hProcess, &status);
	}
	else if(mode == SPAWN_NOWAIT)
		WaitForInputIdle(ps.hProcess, 10);
	
	CloseHandle(ps.hThread);
	CloseHandle(ps.hProcess);
	return status;
}

int service::wait(pid_t pid)
{
	DWORD status = (DWORD)(-1);
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	if(hProc && hProc != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(hProc, INFINITE);
		GetExitCodeProcess(hProc, &status);
		CloseHandle(hProc);
	}
	return status;
}	

bool service::getexit(pid_t pid, int *result)
{
    DWORD status = STILL_ACTIVE;
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

    if(hProc && hProc != INVALID_HANDLE_VALUE) {
        GetExitCodeProcess(hProc, &status);
        CloseHandle(hProc);
    }
	if(status == STILL_ACTIVE || !hProc || hProc == INVALID_HANDLE_VALUE)
		return false;
    return *result = status;
	return true;
}

/* void service::terminate(pid_t pid)
{
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	if(hProc && hProc != INVALID_HANDLE_VALUE) {
		TerminateProcess(hProc, 0 );
		CloseHandle(hProc);
	}
}
*/
bool service::running(pid_t pid)
{
	HANDLE hProc;
	DWORD status;

	hProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	if(hProc != NULL) {
		CloseHandle(hProc);
		return true;
	}

	return false;
}

#else

bool service::getexit(pid_t pid, int *status)
{
	if(waitpid(pid, status, WNOHANG) != pid)
		return false;

	*status = WEXITSTATUS(*status);
	return true;
}

int service::wait(pid_t pid)
{
	int status;
	waitpid(pid, &status, 0);
	return WEXITSTATUS(status);
}

bool service::running(pid_t pid)
{
	if((kill(pid, 0) == -1) && (errno == ESRCH))
		return false;

	return true;
}

int service::spawn(const char *fn, char **args, spawn_t mode, pid_t *pid, fd_t *iov, service *env)
{
	unsigned max = OPEN_MAX, idx = 0;

	*pid = fork();

	if(*pid < 0)
		return -1;

	if(*pid) {
		closeiov(iov);
		switch(mode) {
		case SPAWN_DETACH:
		case SPAWN_NOWAIT:
			return 0;
		case SPAWN_WAIT:
			return wait(*pid);
		}
	}

	while(iov && *iov > -1) {
		if(*iov != (fd_t)idx)
			dup2(*iov, idx);
		++iov;
		++idx;
	}

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

	while(idx < max)
		close(idx++);

	if(mode == SPAWN_DETACH)
		detach();

	if(env)
		env->setEnviron();

	execvp(fn, args);
	exit(-1);
}

void service::closeiov(fd_t *iov)
{
	unsigned idx = 0, np;

	while(iov && iov[idx] > -1) {
		if(iov[idx] != (fd_t)idx) {
			close(iov[idx]);
			np = idx;
			while(iov[++np] != -1) {
				if(iov[np] == iov[idx])
					iov[np] = (fd_t)np;
			}
		}
		++idx;
	}
}

void service::createiov(fd_t *fd)
{
	fd[0] = 0;
	fd[1] = 1;
	fd[2] = 2;
	fd[3] = INVALID_HANDLE_VALUE;
}

void service::attachiov(fd_t *fd, fd_t io)
{
	fd[0] = fd[1] = io;
	fd[2] = INVALID_HANDLE_VALUE;
}

void service::detachiov(fd_t *fd)
{
	fd[0] = open("/dev/null", O_RDWR);
	fd[1] = fd[2] = fd[0];
	fd[3] = INVALID_HANDLE_VALUE;
}

#endif

fd_t service::pipeInput(fd_t *fd, size_t size)
{
	fd_t pfd[2];

	if(!createpipe(pfd, size))
		return INVALID_HANDLE_VALUE;

	fd[0] = pfd[0];
	return duplocal(pfd[1]);
}

fd_t service::pipeOutput(fd_t *fd, size_t size)
{
	fd_t pfd[2];

	if(!createpipe(pfd, size))
		return INVALID_HANDLE_VALUE;

	fd[1] = pfd[1];
	return duplocal(pfd[0]);
}

fd_t service::pipeError(fd_t *fd, size_t size)
{
	return pipeOutput(++fd, size);
}	

#ifdef _MSWINDOWS_

static HANDLE hEvent = INVALID_HANDLE_VALUE;
static HANDLE hFIFO = INVALID_HANDLE_VALUE;
static HANDLE hLoopback = INVALID_HANDLE_VALUE;

static void ctrl_name(char *buf, const char *id, size_t size)
{
	if(*id == '/')
		++id;

	snprintf(buf, size, "\\\\.\\mailslot\\%s_ctrl", id);
}

bool service::control(const char *id, const char *fmt, ...)
{
	va_list args;
	fd_t fd;
	char buf[464];
	char *ep;
	size_t len;
	DWORD msgresult;
	BOOL result;

	va_start(args, fmt);

	if(id) {
		ctrl_name(buf, id, 65);
		fd = CreateFile(buf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(fd == INVALID_HANDLE_VALUE)
			return false;
	}
	else if(hLoopback != INVALID_HANDLE_VALUE)
		fd = hLoopback;
	else 
		return false;

	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args); 
	
	ep = strchr(buf, '\n');
	if(ep)
		*ep = 0;
	
	result = WriteFile(fd, buf, (DWORD)strlen(buf) + 1, &msgresult, NULL);

	if(hLoopback != fd)
		CloseHandle(fd);

	return result;
}

size_t service::createctrl(const char *id)
{
	char buf[65];

	ctrl_name(buf, id, sizeof(buf));

	hFIFO = CreateMailslot(buf, 0, MAILSLOT_WAIT_FOREVER, NULL);
	if(hFIFO == INVALID_HANDLE_VALUE)
		return 0;

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hLoopback = CreateFile(buf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	return 464;
}

void service::releasectrl(const char *id)
{
	if(hFIFO != INVALID_HANDLE_VALUE) {
		CloseHandle(hFIFO);
		CloseHandle(hLoopback);
		CloseHandle(hEvent);
		hFIFO = hLoopback = INVALID_HANDLE_VALUE;
	}
}

size_t service::receive(char *buf, size_t max)
{
	BOOL result;
	DWORD msgresult;
	static OVERLAPPED ov;

	*buf = 0;
	if(hFIFO == INVALID_HANDLE_VALUE)
		return 0;

	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.hEvent = hEvent;
	ResetEvent(hEvent);
	result = ReadFile(hFIFO, buf, max - 1, &msgresult, &ov);
	if(!result && GetLastError() == ERROR_IO_PENDING) {
		int ret = WaitForSingleObject(ov.hEvent, INFINITE);
		if(ret != WAIT_OBJECT_0)
			return 0;
		result = GetOverlappedResult(hFIFO, &ov, &msgresult, TRUE);
	} 

	if(!result || msgresult < 1)
		return 0;

	buf[msgresult] = 0;
	return msgresult + 1;
}

void service::logfile(const char *id, const char *name, const char *fmt, ...)
{
}

void service::openlog(const char *id)
{
}

void service::errlog(err_t log, const char *fmt, ...)
{
	char buf[128];
	const char *level = "error";
	va_list args;
	
	va_start(args, fmt);

	switch(log)
	{
	case SERVICE_INFO:
		level = "info";
		break;
	case SERVICE_NOTICE:
		level = "notice";
		break;
	case SERVICE_WARNING:
		level = "warning";
		break;
	case SERVICE_ERROR:
		level = "error";
		break;
	case SERVICE_FAILURE:
		level = "crit";
		break;
	}

	snprintf(buf, sizeof(buf), "%s %s", "[%s]", fmt);

	vfprintf(stderr, buf, args);
	va_end(args);

	if(log == SERVICE_FAILURE)
		abort();
}

#else

static FILE *fifo = NULL;

static void ctrl_name(char *buf, const char *id, size_t size)
{
	struct stat ino;
	if(*id == '/')
		++id;

	snprintf(buf, size, "/var/run/%s", id);

	if(!stat(buf, &ino) && S_ISDIR(ino.st_mode))	
		snprintf(buf, size, "/var/run/%s/%s.ctrl", id, id);
	else
		snprintf(buf, size, "/tmp/.%s.ctrl", id);
}

void service::logfile(const char *id, const char *name, const char *fmt, ...)
{
	char buffer[256];
	char path[256];
	char *ep;
	int fd;
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);

	if(*id == '/')
		++id;
	snprintf(path, sizeof(path), "/var/log/%s/%s", id, name);
	ep = buffer + strlen(buffer);
	if(ep > buffer) {
		--ep;
		if(*ep != '\n') {
			*(++ep) = '\n';
			*ep = 0;
		}
	}

	fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0770);
	assert(fd > -1);
	if(fd < 0)
		return;

	write(fd, buffer, strlen(buffer));
	fsync(fd);
	close(fd);

	va_end(args);
}

void service::openlog(const char *id)
{
	if(*id == '/')
		++id;

	if(getppid() == 1)
		::openlog(id, LOG_CONS | LOG_NDELAY, LOG_DAEMON);
	else
		::openlog(id, LOG_PERROR, LOG_USER);
}

void service::errlog(err_t log, const char *fmt, ...)
{
	int level = LOG_ERR;
	va_list args;
	
	va_start(args, fmt);

	switch(log)
	{
	case SERVICE_INFO:
		level = LOG_INFO;
		break;
	case SERVICE_NOTICE:
		level = LOG_NOTICE;
		break;
	case SERVICE_WARNING:
		level = LOG_WARNING;
		break;
	case SERVICE_ERROR:
		level = LOG_ERR;
		break;
	case SERVICE_FAILURE:
		level = LOG_CRIT;
		break;
	}

	::vsyslog(level, fmt, args);
	va_end(args);

	if(level == LOG_CRIT)
		abort();
}

bool service::control(const char *id, const char *fmt, ...)
{
	va_list args;
	int fd;
	char buf[512];
	char *ep;
	size_t len;

	va_start(args, fmt);

	if(id) {
		ctrl_name(buf, id, 65);
		fd = open(buf, O_RDWR);
		if(fd < 0)
			return false;
	}
	else if(fifo)
		fd = fileno(fifo);
	else 
		return false;

	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args); 
	
	ep = strchr(buf, '\n');
	if(ep)
		*ep = 0;

	len = strlen(buf);
	buf[len++] = '\n';
		
	write(fd, buf, len);
	if(!fifo || fileno(fifo) != fd)
		close(fd);

	return true;
}

size_t service::createctrl(const char *id)
{
	char buf[65];

	ctrl_name(buf, id, sizeof(buf));
	remove(buf);
	if(mkfifo(buf, 0660))
		return 0;

	fifo = fopen(buf, "r+");
	if(fifo) 
		return 512;
	else
		return 0;
}

void service::releasectrl(const char *id)
{
	char buf[65];

	if(!fifo)
		return;

	ctrl_name(buf, id, sizeof(buf));

	fclose(fifo);
	fifo = NULL;
	remove(buf);
}

size_t service::receive(char *buf, size_t max)
{
	char *cp;

	if(!fifo)
		return 0;

	cp = fgets(buf, max, fifo);
	if(!cp)
		return 0;

	cp = strchr(buf, '\n');
	if(cp) {
		*cp = 0;
		return strlen(buf);
	}
	else
		return 0;
}

#endif

int service::scheduler(int policy, priority_t priority)
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
		setpriority(PRIO_PROCESS, 0, -(priority - PRIORITY_NORMAL));
	else
		setpriority(PRIO_PROCESS, 0, -(priority - PRIORITY_LOW));

	memset(&sparam, 0, sizeof(sparam));
	sparam.sched_priority = pri;
	return sched_setscheduler(0, policy, &sparam);	
#elif defined(_MSWINDOWS_)
	return service::priority(priority);
#endif
}

int service::priority(priority_t priority)
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
	case PRIORITY_LOWEST:
	case PRIORITY_LOW:
		pri = THREAD_PRIORITY_LOWEST;
		break;
	case PRIORITY_NORMAL:
		pri = THREAD_PRIORITY_NORMAL;
		break;
	}
	SetThreadPriority(GetCurrentThread(), priority);
	return 0;
#endif
}

// vim: set ts=4 sw=4:

