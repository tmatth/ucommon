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

/**
 * Example of shell parsing.
 * @example shell.cpp
 */

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef	_UCOMMON_BUFFER_H_
#include <ucommon/buffer.h>
#endif

#ifndef	_UCOMMON_SHELL_H_
#define	_UCOMMON_SHELL_H_

#ifdef	_MSWINDOWS_
#define	INVALID_PID_VALUE	INVALID_HANDLE_VALUE
#else
#define	INVALID_PID_VALUE	-1
#endif

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
	 * Collapse argument list.
	 */
	void collapse(LinkedObject *first);

	/**
	 * Set argv0
	 */
	void set0(char *argv0);

public:
	typedef enum {NOARGS = 0, NOARGUMENT, INVARGUMENT, BADOPTION, OPTION_USED, BAD_VALUE} errmsg_t;

#ifdef	_MSWINDOWS_
	typedef	HANDLE pid_t;
#else
	typedef	int pid_t;
#endif

	typedef	enum {RD = IOBuffer::BUF_RD, WR = IOBuffer::BUF_WR, RDWR = IOBuffer::BUF_RDWR} pmode_t;

	class __EXPORT pipeio
	{
	protected:
		friend class shell;

		pipeio();

		int spawn(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);
		
		int wait(void);
		int cancel(void);
		size_t read(void *buffer, size_t size);
		size_t write(const void *buffer, size_t size);
	
		pid_t pid;
		fd_t input, output;	// input to and output from child process...
		int perror, presult;
	};

	typedef	pipeio pipe_t;

	/**
	 * This can be used to get internationalized error messages.  The internal
	 * text for shell parser errors are passed through here.
	 * @param id of error message to use.
	 * @return published text of error message.
	 */
	static const char *errmsg(errmsg_t id);

	/**
	 * This is used to set internationalized error messages for the shell
	 * parser.
	 * @param id of message to set.
	 * @param text for error message.
	 */
	static void errmsg(errmsg_t id, const char *text);

	/**
	 * A class to redefine error messages.  This can be used as a statically
	 * initialized object to remap error messages for easier
	 * internationalization.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT errormap
	{
	public:
		inline errormap(errmsg_t id, const char *text)
			{shell::errmsg(id, text);};
	};

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
	class __EXPORT flagopt : public Option
	{
	private:
		unsigned counter;
		bool single;

		virtual const char *assign(const char *value);

	public:
		flagopt(char short_option, const char *long_option = NULL, const char *help = NULL, bool single_use = true);

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
	class __EXPORT stringopt : public Option
	{
	private:
		bool used;

	protected:
		const char *text;

		virtual const char *assign(const char *value);

	public:
		stringopt(char short_option, const char *long_option = NULL, const char *help = NULL, const char *type = "text", const char *def_text = NULL);

		inline operator bool()
			{return used;};

		inline bool operator!()
			{return !used;};

		inline operator const char *()
			{return text;};

		inline const char *operator*()
			{return text;};

		char operator[](size_t index);
	};

	/**
	 * Character option for shell parsing.  This offers a quick-use class
	 * to parse a shell flag, along with a character code that may be
	 * saved.  Multiple invokations is an error.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT charopt : public Option
	{
	private:
		bool used;

	protected:
		char code;

		virtual const char *assign(const char *value);

	public:
		charopt(char short_option, const char *long_option = NULL, const char *help = NULL, const char *type = "char", char default_code = ' ');

		inline operator bool()
			{return used;};

		inline bool operator!()
			{return !used;};

		inline operator char()
			{return code;};

		inline char operator*()
			{return code;};
	};

	/**
	 * Numeric option for shell parsing.  This offers a quick-use class
	 * to parse a shell flag, along with a numberic value that may be
	 * saved and a use counter, as multiple invokations is an error.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT numericopt : public Option
	{
	private:
		bool used;

	protected:
		long number;

		virtual const char *assign(const char *value);

	public:
		numericopt(char short_option, const char *long_option = NULL, const char *help = NULL, const char *type = "numeric", long def_value = 0);

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
	 * Display shell options.
	 */
	static void help(void);

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
	 * Return parser argv element.
	 * @param offset into array.
	 * @return argument string.
	 */
	inline const char *operator[](unsigned offset)
		{return _argv[offset];};

	/**
	 * Spawn a child process.  This creates a new child process.  If
	 * the executable path is a pure filename, then the $PATH will be
	 * used to find it.  The argv array may be created from a string
	 * with the shell string parser.
	 * @param path to executable.
	 * @param argv list of command arguments for the child process.
	 * @param env of child process can be explicitly set.
	 * @return process id of child or INVALID_PID_VALUE if fails.
	 */
	static shell::pid_t spawn(const char *path, char **argv, char **env = NULL);

	/**
	 * Spawn a child pipe.  If the executable path is a pure filename, then 
	 * the $PATH will be used to find it.  The argv array may be created from 
	 * a string with the shell string parser.
	 * @param path to executable.
	 * @param argv list of command arguments for the child process.
	 * @param mode of pipe, rd only, wr only, or rdwr.
	 * @param size of pipe buffer.
	 * @param env of child process can be explicitly set.
	 * @return pipe object or NULL on failure.
	 */
	static shell::pipe_t *spawn(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);

	/**
	 * Create a detached process.  This creates a new child process that
	 * is completely detached from the current process.
	 * @param path to executable.
	 * @param argv list of command arguments for the child process.
	 * @param env of child process can be explicity set.
	 * @return 0 if success, -1 on error.
	 */
	int	detach(const char *path, char **argv, char **env = NULL);

	/**
	 * Wait for a child process to terminate.  This operation blocks.
	 * @param pid of process to wait for.
	 * @return exit code of process, -1 if fails or pid is invalid.
	 */
	static int wait(shell::pid_t pid);

	/**
	 * Cancel a child process.
	 * @param pid of child process to cancel.
	 * @return exit code of process, -1 if fails or pid is invalid.
	 */
	static int cancel(shell::pid_t pid);

	/**
	 * Wait for a child pipe to terminate.  This operation blocks.  If
	 * the pipe io handle is dynamic, it is deleted.
	 * @param pointer to pipe of child process to wait for.
	 * @return exit code of process, -1 if fails or pid is invalid.
	 */
	static int wait(shell::pipe_t *pointer);

	/**
	 * Cancel a child pipe.  If the pipe io handle is dynamic, it is deleted.
	 * @param pointer to pipe of child process to cancel.
	 * @return exit code of process, -1 if fails or pid is invalid.
	 */
	static int cancel(shell::pipe_t *pointer);

	/**
	 * Return argc count.
	 * @return argc count.
	 */
	inline unsigned operator()(void)
		{return _argc;};

	/**
	 * Get argc count for an existing array.
	 * @param argv to count items in.
	 * @return argc count of array.
	 */
	static unsigned count(char **argv);
};
		
END_NAMESPACE

#endif
