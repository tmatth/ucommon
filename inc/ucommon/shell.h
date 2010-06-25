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
private:
	char **_argv;
	unsigned _argc;
	char *_argv0;

	class __LOCAL args : public OrderedObject
	{
	public:
		char *item;
	};

	/**
	 * Shell expansion for some platforms.
	 */
	void expand(void);
	
	/**
	 * Collapse argument list.
	 */
	void collapse(LinkedObject *first);

	/**
	 * Set argv0
	 */
	void set0(char *argv0);

public:
	/**
	 * A base class used to create parsable shell options.  The virtual
	 * is invoked when the shell option is detected.  Both short and long
	 * forms of argument parsing are supported.  An instance of a derived
	 * class is created to perform the argument parsing.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT Option : public OrderedObject
	{
	private:
		static OrderedIndex index;

	public:
		char short_option;
		const char *long_option;
		const char *uses_option;
		const char *help_string;

		/**
		 * Construct a shell parser option.
		 * @param short_option for single character code.
		 * @param long_option for extended string.
		 * @param value_type if -x value or -long=yyy.
		 * @param help string, future use.
		 */
		Option(char short_option = 0, const char *long_option = NULL, const char *value_type = NULL, const char *help = NULL);

		virtual ~Option();

		inline static LinkedObject *first(void)
			{return index.begin();};

		/**
		 * Used to send option into derived receiver.
		 * @param value option that was received.
		 * @return NULL or error string to use.
		 */
		virtual const char *assign(const char *value) = 0;
	};

	/**
	 * Flag option for shell parsing.  This offers a quick-use class
	 * to parse a shell flag, along with a counter for how many times
	 * the flag was selected.  The counter might be used for -vvvv style
	 * verbose options, for example.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT flag : public Option
	{
	private:
		unsigned counter;
		bool single;

		virtual const char *assign(const char *value);

	public:
		flag(char short_option, const char *long_option = NULL, const char *help = NULL, bool single_use = true);

		inline operator bool()
			{return counter > 0;};

		inline bool operator!()
			{return counter == 0;};

		inline operator unsigned()
			{return counter;};

		inline unsigned operator*()
			{return counter;};
	};

	/**
	 * Text option for shell parsing.  This offers a quick-use class
	 * to parse a shell flag, along with a numberic text that may be
	 * saved and a use counter, as multiple invokations is an error.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT string : public Option
	{
	private:
		bool used;
		const char *text;

		virtual const char *assign(const char *value);

	public:
		string(char short_option, const char *long_option = NULL, const char *help = NULL, const char *type = "text", const char *def_text = NULL);

		inline operator bool()
			{return used;};

		inline bool operator!()
			{return !used;};

		inline operator const char *()
			{return text;};

		inline const char *operator*()
			{return text;};
	};

	/**
	 * Numeric option for shell parsing.  This offers a quick-use class
	 * to parse a shell flag, along with a numberic value that may be
	 * saved and a use counter, as multiple invokations is an error.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT numeric : public Option
	{
	private:
		bool used;
		long number;

		virtual const char *assign(const char *value);

	public:
		numeric(char short_option, const char *long_option = NULL, const char *help = NULL, const char *type = "numeric", long def_value = 0);

		inline operator bool()
			{return used;};

		inline bool operator!()
			{return !used;};

		inline operator long()
			{return number;};

		inline long operator*()
			{return number;};
	};

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
	 * @param argc from main.
	 * @param argv from main.
	 * @param pagesize for local heap.
	 */
	shell(int argc, char **argv, size_t pagesize = 0);

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
	 * Parse the command line arguments using the option table.  File
	 * arguments will be expanded for wildcards on some platforms.
	 * The argv will be set to the first file argument after all options
	 * are parsed.
	 * @param argc from main.
	 * @param argv from main.
	 */
	void parse(int argc, char **argv);

	/**
	 * Parse shell arguments directly into a shell object.
	 * @param args table.
	 * @param string to parse.
	 * @return argument array.
	 */
	inline static char **parse(shell &args, const char *string)
		{return args.parse(string);};

	/**
	 * Get program name (argv0).
	 */
	inline const char *argv0() const
		{return _argv0;};

	/**
	 * Print error message and exit.
	 * @param exitcode to return to parent process.
	 * @param format string to use.
	 */
	static void errexit(int exitcode, const char *format = NULL, ...) __PRINTF(2, 3);

	/**
	 * Print to standard output.
	 * @param format string to use.
	 */
	static void printf(const char *format, ...) __PRINTF(1, 2);

	/**
	 * Get saved internal argc count for items.  This may be items that
	 * remain after shell expansion and options have been parsed.
	 * @return count of remaining arguments.
	 */
	inline unsigned argc(void) const
		{return _argc;};

	/**
	 * Get saved internal argv count for items in this shell object.  This
	 * may be filename items only that remain after shell expansion and
	 * options that have been parsed.
	 * @return list of remaining arguments.
	 */	
	inline char **argv(void) const
		{return _argv;};

	/**
	 * Get argc count for an existing array.
	 * @param argv to count items in.
	 * @return argc count of array.
	 */
	static unsigned count(char **argv);
};
		
END_NAMESPACE

#endif
