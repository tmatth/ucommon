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
	 * Collapse argument list.  This is used internally to collapse args
	 * that are created in a pool heap when they need to be turned into
	 * an argv style array.
	 */
	void collapse(LinkedObject *first);

	/**
	 * Set argv0.  This gets the simple filename of argv[0].
	 */
	void set0(char *argv0);

public:
	/**
	 * Error table index.
	 */
	typedef enum {NOARGS = 0, NOARGUMENT, INVARGUMENT, BADOPTION, OPTION_USED, BAD_VALUE} errmsg_t;

#ifdef	_MSWINDOWS_
	typedef	HANDLE pid_t;
#else
	/**
	 * Standard type of process id for shell class.
	 */
	typedef	int pid_t;
#endif

	/**
	 * Pipe I/O mode.
	 */
	typedef	enum {RD = IOBuffer::BUF_RD, WR = IOBuffer::BUF_WR, RDWR = IOBuffer::BUF_RDWR} pmode_t;

	/**
	 * A class to control a process that is piped.  This holds the active
	 * file descriptors for the pipe as well as the process id.  Basic I/O
	 * methods are provided to send and receive data with the piped child
	 * process.  This may be used by itself with various shell methods as
	 * a pipe_t, or to construct piped objects such as iobuf.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT pipeio
	{
	protected:
		friend class shell;

		/**
		 * Construct an empty initialized pipe for use.
		 */
		pipeio();

		/**
		 * Spawn and attach child process I/O through piping.  Stderr is left
		 * attached to the console.
		 * @param path of program to execute.  If simple file, $PATH is used.
		 * @param argv to pass to child process.
		 * @param mode of pipe operation; rdonly, wronly, or rdwr.
		 * @param size of atomic pipe buffer if setable.
		 * @param env that may optionally be given to the child process.
		 */
		int spawn(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);
		
		/**
		 * Wait for child process to exit.  When it does, close all piping.
		 * @return process exit code or -1 if error.
		 */
		int wait(void);

		/**
		 * Signal termination and wait for child process to exit.  When it
		 * does all piping is closed.
		 * @return process exit code or -1 if error.
		 */
		int cancel(void);

		/**
		 * Read input from child process.  If there is an error, the result
		 * is 0 and perror holds the error code.  If an error already
		 * happened no further data will be read.
		 * @param address to store input.
		 * @param size of input to read.
		 * @return number of bytes actually read.
		 */
		size_t read(void *address, size_t size);

		/**
		 * Write to the child process.  If there is an error, the result
		 * is 0 and perror holds the error code.  If an error already
		 * happened no further data will be written.
		 * @param address to write data from.
		 * @param size of data to write.
		 * @return number of bytes actually written.
		 */
		size_t write(const void *address, size_t size);
			
		pid_t pid;
		fd_t input, output;	// input to and output from child process...
		int perror, presult;
	};

	/**
	 * Process pipe with I/O buffering.  This allows the creation and management
	 * of a shell pipe with buffered I/O support.  This also offers a common
	 * class to manage stdio sessions generically in the child process.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT iobuf : public IOBuffer, protected pipeio
	{
	protected:
		friend class shell;

		virtual size_t _push(const char *address, size_t size);
		virtual size_t _pull(char *address, size_t size);

	public:
		/**
		 * Construct an i/o buffer. If a non-zero size is specified, then
		 * the object is attached to the process's stdin & stdout.  Otherwise
		 * an un-opened object is created.
		 */
		iobuf(size_t size = 0);

		/**
		 * Construct an i/o buffer for a child process.  This is used to
		 * create a child process directly when the object is made.  It
		 * essentially is the same as the open() method.
		 * @param path of program to execute, if filename uses $PATH.
		 * @param argv to pass to child process.
		 * @param mode of pipe, rdonly, wronly, or rdwr.
		 * @param size of buffering, and atomic pipe size if setable.
		 * @param env that may be passed to child process.
		 */
		iobuf(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);

		/**
		 * Destroy i/o buffer.  This may cancel and block waiting for a child
		 * process to terminate.
		 */
		~iobuf();

		/**
		 * Open the i/o buffer attached to a child process.
		 * @param path of program to execute, if filename uses $PATH.
		 * @param argv to pass to child process.
		 * @param mode of pipe, rdonly, wronly, or rdwr.
		 * @param size of buffering, and atomic pipe size if setable.
		 * @param env that may be passed to child process.
		 */
		void open(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);

		/**
		 * Close the i/o buffer. If attached to a child process it will wait
		 * for the child to exit.
		 */
		void close(void);

		/**
		 * Terminate child process.  This also waits for the child process
		 * to exit and then closes buffers.
		 */
		void cancel(void);
	};

	/**
	 * Buffered i/o type for pipes and stdio.
	 */
	typedef	iobuf io_t;

	/**
	 * Convenience low level pipe object type.
	 */
	typedef	pipeio *pipe_t;

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
		 * Disable a option.  Might happen if argv0 name suggests an
		 * option is no longer actively needed.
		 */
		void disable(void);

		/**
		 * Used to send option into derived receiver.
		 * @param value option that was received.
		 * @return NULL or error string to use.
		 */
		virtual const char *assign(const char *value) = 0;

		inline static void reset(void)
			{index.reset();};
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
	 * Get an environment variable.  This creates a local copy of the
	 * variable in pager memory.
	 * @param name of symbol.
	 * @param value of symbol if not found.
	 * @return value of symbol.
	 */
	const char *getenv(const char *name, const char *value = NULL);

	/**
	 * Parse and extract the argv0 filename alone.
	 * @param argv from main.
	 * @return argv0 simple path name.
	 */
	char *getargv0(char **argv);

	/**
	 * Get the argument list by parsing options, and return the remaining
	 * file arguments.  This is used by parse, and can be fed by main by
	 * posting ++argv.
	 * @param argv of first option.
	 * @return argv of non-option file list.
	 */
	char **getargv(char **argv);

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
	static size_t printf(const char *format, ...) __PRINTF(1, 2);

	static size_t readln(char *address, size_t size);

	static size_t writes(const char *string);

	static size_t read(String& string);

	inline static size_t write(String& string)
		{return writes(string.c_str());};

	/**
	 * Print to a pipe object.
	 * @param pipe to write to.
	 * @param format string to use.
	 * @return number of bytes written.
	 */
	static size_t printf(pipe_t pipe, const char *format, ...) __PRINTF(2, 3);

	/**
	 * Read a line from a pipe object.
	 * @param pipe to read from.
	 * @param buffer to save into.
	 * @param size of buffer.
	 * @return number of bytes read.
	 */
	static size_t readln(pipe_t pipe, char *buffer, size_t size);

	static size_t read(pipe_t pipe, String& string);

	static size_t writes(pipe_t pipe, const char *string);

	inline static size_t write(pipe_t pipe, String& string)
		{return writes(pipe, string.c_str());};

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
	 * @param stdio handles for stdin, stdout, and stderr.
	 * @return process id of child or INVALID_PID_VALUE if fails.
	 */
	static shell::pid_t spawn(const char *path, char **argv, char **env = NULL, fd_t *stdio = NULL);

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
	static shell::pipe_t spawn(const char *path, char **argv, pmode_t mode, size_t size = 512, char **env = NULL);

	/**
	 * Create a detached process.  This creates a new child process that
	 * is completely detached from the current process.
	 * @param path to executable.
	 * @param argv list of command arguments for the child process.
	 * @param env of child process can be explicity set.
	 * @param stdio handles for stdin, stdout, and stderr.
	 * @return 0 if success, -1 on error.
	 */
	int	detach(const char *path, char **argv, char **env = NULL, fd_t *stdio = NULL);

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
	static int wait(shell::pipe_t pointer);

	/**
	 * Cancel a child pipe.  If the pipe io handle is dynamic, it is deleted.
	 * @param pointer to pipe of child process to cancel.
	 * @return exit code of process, -1 if fails or pid is invalid.
	 */
	static int cancel(shell::pipe_t pointer);

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

#ifdef	_MSWINDOWS_

	static inline fd_t input(void)
		{return GetStdHandle(STD_INPUT_HANDLE);}; 

	static inline fd_t output(void)
		{return GetStdHandle(STD_OUTPUT_HANDLE);}; 

	static inline fd_t error(void)
		{return GetStdHandle(STD_ERROR_HANDLE);};

#else
	static inline fd_t input(void)
		{return 0;};

	static inline fd_t output(void)
		{return 1;};

	static inline fd_t error(void)
		{return 2;};
#endif
};
	
END_NAMESPACE

#endif
