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

#ifndef	_MSWINDOWS_
#include <sys/wait.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

using namespace UCOMMON_NAMESPACE;

OrderedIndex shell::Option::index;

shell::Option::Option(char shortopt, const char *longopt, const char *value, const char *help) :
OrderedObject(&index)
{
	while(*longopt == '-')
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
			arg = (args *)mempager::alloc(sizeof(args));
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
			len = strlen(op->long_option);
			value = NULL;
			if(eq(op->long_option, opt, len)) {
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

#ifdef	_MSWINDOWS_
	const char *fn;
	char dirname[128];
	WIN32_FIND_DATA entry;
	args *argitem;
	fd_t dir;
	OrderedIndex arglist;
	_argc = 0;

	while(_argv && *_argv) {
		fn = strrchr(*_argv, '/');
		if(!fn)
			fn = strrchr(*_argv, '\\');
		if(!fn)
			fn = strrchr(*_argv, ':');
		if(fn)
			++fn;
		else
			fn = *_argv;
		if(!*fn)
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
			argitem = (args *)mempager::alloc(sizeof(args));
			argitem->item = mempager::dup(dirname);
			argitem->enlist(&arglist);
			++_argc;
		} while(FindNextFile(dir, &entry));
		CloseHandle(dir);
		++*_argv;
		continue;
skip:
		argitem = (args *)mempager::alloc(sizeof(args));
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

#ifdef _MSWINDOWS_

int shell::system(const char *cmd, const char **envp)
{
	char cmdspec[128];
	DWORD code;
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

	GetEnvironmentVariable("ComSpec", cmdspec, sizeof(cmdspec));
	
	if(!CreateProcess((CHAR *)cmdspec, (CHAR *)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, ep, NULL, NULL, &pi)) {
		if(ep)
			delete[] ep;
		return 127;
	}
	if(ep)
		delete[] ep;
	
	if(WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
		return -1;
	}

	GetExitCodeProcess(pi.hProcess, &code);
	return (int)code;
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
	::exit(127);
}

#endif
