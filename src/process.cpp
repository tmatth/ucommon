#include <private.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/linked.h>
#include <inc/access.h>
#include <inc/timers.h>
#include <inc/memory.h>
#include <inc/string.h>
#include <inc/thread.h>
#include <inc/file.h>
#include <inc/process.h>

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

keypair::keydata::keydata(keydata **root, const char *kid, const char *value) :
NamedObject((NamedObject **)root, kid)
{
	data = value;
	if(data) {
		strcpy(key, kid);
		id = key;
	}
	else
		key[0] = 0;
}

keypair::keypair(define *defaults, mempager *mem)
{
	keypairs = NULL;
	pager = mem;

	if(defaults)
		load(defaults);
}

keypair::keydata *keypair::create(const char *id, const char *data)
{
	size_t overdraft = cpr_strlen(id);
	if(!data)
		overdraft = 0;

	if(pager)
		return new(pager, overdraft) keydata(&keypairs, id, data);
	else
		return new(overdraft) keydata(&keypairs, id, data);
}

const char *keypair::alloc(const char *data)
{
	if(pager)
		return pager->dup(data);

	return cpr_strdup(data);
}

void keypair::section(FILE *fp, char *buf, size_t size)
{
	stringbuf<256> input;
	const char *pos;
	char *ep;

	for(;;) {
		*buf = 0;
		input << fp;
		if(feof(fp))
			break;
		pos = input.skip(" \t\r\n");
		if(*pos != '[')
			continue;
		cpr_strset(buf, size, ++pos);
		ep = strchr(buf, ']');
		if(ep) {
			*ep = 0;
			break;
		}
	}
}

void keypair::load(FILE *fp, const char *section)
{
	stringbuf<256> input;
	stringbuf<256> value;
	const char *pos, *eq;
	unsigned len = cpr_strlen(section);
	fpos_t fpos;

	if(len)
		rewind(fp);

	while(len) {
		input << fp;
		if(feof(fp))
			return;
		if(NULL == (pos = input.skip(" \t\r\n")))
			continue;
		if(*pos != '[')
			continue;
		if(cpr_strnicmp(section, ++pos, len))
			continue;
		if(pos[len] == ']')
			break;
	}

	for(;;) {
		if(!len)
			fgetpos(fp, &fpos);

		input << fp;
		if(feof(fp))
			return;
		pos = input.skip("\t\r\n ");
		if(!pos)
			continue;
		if(*pos == '[')	{
			if(!len)
				fsetpos(fp, &fpos);
			return;
		}
		if(!isalpha(*pos))
			continue;
		eq = input.chr('=');
		if(!eq)
			continue;
		value.set(eq + 1);
		input.split(eq);
		input.strip(" \t");
		input.lower();
		value.strip(" \t\r\n");
		eq = NULL;
		if(value.at(0) == '\'' || value.at(0) == '\"') {
			value.split(strchr(value[1], value.at(0)));
			++value;
		}
		create(input, value);		
	}
}

void keypair::load(define *list)
{
	keydata *key;
	while(list->key) {
		key = create(list->key);
		key->data = list->data;
		++list;
	}
}

void keypair::dealloc(const char *data)
{
	if(!pager)
		free((char *)data);
}

void keypair::update(keydata *key, const char *value)
{
	const char *old = NULL;

	if(key->data && key->key[0])
		old = key->data;
	
	if(value)
		key->data = alloc(value);
	else
		key->data = NULL;

	if(!key->key[0]) {
		key->key[0] = 1;
		old = NULL;
	}

	if(old)
		dealloc(old);
}

const char *keypair::get(const char *id)
{
    keydata *key = static_cast<keydata *>
        (NamedObject::find(static_cast<NamedObject *>(keypairs), id));

	if(!key)
		return NULL;

	return key->data;
}

void keypair::set(const char *id, const char *value)
{
	keydata *key = static_cast<keydata *>
		(NamedObject::find(static_cast<NamedObject *>(keypairs), id));
	if(key)
		update(key, value);
	else
		create(id, value);
}		

keyconfig::instance::instance(pointer &p) :
locked_release(p)
{
}

keypair *keyconfig::instance::operator[](unsigned idx)
{
	if(!object)
		return NULL;

	return (static_cast<keyconfig *>(object))->operator[](idx);
}

const char *keyconfig::instance::operator()(unsigned idx, const char *key)
{
	keypair *pair;
	if(!object)
		return NULL;

	pair = (static_cast<keyconfig *>(object))->operator[](idx);
	if(!pair)
		return NULL;

	return pair->get(key);
}

keyconfig::keyconfig(unsigned limit, size_t pagesize) :
CountedObject(), mempager(pagesize)
{
	size = limit;
	keypairs = new(static_cast<mempager *>(this)) keypair[limit];
	for(unsigned pos = 0; pos < size; ++pos)
		keypairs[pos].pager = static_cast<mempager *>(this);
}

keypair *keyconfig::operator[](unsigned idx)
{
	crit(idx < size);
	return &keypairs[idx];
}

void keyconfig::dealloc(void)
{
	mempager::purge();
	delete this;
}

void ucc::suspend(timeout_t timeout)
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

void ucc::suspend(void)
{
#ifdef	HAVE_PTHREAD_YIELD
	pthread_yield();
#else
	sched_yield();
#endif
}

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

#ifndef	_MSWINDOWS_
extern "C" void cpr_detach(void)
{
	if(getppid() == 1)
		return;
	cpr_attach("/dev/null");
}

extern "C" void cpr_attach(const char *dev)
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

extern "C" int cpr_spawn(const char *fn, char **args, int mode)
{
	pid_t pid = fork();
	int status;
	if(pid < 0)
		return -1;

	if(pid) {
		switch(mode) {
		case SPAWN_DETACH:
			return 0;
		case SPAWN_NOWAIT:
			return pid;
		case SPAWN_WAIT:
			waitpid(pid, &status, 0);
			return WEXITSTATUS(status);
		}
	}

	cpr_closeall();
	if(mode == SPAWN_DETACH)
		cpr_detach();

	execvp(fn, args);
	exit(-1);
}

extern "C" pid_t cpr_create(const char *fn, char **args, fd_t *iov)
{
	int in[2], out[2];
	if(iov) {
		crit(pipe(in) == 0);
		crit(pipe(out) == 0);
	}
	pid_t pid = fork();
	if(pid < 0 && iov) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		return pid;
	}
	if(pid && iov) {
		iov[0] = out[0];
		close(out[1]);
		iov[1] = in[1];
		close(in[0]);
	}
	if(pid)
		return pid;
	dup2(in[0], 0);
	dup2(out[1], 1);
	cpr_closeall();
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


