// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/string.h>
#include <ucommon/memory.h>
#include <ucommon/shell.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#ifdef	_MSWINDOWS_
#include <process.h>
#else
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

using namespace UCOMMON_NAMESPACE;

OrderedIndex shell::Option::index;

shell::pipeio::pipeio()
{
	pid = INVALID_PID_VALUE;
	input = output = INVALID_HANDLE_VALUE;
	perror = presult = 0;
}

#ifdef	_MSWINDOWS_

int shell::pipeio::cancel(void)
{
	TerminateProcess(pid, 7);
	return wait();
}

int shell::pipeio::wait(void)
{
	DWORD code;

	presult = -1;

	if(input != INVALID_HANDLE_VALUE)
		CloseHandle(input);

	if(output != INVALID_HANDLE_VALUE)
		CloseHandle(output);

	input = output = INVALID_HANDLE_VALUE;

	if(WaitForSingleObject(pid, INFINITE) == WAIT_FAILED) {
		pid = INVALID_PID_VALUE;
		return -1;
	}

	GetExitCodeProcess(pid, &code);
	CloseHandle(pid);
	pid = INVALID_PID_VALUE;	
	presult = code;
	return (int)code;
}

size_t shell::pipeio::read(void *address, size_t size)
{
	DWORD count;

	if(input == INVALID_HANDLE_VALUE || perror)
		return 0;

	if(ReadFile(input, (LPVOID)address, (DWORD)size, &count, NULL))
		return (size_t)count;
	
	perror = fsys::remapError();
	return 0;
}		

size_t shell::pipeio::write(const void *address, size_t size)
{
	DWORD count;

	if(output == INVALID_HANDLE_VALUE || perror)
		return 0;

	if(WriteFile(output, (LPVOID)address, (DWORD)size, &count, NULL))
		return (size_t)count;
	
	perror = fsys::remapError();
	return 0;
}		

#else

int shell::pipeio::spawn(const char *path, char **argv, pmode_t mode, size_t buffer, char **envp)
{
	fd_t pin[2], pout[2];
	fd_t stdio[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 2};

	pin[0] = INVALID_HANDLE_VALUE;
	pout[1] = INVALID_HANDLE_VALUE;
	stdio[2] = 2;

	if(mode == RD || mode == RDWR)
	{
		::pipe(pin);
		stdio[1] = pin[1];
	}
	if(mode == WR || mode == RDWR) {
		::pipe(pout);
		stdio[0] = pout[0];
	}

	pid = shell::spawn(path, argv, envp, stdio);

	if(mode == RD || mode == RDWR)
		::close(pin[1]);
	if(mode == WR || mode == RDWR)
		::close(pout[0]);

	perror = 0;
	if(pid == INVALID_PID_VALUE) {
		perror = errno;
		if(mode == RD || mode == RDWR) {
			::close(pin[0]);
			pin[0] = INVALID_HANDLE_VALUE;
		}
		if(mode == WR || mode == RDWR) {
			::close(pout[1]);
			pout[1] = INVALID_HANDLE_VALUE;
		}
	}

	input = pin[0];
	output = pout[1];
	return perror;
}
	
int shell::pipeio::wait(void)
{
	presult = -1;

	if(input != INVALID_HANDLE_VALUE)
		::close(input);

	if(output != INVALID_HANDLE_VALUE)
		::close(output);

	input = output = INVALID_HANDLE_VALUE;

	if(pid == INVALID_PID_VALUE || ::waitpid(pid, &presult, 0) != pid)
		presult = -1;

	pid = INVALID_PID_VALUE;

	if(presult == -1)
		return -1;

	presult = WEXITSTATUS(presult);
	return presult;
}

int shell::pipeio::cancel(void)
{
	if(kill(pid, SIGTERM))
		return -1;
	return wait();
}

size_t shell::pipeio::read(void *address, size_t size)
{
	if(input == INVALID_HANDLE_VALUE || perror)
		return 0;

	ssize_t result = ::read(input, address, size);
	if(result < 0) {
		perror = fsys::remapError();
		return 0;
	}
	return (size_t)result;
}		

size_t shell::pipeio::write(const void *address, size_t size)
{
	if(output == INVALID_HANDLE_VALUE || perror)
		return 0;

	ssize_t result = ::write(output, address, size);
	if(result < 0) {
		perror = fsys::remapError();
		return 0;
	}
	return (size_t)result;
}		

#endif

shell::Option::Option(char shortopt, const char *longopt, const char *value, const char *help) :
OrderedObject(&index)
{
	while(longopt && *longopt == '-')
		++longopt;

	short_option = shortopt;
	long_option = longopt;
	uses_option = value;
	help_string = help;
}

shell::Option::~Option()
{
}

shell::flagopt::flagopt(char short_option, const char *long_option, const char *help_string, bool single_use) :
shell::Option(short_option, long_option, NULL, help_string)
{
	single = single_use;
	counter = 0;
}

const char *shell::flagopt::assign(const char *value)
{
	if(single && counter)
		return shell::errmsg(shell::OPTION_USED);

	++counter;
	return NULL;
}

shell::numericopt::numericopt(char short_option, const char *long_option, const char *help_string, const char *type, long def_value) :
shell::Option(short_option, long_option, type, help_string)
{
	used = false;
	number = def_value;
}

const char *shell::numericopt::assign(const char *value)
{
	char *endptr = NULL;

	if(used)
		return errmsg(shell::OPTION_USED);

	number = strtol(value, &endptr, 0);
	if(!endptr || *endptr != 0)
		return errmsg(shell::BAD_VALUE);

	return NULL;
}

shell::stringopt::stringopt(char short_option, const char *long_option, const char *help_string, const char *type, const char *def_value) :
shell::Option(short_option, long_option, type, help_string)
{
	used = false;
	text = def_value;
}

const char *shell::stringopt::assign(const char *value)
{
	if(used)
		return shell::errmsg(shell::OPTION_USED);

	text = value;
	return NULL;
}

char shell::stringopt::operator[](size_t index)
{
	if(!text)
		return 0;

	if(index >= strlen(text))
		return 0;

	return text[index];
}

shell::charopt::charopt(char short_option, const char *long_option, const char *help_string, const char *type, char def_value) :
shell::Option(short_option, long_option, type, help_string)
{
	used = false;
	code = def_value;
}

const char *shell::charopt::assign(const char *value)
{
	long number;
	char *endptr = NULL;

	if(used)
		return shell::errmsg(shell::OPTION_USED);

	if(value[1] == 0) {
		code = value[0];
		return NULL;
	}

	number = strtol(value, &endptr, 0);
	if(!endptr || *endptr != 0)
		return errmsg(shell::BAD_VALUE);

	if(number < 0 || number > 255)
		return errmsg(shell::BAD_VALUE);

	code = (char)(number);
	return NULL;
}

void shell::collapse(LinkedObject *first)
{
	char **argv = _argv = (char **)mempager::alloc(sizeof(char **) * (_argc + 1));
	linked_pointer<args> ap = first;
	while(is(ap)) {
		*(argv++) = ap->item;
		ap.next();
	}
	*argv = NULL;
}

void shell::set0(char *argv0)
{
	_argv0 = strrchr(argv0, '/');
#ifdef	_MSWINDOWS_
	if(!_argv0)
		_argv0 = strrchr(argv0, '\\');
	if(!_argv0)
		_argv0 = strchr(argv0, ':');
#endif
	if(!_argv0)
		_argv0 = argv0;
	else
		++_argv0;

	_argv0 = dup(_argv0);

#ifdef	_MSWINDOWS_
	char *ext = strrchr(_argv0, '.');
	if(ieq(ext, ".exe") || ieq(ext, ".com"))
		*ext = 0;
#endif
}

shell::shell(size_t pagesize) :
mempager(pagesize)
{
	_argv = NULL;
	_argc = 0;
}

shell::shell(const char *string, size_t pagesize) :
mempager(pagesize)
{
	parse(string);
}

shell::shell(int argc, char **argv, size_t pagesize) :
mempager(pagesize)
{
	parse(argc, argv);
}

static const char *msgs[] = {
	"missing command line arguments",
	"missing argument for option",
	"option does not have argument",
	"unknown command option",
	"option already used",
	"invalid argument used",
	NULL};

const char *shell::errmsg(errmsg_t id)
{
	return msgs[id];
}

void shell::errmsg(errmsg_t id, const char *text)
{
	msgs[id] = text;
}

unsigned shell::count(char **argv)
{
	unsigned count = 0;

	while(argv && *argv[count])
		++count;

	return count;
}

void shell::help(void)
{
	linked_pointer<Option> op = Option::first();
	unsigned hp = 0;
	while(is(op)) {
		if(!op->help_string) {
			op.next();
			continue;
		}
		if(op->short_option && op->long_option)
			printf("  -%c, ", op->short_option);
		else if(op->long_option)
			printf("     ");
		else
			printf("  -%c ", op->short_option);
		hp = 5;
		if(op->long_option && op->uses_option) {
			printf("%s=%s", op->long_option, op->uses_option);
			hp += strlen(op->long_option) + strlen(op->uses_option) + 1;
		}
		else if(op->long_option) {
			printf("%s", op->long_option);
			hp += strlen(op->long_option);
		}
		if(hp > 29) {
			printf("\n");
			hp = 0;
		}
		while(hp < 30) {
			putchar(' ');
			++hp;
		} 
		const char *hs = op->help_string;
		while(*hs) {
			if(*hs == '\n' || ((*hs == ' ' || *hs == '\t')) && hp > 75) {
				printf("\n                              ");
				hp = 30;
			}
			else if(*hs == '\t') {
				if(!hp % 8) {
					putchar(' ');
					++hp;
				}
				while(hp % 8) {
					putchar(' ');
					++hp;
				}
			}
			else
				putchar(*hs);
			++hs;
		}
		printf("\n");
		op.next();
	}
}

char **shell::parse(const char *string)
{
	assert(string != NULL);

	args *arg;
	char quote = 0;
	char *cp = mempager::dup(string);
	bool active = false;
	OrderedIndex arglist;

	_argc = 0;

	while(*cp) {
		if(isspace(*cp) && active && !quote) {
inactive:
			active = false;
			*(cp++) = 0;
			continue;
		}
		if(*cp == '\'' && !active) {
			quote = *cp;
			goto argument;
		}
		if(*cp == '\"' && !active) {
			quote = *(cp++);
			goto argument;
		}
		if(*cp == quote && active) {
			if(quote == '\"')
				goto inactive;
			if(isspace(cp[1])) {
				++cp;
				goto inactive;
			}
		}
		if(!isspace(*cp) && !active) {
argument:
			++_argc;
			active = true;
			arg = init<args>((args *)mempager::alloc(sizeof(args)));
			arg->item = (cp++);
			arg->enlist(&arglist);
			continue;
		}
		++cp;
	}
	collapse(arglist.begin());
	set0(*_argv);
	return _argv;
}

int shell::systemf(const char *format, ...)
{
	va_list args;
	char buffer[1024];
	
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	return system(buffer);
}

void shell::parse(int argc, char **argv)
{
	int argp = 1;
	char *arg, *opt;
	unsigned len;
	const char *value;
	const char *err;

	if(argc < 1 || !argv)
		errexit(-1, "*** %s\n", errmsg(shell::NOARGS));

	set0(argv[0]);

	while(argp < argc) {
		if(eq(argv[argp], "--")) {
			++argp;
			break;
		}
		arg = opt = argv[argp];
		if(*arg != '-')
			break;

		++argp;

		linked_pointer<Option> op = Option::first();
		err = NULL;
		value = NULL;

		++opt;
		if(*opt == '-')
			++opt;

		// long option parsing...

		while(is(op)) {
			if(!op->long_option) {
				op.next();
				continue;
			}
			len = strlen(op->long_option);
			value = NULL;
			if(op->long_option && eq(op->long_option, opt, len)) {
				if(opt[len] == '=' && !op->uses_option) 			
					errexit(1, "*** %s: --%s: %s\n", _argv0, op->long_option, errmsg(shell::INVARGUMENT));
				if(opt[len] == '=') {
					value = opt + len + 1;
					break;
				}
				if(opt[len] == 0) {
					if(op->uses_option)
						value = argv[argp++];
					break;
				}
			}
			op.next();
		}

		// if we have long option, try to assign it...
		if(is(op)) {
			if(op->uses_option && value == NULL)
				errexit(1, "*** %s: --%s: %s\n", _argv0, op->long_option, errmsg(shell::NOARGUMENT));
			err = op->assign(value);
			if(err)
				errexit(1, "*** %s: --%s: %s\n", _argv0, op->long_option, err);
			continue;
		}

		// if unknown long option was used...
		if(eq(arg, "--", 2)) {
			char *cp = strchr(arg, '=');
			if(cp)
				*cp = 0;
			errexit(1, "*** %s: %s: %s\n", _argv0, arg, errmsg(shell::BADOPTION));
		}

		// short form -xyz flags parsing...
	
		while(*(++arg) != 0) {
			value = NULL;

			op = Option::first();
			while(is(op)) {
				if(op->short_option == *arg) 
					break;
				op.next();
			}
			
			if(!is(op))
				errexit(1, "*** %s: -%c: %s\n", _argv0, *arg, errmsg(shell::BADOPTION));
			if(op->uses_option && arg[1] == 0) {
				value = argv[argp++];
				break;
			}
			if(op->uses_option) {
				value = ++arg;
				break;
			}
	
			if(is(op)) {
				if(op->uses_option && value == NULL)
					errexit(1, "*** %s: -%c: %s\n", _argv0, op->short_option, errmsg(shell::NOARGUMENT));
				err = op->assign(value);
				if(err)
					errexit(1, "*** %s: -%c: %s\n", _argv0, op->short_option, err);
			}
		}
	}
	_argv = &argv[argp];
	_argc = argc - argp;

#if defined(_MSWINDOWS_) && defined(_MSC_VER)
	const char *fn;
	char dirname[128];
	WIN32_FIND_DATA entry;
	args *argitem;
	fd_t dir;
	OrderedIndex arglist;
	_argc = 0;

	while(_argv != NULL && *_argv != NULL) {
		fn = strrchr(*_argv, '/');
		arg = *_argv;
		if(!fn)
			fn = strrchr(*_argv, '\\');
		if(!fn && arg[1] == ':')
			fn = strrchr(*_argv, ':');
		if(fn)
			++fn;
		else
			fn = *_argv;
		if(!*fn)
			goto skip;
		// url type things do not get expanded...
		if(strchr(fn, ':'))
			goto skip;
		if(*fn != '*' && fn[strlen(fn) - 1] != '*' && !strchr(fn, '?'))
			goto skip;
		if(eq(fn, "*"))
			fn = "*.*";
		len = fn - *_argv;
		if(len >= sizeof(dirname))
			len = sizeof(dirname) - 1;
		if(len == 0)
			dirname[0] = 0;
		else
			String::set(dirname, ++len, *_argv);	
		len = strlen(dirname);
		if(len)
			String::set(dirname + len, sizeof(dirname) - len, fn);
		else
			String::set(dirname, sizeof(dirname), fn);
		dir = FindFirstFile(dirname, &entry);
		if(dir == INVALID_HANDLE_VALUE)
			goto skip;
		do {
			if(len)
				String::set(dirname + len, sizeof(dirname) - len, fn);
			else
				String::set(dirname, sizeof(dirname), fn);
			argitem = init<args>((args *)mempager::alloc(sizeof(args)));
			argitem->item = mempager::dup(dirname);
			argitem->enlist(&arglist);
			++_argc;
		} while(FindNextFile(dir, &entry));
		CloseHandle(dir);
		++*_argv;
		continue;
skip:
		argitem = init<args>((args *)mempager::alloc(sizeof(args)));
		argitem->item = *(_argv++);
		argitem->enlist(&arglist);
		++_argc;
	}
	collapse(arglist.begin());
#endif
}

void shell::errexit(int exitcode, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
	if(exitcode)
		::exit(exitcode);
}

void shell::printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	fflush(stdout);
	va_end(args);
}

int shell::cancel(shell::pipe_t *pio)
{
	int status = pio->cancel();
	delete pio;
	return status;
}

int shell::wait(shell::pipe_t *pio)
{
	int status = pio->wait();
	delete pio;
	return status;
}

#ifdef _MSWINDOWS_

int shell::system(const char *cmd, const char **envp)
{
	char cmdspec[128];
	DWORD code = -1;
	PROCESS_INFORMATION pi;
	char *ep = NULL;
	unsigned len = 0;

	if(envp)
		ep = new char[4096];

	while(envp && *envp && len < 4090) {
		String::set(ep + len, 4094 - len, *envp);
		len += strlen(*(envp++)) + 1;
	}

	if(ep)
		ep[len] = 0;

	if(!GetEnvironmentVariable("SHELL", cmdspec, sizeof(cmdspec)))
		GetEnvironmentVariable("ComSpec", cmdspec, sizeof(cmdspec));
	
	if(!CreateProcess((CHAR *)cmdspec, (CHAR *)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, ep, NULL, NULL, &pi)) {
		if(ep)
			delete[] ep;
		return -1;
	}
	if(ep)
		delete[] ep;

	CloseHandle(pi.hThread);
	int status = wait(pi.hProcess);
	return status;
}

int shell::wait(shell::pid_t pid)
{
	DWORD code;

	if(WaitForSingleObject(pid, INFINITE) == WAIT_FAILED) {
		return -1;
	}

	GetExitCodeProcess(pid, &code);
	CloseHandle(pid);
	return (int)code;
}

shell::pid_t spawn(const char *path, char **argv, char **env)
{
	bool pmode = false;

	if(strchr(path, '/') || strchr(path, '\\') || strchr(path, ':'))
		pmode = true;

	if(pmode && env)
		return (shell::pid_t)_spawnve(P_NOWAIT, path, argv, env);
	else if(pmode)
		return (shell::pid_t)_spawnv(P_NOWAIT, path, argv);
	else if(env)
		return (shell::pid_t)_spawnvpe(P_NOWAIT, path, argv, env);
	else
		return (shell::pid_t)_spawnvp(P_NOWAIT, path, argv);
}

int detach(const char *path, char **argv, char **env)
{
	bool pmode = false;
	int code;

	if(strchr(path, '/') || strchr(path, '\\') || strchr(path, ':'))
		pmode = true;

	if(pmode && env)
		code = _spawnve(P_DETACH, path, argv, env);
	else if(pmode)
		code = _spawnv(P_DETACH, path, argv);
	else if(env)
		code = _spawnvpe(P_DETACH, path, argv, env);
	else
		code = _spawnvp(P_DETACH, path, argv);

	if(code > 0)
		code = 0;
	
	return code;
}

#else

int shell::system(const char *cmd, const char **envp)
{
	assert(cmd != NULL);
	
	char symname[129];
	const char *cp;
	char *ep;
	int status;
	int max = sizeof(fd_set) * 8;

#ifdef	RLIMIT_NOFILE
	struct rlimit rlim;

	if(!getrlimit(RLIMIT_NOFILE, &rlim))
		max = rlim.rlim_max;
#endif

	pid_t pid = fork();
	if(pid < 0)
		return -1;

	if(pid > 0) {
		if(::waitpid(pid, &status, 0) != pid)
			status = -1;
		return status;
	}

	for(int fd = 3; fd < max; ++fd)
		::close(fd);

	while(envp && *envp) {
		String::set(symname, sizeof(symname), *envp);
		ep = strchr(symname, '=');
		if(ep)
			*ep = 0;
		cp = strchr(*envp, '=');
		if(cp)
			++cp;
		::setenv(symname, cp, 1);
		++envp;
	}

	::signal(SIGQUIT, SIG_DFL);
	::signal(SIGINT, SIG_DFL);
	::signal(SIGCHLD, SIG_DFL);
	::signal(SIGPIPE, SIG_DFL);
	::execlp("/bin/sh", "sh", "-c", cmd, NULL);
	::exit(-1);
}

int shell::detach(const char *path, char **argv, char **envp, fd_t *stdio)
{
	char symname[129];
	const char *cp;
	char *ep;	
	fd_t fd;

	int max = sizeof(fd_set) * 8;
#ifdef	RLIMIT_NOFILE
	struct rlimit rlim;

	if(!getrlimit(RLIMIT_NOFILE, &rlim))
		max = rlim.rlim_max;
#endif

	pid_t pid = fork();
	if(pid < 0)
		return INVALID_PID_VALUE;

	if(pid > 0)
		return pid;

	::signal(SIGQUIT, SIG_DFL);
	::signal(SIGINT, SIG_DFL);
	::signal(SIGCHLD, SIG_DFL);
	::signal(SIGPIPE, SIG_DFL);
	::signal(SIGHUP, SIG_IGN);

#ifdef	SIGTTOU
	::signal(SIGTTOU, SIG_IGN);
#endif

#ifdef	SIGTTIN
	::signal(SIGTTIN, SIG_IGN);
#endif

#ifdef	SIGTSTP
	::signal(SIGTSTP, SIG_IGN);
#endif

	for(fd = 0; fd < 3; ++fd) {
		if(stdio && stdio[fd] != INVALID_HANDLE_VALUE)
			::dup2(stdio[fd], fd);
		else
			::close(fd);
	}

	for(fd = 3; fd < max; ++fd)
		::close(fd);

#if defined(SIGTSTP) && defined(TIOCNOTTY)
	if(setpgid(0, getpid()) == -1)
		::exit(-1);
	
	if((fd = open("/dev/tty", O_RDWR)) >= 0) {
		::ioctl(fd, TIOCNOTTY, NULL);
		::close(fd);
	}
#else
	
#ifdef	HAVE_SETPGRP
	if(setpgrp() == -1)
		::exit(-1);
#else
	if(setpgid(0, getpid()) == -1)		
		::exit(-1);
#endif

	if(getppid() != 1) {
		if((pid = fork()) < 0)
			::exit(-1);
		else if(pid > 0)
			::exit(0);
	}
#endif

	::open("/dev/null", O_RDWR);
	::open("/dev/null", O_RDWR);
	::open("/dev/null", O_RDWR);

	while(envp && *envp) {
		String::set(symname, sizeof(symname), *envp);
		ep = strchr(symname, '=');
		if(ep)
			*ep = 0;
		cp = strchr(*envp, '=');
		if(cp)
			++cp;
		::setenv(symname, cp, 1);
		++envp;
	}

	if(strchr(path, '/'))
		execv(path, argv);
	else
		execvp(path, argv);
	exit(-1);
}


shell::pid_t shell::spawn(const char *path, char **argv, char **envp, fd_t *stdio)
{
	char symname[129];
	const char *cp;
	char *ep;
	int fd;

	int max = sizeof(fd_set) * 8;
#ifdef	RLIMIT_NOFILE
	struct rlimit rlim;

	if(!getrlimit(RLIMIT_NOFILE, &rlim))
		max = rlim.rlim_max;
#endif

	pid_t pid = fork();
	if(pid < 0)
		return INVALID_PID_VALUE;

	if(pid > 0)
		return pid;

	::signal(SIGQUIT, SIG_DFL);
	::signal(SIGINT, SIG_DFL);
	::signal(SIGCHLD, SIG_DFL);
	::signal(SIGPIPE, SIG_DFL);

	for(fd = 0; fd < 3; ++fd) {
		if(stdio && stdio[fd] != INVALID_HANDLE_VALUE)
			::dup2(stdio[fd], fd);
	}

	for(fd = 3; fd < max; ++fd)
		::close(fd);

	while(envp && *envp) {
		String::set(symname, sizeof(symname), *envp);
		ep = strchr(symname, '=');
		if(ep)
			*ep = 0;
		cp = strchr(*envp, '=');
		if(cp)
			++cp;
		::setenv(symname, cp, 1);
		++envp;
	}

	if(strchr(path, '/'))
		execv(path, argv);
	else
		execvp(path, argv);
	exit(-1);
}

int shell::wait(shell::pid_t pid)
{
	int status = -1;

	if(pid == INVALID_PID_VALUE || ::waitpid(pid, &status, 0) != pid)
		return -1;

	if(status == -1)
		return -1;

	return WEXITSTATUS(status);
}

int shell::cancel(shell::pid_t pid)
{
	if(kill(pid, SIGTERM))
		return -1;
	return wait(pid);
}

#endif
