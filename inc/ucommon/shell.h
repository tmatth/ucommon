// Copyright (C) 1999-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

/**
 * Generic shell parsing and application services.
 * @file ucommon/shell.h
 */

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef	_UCOMMON_SHELL_H_
#define	_UCOMMON_SHELL_H_

NAMESPACE_UCOMMON

/**
 * A utility class for generic shell operations.  This includes utilities
 * to parse and expand arguments, and to call system shell services.  This
 * also includes a common shell class to parse and process command line
 * arguments which are managed through a local heap.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT shell : public mempager
{
protected:
	char **_argv;
	int _argc;

	class args
	{
	public:
		args *next;
		char *item;
	} *first, *last;

	/**
	 * Shell expansion for some platforms.
	 */
	void expand(void);
	
	/**
	 * Collapse argument list.
	 */
	void collapse(void);

public:
	/**
	 * Construct a shell argument list by parsing a simple command string.
	 * This seperates a string into a list of command line arguments which
	 * can be used with exec functions.
	 * @param string to parse.
	 * @param pagesize for local heap.
	 */
	shell(const char *string, size_t pagesize = 0);

	/**
	 * Construct a shell argument list from existing arguments.  This
	 * copies and on some platforms expands the argument list originally 
	 * passed to main.
	 * @param argv from main.
	 * @param pagesize for local heap.
	 */
	shell(char **argv, size_t pagesize = 0);

	/**
	 * Construct an empty shell parser argument list.
	 * @param pagesize for local heap.
	 */
	shell(size_t pagesize = 0);

	/**
	 * A shell system call.  This uses the native system shell to invoke the 
	 * command.
	 * @param command string..
	 * @param env array to optionally use.
	 * @return error code of child process.
	 */
	static int system(const char *command, const char **env = NULL);

	/**
	 * A shell system call that can be issued using a formatted string.  This
	 * uses the native system shell to invoke the command.
	 * @param format of/command string.
	 * @return error code of child process.
	 */
	static int systemf(const char *format, ...) __PRINTF(1,2);

	/**
	 * Parse a string as a series of arguments for use in exec calls.
	 * @param string to parse.
	 * @return argument array.
	 */
	char **parse(const char *string);

	/**
	 * Parse/copy an argv list, expand as needed for wildcards on some
	 * platforms.  The original argv is replaced with an expanded one.
	 * @param argc from main.
	 * @param argv from main.
	 * @return new argc.  argv also overridden.
	 */
	int expand(int *argc, char ***argv);

	/**
	 * Fetch arguments list.
	 * @return argument array.
	 */
	inline char **get(void)
		{return _argv;};

	/**
	 * Parse shell arguments directly into a shell object.
	 * @param args table.
	 * @param string to parse.
	 * @return argument array.
	 */
	inline static char **parse(shell &args, const char *string)
		{return args.parse(string);};

	/**
	 * Parse shell arguments from main directly into a shell object.  This
	 * expands arguments as needed.
	 * @param args table.
	 * @param argc from main.
	 * @param argv from main, replaced.
	 * @return argument count.
	 */
	inline static int expand(shell &args, int *argc, char ***argv)
		{return args.expand(argc, argv);};
};
		
END_NAMESPACE

#endif
