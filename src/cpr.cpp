#include <config.h>
#include <ucommon/string.h>
#include <ucommon/service.h>
#include <ucommon/socket.h>
#include <errno.h>
#include <stdarg.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
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

extern "C" pid_t cpr_waitpid(pid_t pid, int *status)
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

extern "C" int cpr_exitpid(pid_t pid)
{
	int status;

	kill(pid, SIGINT);
	cpr_waitpid(pid, &status);
	return status;
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

#if _POSIX_MAPPED_FILES > 0

extern "C" caddr_t cpr_mapfile(const char *fn)
{
	struct stat ino;
	int fd = open(fn, O_RDONLY);
	void *map;

	if(fd < 0)
		return NULL;

	if(fstat(fd, &ino) || !ino.st_size) {
		close(fd);
		return NULL;
	}

	map = mmap(NULL, ino.st_size, PROT_READ, MAP_SHARED, fd, 0);
	msync(map, ino.st_size, MS_SYNC);
	close(fd);
	if(map == MAP_FAILED)
		return NULL;
	
	return (caddr_t)map;
}
#else

extern "C" caddr_t cpr_mapfile(const char *fn)
{
/*	caddr_t mem;
	fd_t fd;
	size_t size;

	fd = cpr_openfile(fn, false);
	if(!cpr_isopen(fd))
		return NULL;

	size = cpr_filesize(fd);
	if(size < 1) {
		cpr_closefile(fd);
		return NULL;
	}
	mem = (caddr_t)malloc(size);
	if(!mem)
		abort();
	
	cpr_readfile(fd, mem, size);
	cpr_closefile(fd);
	return (caddr_t)mem;
*/
}

#endif

extern "C" uint16_t lsb_getshort(uint8_t *b)
{
	return (b[1] * 256) + b[0];
}
	
extern "C" uint32_t lsb_getlong(uint8_t *b)
{
	return (b[3] * 16777216l) + (b[2] * 65536l) + (b[1] * 256) + b[0];
}

extern "C" uint16_t msb_getshort(uint8_t *b)
{
	return (b[0] * 256) + b[1];
}
	
extern "C" uint32_t msb_getlong(uint8_t *b)
{
	return (b[0] * 16777216l) + (b[1] * 65536l) + (b[2] * 256) + b[3];
}

extern "C" void lsb_setshort(uint8_t *b, uint16_t v)
{
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
}

extern "C" void msb_setshort(uint8_t *b, uint16_t v)
{
	b[1] = v & 0x0ff;
	b[0] = (v / 256) & 0xff;
}

extern "C" void lsb_setlong(uint8_t *b, uint32_t v)
{
	b[0] = v & 0x0ff;
	b[1] = (v / 256) & 0xff;
	b[2] = (v / 65536l) & 0xff;
	b[3] = (v / 16777216l) & 0xff;
}

extern "C" void msb_setling(uint8_t *b, uint32_t v)
{
	b[3] = v & 0x0ff;
	b[2] = (v / 256) & 0xff;
	b[1] = (v / 65536l) & 0xff;
	b[0] = (v / 16777216l) & 0xff;
}


extern "C" void *cpr_memalloc(size_t size)
{
	void *mem;

	if(!size)
		++size;

	mem = malloc(size);
	crit(mem != NULL);
	return mem;
}

extern "C" void *cpr_memassign(size_t size, caddr_t addr, size_t max)
{
	if(!addr)
		addr = (caddr_t)malloc(size);
	crit((size <= max));
	return addr;
}

#ifdef	__GNUC__

extern "C" void __cxa_pure_virtual(void)
{
	abort();
}

#endif

// vim: set ts=4 sw=4:
// Local Variables:
// c-basic-offset: 4
// tab-width: 4
// End:
