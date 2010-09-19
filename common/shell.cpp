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
#include <winreg.h>
#else
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef	HAVE_SETLOCALE
#include <locale.h>
#else
#define	setlocale(s, t)
#endif

#ifndef	HAVE_LIBINTL_H
#undef	HAVE_GETTEXT
#endif

#ifdef	HAVE_GETTEXT
#include <libintl.h>
#else
#define dgettext(d, s) s
#define	gettext(s)	s
#define	bindtextdomain(s, t)
#define	textdomain(s)
#endif

#ifdef	HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifndef OPEN_MAX
#define OPEN_MAX 20
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) ((unsigned)(status) >> 8)
#endif

#ifndef _PATH_TTY
#define _PATH_TTY   "/dev/tty"
#endif

using namespace UCOMMON_NAMESPACE;

static shell::loglevel_t errlevel = shell::WARN;
static shell::logmode_t errmode = shell::NONE;
static const char *errname = NULL;
static shell::logproc_t errproc = (shell::logproc_t)NULL;
static mutex_t symlock;
static shell::downproc_t downproc = (shell::downproc_t)NULL;
static char **_orig = NULL;

OrderedIndex shell::Option::index;
const char *shell::domain = NULL;

shell::pipeio::pipeio()
{
	pid = INVALID_PID_VALUE;
	input = output = INVALID_HANDLE_VALUE;
	perror = presult = 0;
}

#ifdef	_MSWINDOWS_

int shell::pipeio::spawn(const char *path, char **argv, pmode_t mode, size_t size, char **envp)
{
	SECURITY_ATTRIBUTES sa;
	fd_t stdio[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if(mode == RD || mode == RDWR)
		CreatePipe(&input, &stdio[1], &sa, size);

	if(mode == WR || mode == RDWR)
		CreatePipe(&stdio[0], &output, &sa, size);
		
	pid = shell::spawn(path, argv, envp, stdio);
	perror = 0;
	if(pid == INVALID_PID_VALUE) {
		perror = fsys::remapError();
		if(mode == RD || mode == RDWR) {
			CloseHandle(input);
			input = INVALID_HANDLE_VALUE;
		}
		if(mode == WR || mode == RDWR) {
			CloseHandle(output);
			output = INVALID_HANDLE_VALUE;
		}
	}

	if(mode == RD || mode == RDWR)
		CloseHandle(stdio[1]);
	if(mode == WR || mode == RDWR)
		CloseHandle(stdio[0]);

	return perror;
}


int shell::pipeio::cancel(void)
{
	if(pid != INVALID_PID_VALUE)
		TerminateProcess(pid, 7);
	
	return wait();
}

int shell::pipeio::wait(void)
{
	DWORD code;

	presult = -1;

	// if stdio, we do not want to close stdin/stdout handles 
	// instead we just mark the object to disable buffering
	if(pid == INVALID_PID_VALUE) {
		perror = EPIPE;
		return presult;
	}

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
	fd_t stdio[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

	pin[0] = INVALID_HANDLE_VALUE;
	pout[1] = INVALID_HANDLE_VALUE;
	stdio[2] = 2;

	if(mode == RD || mode == RDWR) {
		if(::pipe(pin)) {
			perror = errno;
			return perror;
		}
		stdio[1] = pin[1];
	}

	if(mode == WR || mode == RDWR) {
		if(::pipe(pout)) {
			perror = errno;
			if(mode == RDWR) {
				::close(pin[0]);
				::close(pin[1]);
			}
			return perror;
		}	
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

	// we do not want to close our stdin/stdout handles
	if(pid == INVALID_PID_VALUE) {
		perror = EPIPE;
		return -1;
	}

	if(input != INVALID_HANDLE_VALUE)
		::close(input);

	if(output != INVALID_HANDLE_VALUE)
		::close(output);

	input = output = INVALID_HANDLE_VALUE;

	if(::waitpid(pid, &presult, 0) != pid)
		presult = -1;

	pid = INVALID_PID_VALUE;

	if(presult == -1)
		return -1;

	presult = WEXITSTATUS(presult);
	return presult;
}

int shell::pipeio::cancel(void)
{
	if(pid != INVALID_PID_VALUE)
		kill(pid, SIGTERM);

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

shell::iobuf::iobuf(const char *path, char **argv, shell::pmode_t mode, size_t size, char **env) : 
IOBuffer(), shell::pipeio()
{
	ioerror = 0;
	open(path, argv, mode, size, env);
}

shell::iobuf::iobuf(size_t size) :
IOBuffer(), shell::pipeio()
{
	ioerror = 0;

	if(size) {
		pipeio::input = shell::input();
		pipeio::output = shell::output();
		allocate(size, BUF_RDWR);
	}
}

shell::iobuf::~iobuf()
{
	cancel();
}

void shell::iobuf::_clear(void)
{
	ioerror = 0;
}

int shell::iobuf::_err(void)
{
	return ioerror;
}

size_t shell::iobuf::_pull(char *buf, size_t size)
{
	size_t result = pipeio::read(buf, size);
	if(perror)
		ioerror = perror;
	return result;
}

size_t shell::iobuf::_push(const char *buf, size_t size)
{
	size_t result = pipeio::write(buf, size);
	if(perror)
		ioerror = perror;
	return result;
}

void shell::iobuf::close(void)
{
	pipeio::wait();
	IOBuffer::release();
}

void shell::iobuf::cancel(void)
{
	pipeio::cancel();
	IOBuffer::release();
}

void shell::iobuf::open(const char *path, char **argv, shell::pmode_t mode, size_t size, char **env)
{
	if(!pipeio::spawn(path, argv, mode, size, env))
		IOBuffer::allocate(size, (type_t)mode);
}
	
shell::Option::Option(char shortopt, const char *longopt, const char *value, const char *help) :
OrderedObject(&index)
{
	while(longopt && *longopt == '-')
		++longopt;

	short_option = shortopt;
	long_option = longopt;
	uses_option = value;
	help_string = help;
	trigger_option = false;
}

shell::Option::~Option()
{
}

void shell::Option::disable(void)
{
	short_option = 0;
	long_option = NULL;
	help_string = NULL;
	uses_option = NULL;
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

	used = true;
	number = strtol(value, &endptr, 0);
	if(!endptr || *endptr != 0)
		return errmsg(shell::BAD_VALUE);

	return NULL;
}

shell::counteropt::counteropt(char short_option, const char *long_option, const char *help_string, const char *type, long def_value) :
shell::Option(short_option, long_option, type, help_string)
{
	used = false;
	number = def_value;
	trigger_option = true;
}

const char *shell::counteropt::assign(const char *value)
{
	char *endptr = NULL;

	// trigger option mode received...
	if(value == NULL) {
		++number;
		used = true;
		return NULL;
	}

	if(used)
		return errmsg(shell::OPTION_USED);

	used = true;
	number = strtol(value, &endptr, 0);
	if(!endptr || *endptr != 0)
		return errmsg(shell::BAD_VALUE);

	return NULL;
}

shell::groupopt::groupopt(const char *help_string) :
shell::Option(0, NULL, NULL, help_string)
{
}

const char *shell::groupopt::assign(const char *value)
{
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
	used = true;
	return NULL;
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

	used = true;
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
	char **argv = _argv = (char **)mempager::_alloc(sizeof(char **) * (_argc + 1));
	linked_pointer<args> ap = first;
	while(is(ap)) {
		*(argv++) = ap->item;
		ap.next();
	}
	*argv = NULL;
}

void shell::set0(char *argv0)
{
	if(_argv0)
		return;

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

	if(eq(_argv0, "lt-", 3))
		_argv0 += 3;

	_argv0 = dup(_argv0);

#ifdef	_MSWINDOWS_
	char *ext = strrchr(_argv0, '.');
	if(ieq(ext, ".exe") || ieq(ext, ".com"))
		*ext = 0;
#endif

	if(!domain)
		bind(_argv0);
}

shell::shell(size_t pagesize) :
mempager(pagesize)
{
	_argv0 = NULL;
	_argv = NULL;
	_argc = 0;
	_syms = NULL;
}

shell::shell(const char *string, size_t pagesize) :
mempager(pagesize)
{
	_argv0 = NULL;
	_argv = NULL;
	_argc = 0;
	_syms = NULL;

	parse(string);
}

shell::shell(int argc, char **argv, size_t pagesize) :
mempager(pagesize)
{
	_argv0 = NULL;
	_argv = NULL;
	_argc = 0;
	_syms = NULL;

	parse(argc, argv);
}

static const char *msgs[] = {
	_TEXT("missing command line arguments"),
	_TEXT("missing argument for option"),
	_TEXT("option does not have argument"),
	_TEXT("unknown command option"),
	_TEXT("option already used"),
	_TEXT("invalid argument used"),
	NULL};

const char *shell::errmsg(errmsg_t id)
{
	return dgettext("ucommon", msgs[id]);
}

void shell::errmsg(errmsg_t id, const char *text)
{
	msgs[id] = shell::text(text);
}

unsigned shell::count(char **argv)
{
	unsigned count = 0;

	while(argv && argv[count])
		++count;

	return count;
}

void shell::help(void)
{
	linked_pointer<Option> op = Option::first();
	unsigned hp = 0, count = 0;
	while(is(op)) {
		if(!op->help_string) {
			++op;
			continue;
		}
		if(op->short_option && op->long_option && op->uses_option && !op->trigger_option) {
			printf("  -%c .., ", op->short_option);
			hp = 9;
		}
		else if(op->short_option && op->long_option) {
			printf("  -%c, ", op->short_option);
			hp = 6;
		}
		else if(op->long_option) {
			printf("  ");
			hp = 2;
		}
		else if(op->uses_option) {
			printf("  -%c %s", op->short_option, op->uses_option);
			hp = 5 + strlen(op->uses_option);
		}
		else if(op->short_option) {
			printf("  -%c ", op->short_option);
			hp = 5;
		}
		else {		// grouping separator
			if(count)
				printf("\n%s:\n", op->help_string);
			else
				printf("%s:\n", op->help_string);
			++op;
			continue;
		}

		++count;
	
		if(op->long_option && op->uses_option) {
			printf("--%s=%s", op->long_option, op->uses_option);
			hp += strlen(op->long_option) + strlen(op->uses_option) + 3;
		}
		else if(op->long_option) {
			printf("--%s", op->long_option);
			hp += strlen(op->long_option) + 2;
		}
		if(hp > 29) {
			printf("\n");
			hp = 0;
		}
		while(hp < 30) {
			putchar(' ');
			++hp;
		} 
		const char *hs = shell::text(op->help_string);
		while(*hs) {
			if(*hs == '\n' || (((*hs == ' ' || *hs == '\t')) && (hp > 75))) {
				printf("\n                              ");
				hp = 30;
			}
			else if(*hs == '\t') {
				if(!(hp % 8)) {
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
		++op;
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
			arg = init<args>((args *)mempager::_alloc(sizeof(args)));
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
	if(!_orig)
		_orig = argv;

	getargv0(argv);
	getargv(++argv);
}

char *shell::getargv0(char **argv)
{
	if(!argv || !argv[0])
		errexit(-1, "*** %s\n", errmsg(shell::NOARGS));

	set0(argv[0]);
	return _argv0;
}

char **shell::getargv(char **argv)
{
	char *arg, *opt;
	unsigned len;
	const char *value;
	const char *err;
	unsigned argp = 0;

	while(argv[argp]) {
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
				errexit(1, "*** %s: --%s: %s\n", _argv0, op->long_option, shell::text(err));
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

			value = NULL;
			if(op->trigger_option) 
				goto trigger;

			if(op->uses_option && arg[1] == 0)
				value = argv[argp++];
			else if(op->uses_option)
				value = ++arg;

			if(op->uses_option && value == NULL)
				errexit(1, "*** %s: -%c: %s\n", _argv0, op->short_option, errmsg(shell::NOARGUMENT));
trigger:
			err = op->assign(value);
			if(err)
				errexit(1, "*** %s: -%c: %s\n", _argv0, op->short_option, shell::text(err));
			if(value)
				break;
		}
	}
	_argv = &argv[argp];

	_argc = 0;
	argv = _argv;
	while(argv[_argc])
		++_argc;

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
			argitem = init<args>((args *)mempager::_alloc(sizeof(args)));
			argitem->item = mempager::dup(dirname);
			argitem->enlist(&arglist);
			++_argc;
		} while(FindNextFile(dir, &entry));
		CloseHandle(dir);
		++*_argv;
		continue;
skip:
		argitem = init<args>((args *)mempager::_alloc(sizeof(args)));
		argitem->item = *(_argv++);
		argitem->enlist(&arglist);
		++_argc;
	}
	collapse(arglist.begin());
#endif
	return _argv;
}

void shell::errexit(int exitcode, const char *format, ...)
{
	va_list args;
	loglevel_t level = ERR;
	char buf[256];

	String::set(buf, sizeof(buf) - 1, format);
	size_t len = strlen(buf);

	if(buf[len - 1] != '\n') {
		buf[len] = '\n';
		buf[len + 1] = 0;
	}
	else
		--len;

	format = buf;

	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);

	buf[len] = 0;

	if(exitcode)
		level = FAIL;

#ifdef	HAVE_SYSLOG_H
	if(errname && errmode != NONE && level <= errlevel) {
		if(eq("*** ", format, 4)) {
			format += 4;			
			const char *cp = format;
			while(isalnum(*cp) || *cp == '-' || *cp == '.')
				++cp;
			if(*cp == ':' && cp[1] == ' ')
				format = cp + 2;
		}
			
		if(exitcode)
			vsyslog(LOG_CRIT, format, args);
		else
			vsyslog(LOG_ERR, format, args);
	}
#endif

	va_end(args);
	
	if(exitcode)
		::exit(exitcode);
}

size_t shell::readln(pipe_t pipe, char *buffer, size_t size)
{
	unsigned pos = 0;

	while(pos < size - 1) {
		if(pipe->read(buffer, 1) < 1)
			break;
		if(buffer[pos] == '\n')
			break;
		++pos;
	}

	if(pos && buffer[pos - 1] == '\r')
		--pos;

	buffer[pos] = 0;
	return pos;
}

size_t shell::printf(pipe_t pipe, const char *format, ...)
{
	char buf[512];

	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	if(buf[0])
		return pipe->write(buf, strlen(buf));
	return 0;
}

size_t shell::printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	size_t result = vprintf(format, args);
	va_end(args);
	if(result == (size_t)EOF)
		result = 0;
	return result;
}

size_t shell::readln(char *address, size_t size)
{
	address[0] = 0;

	if(!fgets(address, size, stdin))
		return 0;

	if(address[size - 1] == '\n') {
		--size;
		if(address[size - 1] == '\r')
			--size;
	}
	address[size] = 0;
	return size;
}

size_t shell::read(String& string)
{
	char *cp = string.c_mem();
	size_t size = string.size();
	size = readln(cp, size);
	String::fix(string);
	return size;
}

size_t shell::writes(pipe_t pipe, const char *string)
{
	size_t size = 0;

	if(string)
		size = strlen(string);

	if(size)
		return pipe->write(string, size);
	
	return 0;
}

size_t shell::writes(const char *string)
{
	size_t result = fputs(string, stdout);
	if(result == ((size_t)(EOF)))
		return 0;
	return result;
}

size_t shell::read(pipe_t pipe, String& str)
{
	char *cp = str.c_mem();
	size_t size = str.size();

	if(!size)
		return 0;

	size = readln(pipe, cp, size);
	cp[size] = 0;
	String::fix(str);
	return size;
}

int shell::cancel(shell::pipe_t pio)
{
	int status = pio->cancel();
	delete pio;
	return status;
}

int shell::wait(shell::pipe_t pio)
{
	int status = pio->wait();
	delete pio;
	return status;
}

shell::pipe_t shell::spawn(const char *path, char **argv, pmode_t mode, size_t size, char **env)
{
	shell::pipe_t pio = new shell::pipeio;
	if(pio->spawn(path, argv, mode, size, env)) {
		delete pio;
		return NULL;
	}
	return pio;
}

#ifdef _MSWINDOWS_

static void pathfinder(const char *name, char *buf, size_t size)
{
	char path[512];
	char *tokbuf = NULL;
	char *element;
	char *ext;

	String::set(buf, size, name);

	if(!GetEnvironmentVariable("PATH", path, sizeof(path)))
		goto tail;

	if(strchr(name, '/') || strchr(name, '\\') || strchr(name, ':'))
		goto tail;

	while(NULL != (element = String::token(path, &tokbuf, ";"))) {
		snprintf(buf, sizeof(buf), "%s\\%s", element, name);
		ext = strrchr(buf, '.');
		if(!ext || (!ieq(ext, ".exe") && !ieq(ext, ".com")))
			String::add(buf, size, ".exe");
		if(fsys::isfile(buf))
			return;
	}

	String::set(buf, size, name);

tail:
	ext = strrchr(buf, '.');
	if(!ext || (!ieq(ext, ".exe") && !ieq(ext, ".com")))
		String::add(buf, size, ".exe");
}	

const char *shell::getenv(const char *id, const char *value)
{
	char buf[512];
	char path[255];
	const char *keyid = NULL;

	if(GetEnvironmentVariable(id, buf, sizeof(buf)))
		return dup(buf);

	if(errname)
		keyid = errname;
	else if(domain)
		keyid = domain;
	else if(_argv0)
		keyid = _argv0;
	
	if(keyid) {
		snprintf(path, sizeof(path), "Default Environment\\%s", keyid);
		HKEY key;
		if(RegOpenKey(HKEY_CLASSES_ROOT, path, &key) == ERROR_SUCCESS) {
			LONG dlen = sizeof(buf);
			buf[0] = 0;
			RegQueryValueA(key, id, (LPTSTR)buf, &dlen);
			RegCloseKey(key);
			if(buf[0])
				return dup(buf);
		}	
	}
	
	return value;		
}
	
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

shell::pid_t shell::spawn(const char *path, char **argv, char **envp, fd_t *stdio)
{
	STARTUPINFO	si;
	PROCESS_INFORMATION pi;
	char filename[128];
	int pos;
	pid_t pid = INVALID_PID_VALUE;
	fd_t stdfd;
	fd_t dups[3] = 
		{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

	char *ep = NULL;
	unsigned len = 0;

	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if(envp)
		ep = new char[4096];

	while(envp && *envp && len < 4090) {
		String::set(ep + len, 4094 - len, *envp);
		len += strlen(*(envp++)) + 1;
	}

	if(ep)
		ep[len] = 0;

	pathfinder(path, filename, sizeof(filename));
	char *args = new char[32768];

	args[0] = 0;
	unsigned argc = 0;
	while(argv && argv[argc]) {
		if(!argc)
			String::add(args, 32768, " ");
		String::add(args, 32768, argv[argc++]);
	}

	if(stdio) {
		for(pos = 0; pos < 3; ++pos) {
			stdfd = INVALID_HANDLE_VALUE;
			switch(pos) {
			case 0:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_INPUT_HANDLE);
				break;
			case 1:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_OUTPUT_HANDLE);
				break;
			case 2:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_ERROR_HANDLE);
				break;
			}
			if(stdfd != INVALID_HANDLE_VALUE) {
				DuplicateHandle(GetCurrentProcess(), stdfd,
					GetCurrentProcess(), &dups[pos], 0,
					TRUE, DUPLICATE_SAME_ACCESS);
				stdfd = dups[pos];
			}
			else
				stdfd = stdio[pos];
			switch(pos) {
			case 0:
				si.hStdInput = stdfd;
				break;
			case 1:
				si.hStdOutput = stdfd;
				break;
			case 2:
				si.hStdError = stdfd;
				break;
			}
		}
		si.dwFlags = STARTF_USESTDHANDLES;
	}					

	if(!CreateProcess((CHAR *)filename, (CHAR *)args, NULL, NULL, TRUE, 0, ep, NULL, &si, &pi)) 
		goto exit;

	pid = pi.hProcess;
	CloseHandle(pi.hThread);

exit:
	if(ep)
		delete ep;
	delete args;
	for(pos = 0; pos < 3; ++pos) {
		if(dups[pos] != INVALID_HANDLE_VALUE)
			CloseHandle(dups[pos]);
	}
	return pid;
}

static SERVICE_STATUS_HANDLE hStatus;
static SERVICE_STATUS status;
static unsigned detaches = 0;

static void WINAPI control(DWORD control)
{
	switch(control) {
	case 128:
		if(downproc)
			(*downproc)(true);
		return;
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwWin32ExitCode = 0;
        status.dwCheckPoint = 0;
        status.dwWaitHint = 6000;
		SetServiceStatus(hStatus, &status);
		if(downproc)
			(*downproc)(false);
		else
			exit(0);
		break;
	default:
		break;
	}
}

extern "C" {
	extern int main(int argc, char **argv);
}

static void WINAPI _start(DWORD argc, LPSTR *argv)
{
	++detaches;
	::main(argc, argv);
}

void shell::detach(downproc_t stop)
{
	const char *name = _argv0;

	downproc = stop;

	if(domain)
		name = domain;

	name = strdup(name);

	if(detaches) {
		memset(&status, 0, sizeof(SERVICE_STATUS));
		status.dwServiceType = SERVICE_WIN32;
		status.dwCurrentState = SERVICE_START_PENDING;
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;

		hStatus = ::RegisterServiceCtrlHandler(name, &control);
		return;
	}

	SERVICE_TABLE_ENTRY servicetable[] = {
		{(LPSTR)name, &_start},
		{NULL, NULL}
	};

	if(!StartServiceCtrlDispatcher(servicetable))
		errexit(1, "*** %s: %s\n", name, _TEXT("failed service start"));
}

void shell::restart(void)
{
}

void shell::up(void)
{
	if(detaches) {
		status.dwCurrentState = SERVICE_RUNNING;
		::SetServiceStatus(hStatus, &status);
	}
}
	
int shell::detach(const char *path, char **argv, char **envp, fd_t *stdio)
{
	STARTUPINFO	si;
	PROCESS_INFORMATION pi;
	char filename[128];
	int err;
	int pos;
	pid_t pid = INVALID_PID_VALUE;
	fd_t stdfd;
	fd_t dups[3] = 
		{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

	char *ep = NULL;
	unsigned len = 0;

	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if(envp)
		ep = new char[4096];

	while(envp && *envp && len < 4090) {
		String::set(ep + len, 4094 - len, *envp);
		len += strlen(*(envp++)) + 1;
	}

	if(ep)
		ep[len] = 0;

	pathfinder(path, filename, sizeof(filename));
	char *args = new char[32768];

	args[0] = 0;
	unsigned argc = 0;
	while(argv && argv[argc]) {
		if(!argc)
			String::add(args, 32768, " ");
		String::add(args, 32768, argv[argc++]);
	}

	if(stdio) {
		for(pos = 0; pos < 3; ++pos) {
			stdfd = INVALID_HANDLE_VALUE;
			switch(pos) {
			case 0:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_INPUT_HANDLE);
				break;
			case 1:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_OUTPUT_HANDLE);
				break;
			case 2:
				if(stdio[pos] == INVALID_HANDLE_VALUE)
					stdfd = GetStdHandle(STD_ERROR_HANDLE);
				break;
			}
			if(stdfd != INVALID_HANDLE_VALUE) {
				DuplicateHandle(GetCurrentProcess(), stdfd,
					GetCurrentProcess(), &dups[pos], 0,
					TRUE, DUPLICATE_SAME_ACCESS);
				stdfd = dups[pos];
			}
			else
				stdfd = stdio[pos];
			switch(pos) {
			case 0:
				si.hStdInput = stdfd;
				break;
			case 1:
				si.hStdOutput = stdfd;
				break;
			case 2:
				si.hStdError = stdfd;
				break;
			}
		}
		si.dwFlags = STARTF_USESTDHANDLES;
	}					

	if(!CreateProcess((CHAR *)filename, (CHAR *)args, NULL, NULL, TRUE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, ep, NULL, &si, &pi)) {
		err = fsys::remapError();
		goto exit;
	}

	pid = pi.hProcess;
	CloseHandle(pi.hThread);

exit:
	if(ep)
		delete ep;
	delete args;
	for(pos = 0; pos < 3; ++pos) {
		if(dups[pos] != INVALID_HANDLE_VALUE)
			CloseHandle(dups[pos]);
	}
	if(pid == INVALID_PID_VALUE)
		return err;
	
	return 0;
}

#else

const char *shell::getenv(const char *id, const char *value)
{
	const char *v = ::getenv(id);
	if(v)
		return dup(v);

	return value;
}

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

void shell::up(void)
{
}

void shell::restart(void)
{
	pid_t pid;
	int status;

restart:
	pid = fork();
	if(pid > 0) {
		waitpid(pid, &status, 0);
		if(WIFSIGNALED(status))
			status = WTERMSIG(status);
		else
			status = WIFEXITED(status);
		switch(status) {
#ifdef	SIGPWR
		case SIGPWR:
#endif
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
		case 0:
			exit(status);
		default:
			goto restart;
		}
	}
}

void shell::detach(downproc_t stop)
{
	const char *dev = "/dev/null";
	pid_t pid;
	int fd;

	downproc = stop;

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
	crit(pid == 0, "detach without process");

#if defined(SIGTSTP) && defined(TIOCNOTTY)
	crit(setpgid(0, getpid()) == 0, "detach without process group");
	if((fd = open(_PATH_TTY, O_RDWR)) >= 0) {
		ioctl(fd, TIOCNOTTY, NULL);
		close(fd);
	}
#else

#ifdef HAVE_SETPGRP
	crit(setpgrp() == 0, "detach without process group");
#else
	crit(setpgid(0, getpid()) == 0, "detach without process group");
#endif
	signal(SIGHUP, SIG_IGN);
	pid = fork();
	if(pid > 0)
		exit(0);
	crit(pid == 0, "detach without process");
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
		return errno;

	if(pid > 0)
		return 0;

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

const char *shell::texts(const char *singular, const char *plural, unsigned long value)
{
#ifdef	HAVE_GETTEXT
	return ::ngettext(singular, plural, value);
#else
	if(value > 1)
		return plural;

	return singular;
#endif
}

const char *shell::text(const char *msg)
{
#ifdef	HAVE_GETTEXT
	return ::gettext(msg);
#else
	return msg;
#endif
}

void shell::bind(const char *name)
{
#ifdef	_MSWINDOWS_
	const char *locale = "C:\\Program Files\\Common Files\\locale";

	if(fsys::isdir("locale")) {
		char prefix[256];
		fsys::getPrefix(prefix, sizeof(prefix));
		String::add(prefix, sizeof(prefix), "/locale");	
		locale = strdup(prefix);
	}

#else
	const char *locale = UCOMMON_LOCALE;
#endif

	if(!domain) {
		setlocale(LC_ALL, "");
		bindtextdomain("ucommon", locale);
	}

	bindtextdomain(name, locale);
	textdomain(name);
	domain = name;
}

void shell::rebind(const char *name)
{
	if(name)
		textdomain(name);
	else
		textdomain(domain);
}

#ifdef	_MSWINDOWS_
void shell::priority(int level)
{
}
#else
void shell::priority(int level)
{
#if _POSIX_PRIORITY_SCHEDULING > 0
    int policy = SCHED_OTHER;

    if(level > 0)
        policy = SCHED_RR;

    struct sched_param sparam;
    int min = sched_get_priority_min(policy);
    int max = sched_get_priority_max(policy);
    int pri = (int)level;

    if(min == max)
        pri = min;
    else
        pri += min;
    if(pri > max)
        pri = max;

    setpriority(PRIO_PROCESS, 0, -level);
    memset(&sparam, 0, sizeof(sparam));
    sparam.sched_priority = pri;
    sched_setscheduler(0, policy, &sparam);
#else
    nice(-level);
#endif
}
#endif

void shell::debug(unsigned level, const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    char buf[256];
    va_list args;

	level += (unsigned)DEBUG0;

	if(!errname || level > (unsigned)errlevel)
		return;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

	if(fmt[strlen(fmt) - 1] == '\n')
        fprintf(stderr, "%s: %s", errname, buf);
    else
        fprintf(stderr, "%s: %s\n", errname, buf);
}

#ifdef	HAVE_SYSLOG_H

void shell::log(const char *name, loglevel_t level, logmode_t mode, logproc_t handler)
{
	errlevel = level;
	errmode = mode;
	errname = name;

	if(handler != (logproc_t)NULL)
		errproc = handler;

	switch(mode) {
	case NONE:
		closelog();
		return;
	case CONSOLE_LOG:
		::openlog(name, LOG_CONS, LOG_DAEMON);
		return;
	case USER_LOG:
		::openlog(name, 0, LOG_USER);
		return;
	case SYSTEM_LOG:
		::openlog(name, LOG_CONS, LOG_DAEMON);
		return;
	}
}

void shell::log(loglevel_t loglevel, const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    char buf[256];
    va_list args;
	int level= LOG_ERR;

	if(!errname || errmode == NONE || loglevel > errlevel)
		return;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

	if(errproc != (logproc_t)NULL) {
		if((*errproc)(loglevel, buf))
			return;
	}

	if(loglevel >= DEBUG0) {
        if(getppid() > 1) {
            if(fmt[strlen(fmt) - 1] == '\n')
                fprintf(stderr, "%s: %s", errname, buf);
            else
                fprintf(stderr, "%s: %s\n", errname, buf);
        }
		return;
	}

	switch(loglevel) {
	case INFO:
		level = LOG_INFO;
		break;
	case NOTIFY:
		level = LOG_NOTICE;
		break;
	case WARN:
		level = LOG_WARNING;
		break;
	case ERR:
		level = LOG_ERR;
		break;
	case FAIL:
		level = LOG_CRIT;
		break;
	default:
		level = LOG_ERR;
	}

	if(getppid() > 1) {
		if(fmt[strlen(fmt) - 1] == '\n')
			fprintf(stderr, "%s: %s", errname, buf);
		else
			fprintf(stderr, "%s: %s\n", errname, buf);
	}
	::syslog(level, "%s", buf);

	if(level == LOG_CRIT)
		cpr_runtime_error(buf);
}
		
#else

void shell::log(const char *name, loglevel_t level, logmode_t mode, logproc_t handler)
{
	errlevel = level;
	errmode = mode;
	errname = name;

	if(handler != (logproc_t)NULL)
		errproc = handler;
}

void shell::log(loglevel_t loglevel, const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    char buf[256];
    va_list args;

	if(!errname || errmode == NONE || loglevel > errlevel)
		return;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

	if(errproc != (logproc_t)NULL) {
		if((*errproc)(loglevel, buf))
			return;
	}

	if(loglevel >= DEBUG0) {
		if(fmt[strlen(fmt) - 1] == '\n')
			fprintf(stderr, "%s: %s", errname, buf);
		else
			fprintf(stderr, "%s: %s\n", errname, buf);
		return;
	}

	if(fmt[strlen(fmt) - 1] == '\n')
		fprintf(stderr, "%s: %s", errname, buf);
	else
		fprintf(stderr, "%s: %s\n", errname, buf);

	if(loglevel == FAIL)
		cpr_runtime_error(buf);
}
		
#endif

#ifndef _MSWINDOWS_

void shell::restart(char *argv0, char **argv, char **list)
{
	unsigned args = count(argv);
	unsigned head = count(list);
	unsigned argc = 2 + args + head;
	char **newargs = (char **)mempager::_alloc(sizeof(char **) * argc--);

	memcpy(newargs, list, head * sizeof(char **));
	newargs[head++] = argv0;
	if(args)
		memcpy(&newargs[head], argv, args * sizeof(char **));

	newargs[argc] = NULL;
	execvp(*list, newargs);
	exit(-1);
}

#else

void shell::restart(char *argv0, char **argv, char **list)
{
	exit(-1);
}

#endif

bool shell::issym(const char *name)
{
	symlock.acquire();

	linked_pointer<syms> sp;

	while(is(sp)) {
		if(eq(sp->name, name)) {
			symlock.release();
			return true;
		}
		++sp;
	}
	symlock.release();
	return false;
}

const char *shell::getsym(const char *name, const char *value)
{
	symlock.acquire();
	linked_pointer<syms> sp = _syms;

	while(is(sp)) {
		if(eq(sp->name, name)) {
			value = sp->value;
			symlock.release();
			return value;
		}
		++sp;
	}

	symlock.release();
	return getenv(name, value);
}

void shell::setsym(const char *name, const char *value)
{
	symlock.acquire();
	linked_pointer<syms> sp;

	while(is(sp)) {
		if(eq(sp->name, name)) {
			sp->value = dup(value);
			symlock.release();
			return;
		}
		++sp;
	}

	syms *v = (syms *)mempager::_alloc(sizeof(syms));
	v->name = dup(name);
	v->value = dup(value);
	v->enlist(&_syms);
	symlock.release();
}


