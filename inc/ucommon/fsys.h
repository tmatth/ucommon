// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
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
 * Thread-aware file system manipulation class.  This is used to provide
 * generic file operations that are OS independent and thread-safe in
 * behavior.  This is used in particular to wrap posix calls internally
 * to pth, and to create portable code between MSWINDOWS and Posix low-level
 * file I/O operations.
 * @file ucommon/file.h
 */

#ifndef	_UCOMMON_FILE_H_
#define	_UCOMMON_FILE_H_

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef	_MSWINDOWS_
#include <sys/stat.h>
#endif

#include <errno.h>

NAMESPACE_UCOMMON

/**
 * Convenience type for directory scan operations.
 */
typedef	void *dir_t;

/**
 * Convenience type for loader operations.
 */
typedef	void *mem_t;

/**
 * A container for generic and o/s portable threadsafe file system functions.
 * These are based roughly on their posix equivilents.  For libpth, the
 * system calls are wrapped.  The native file descriptor or handle may be
 * used, but it is best to use "class fsys" instead because it can capture
 * the errno of a file operation in a threadsafe and platform independent
 * manner, including for mswindows targets.
 */
class __EXPORT fsys
{
protected:
	fd_t	fd;
	int		error;

public:
	/**
	 * Enumerated file access modes.
	 */
	typedef enum {
		ACCESS_CREATE,
		ACCESS_RDONLY,
		ACCESS_WRONLY,
		ACCESS_REWRITE,
		ACCESS_APPEND,
		ACCESS_SHARED
	} access_t;

	/**
	 * Used to mark "append" in set position operations.
	 */
	static const size_t end;

	/**
	 * Construct an unattached fsys descriptor.
	 */
	fsys();

	/**
	 * Copy (dup) an existing fsys descriptor.
	 * @param descriptor to copy from.
	 */
	fsys(const fsys& descriptor);

	/**
	 * Create a fsys descriptor by opening a file.
	 * @param type of file access.
	 * @param mode of file.
	 */
	fsys(const char *path, access_t type, unsigned mode = 0666);

	/**
	 * Close and release a file descriptor.
	 */
	~fsys();

	/**
	 * Test if file descriptor is open.
	 * @return true if open.
	 */
	inline operator bool()
		{return fd != INVALID_HANDLE_VALUE;};

	/**
	 * Test if file descriptor is closed.
	 * @return true if closed.
	 */
	inline bool operator!()
		{return fd == INVALID_HANDLE_VALUE;};

	/**
	 * Assign file descriptor by duplicating another descriptor.
	 * @param descriptor to dup from.
	 */
	inline void operator=(fsys& descriptor);

	/**
	 * Assing file descriptor from system descriptor.
	 * @param descriptor to dup from.
	 */
	inline void operator=(fd_t descriptor);

	/**
	 * Get the error number (errno) associated with the descriptor from
	 * the last error recorded.
	 * @return error number.
	 */
	inline int getError(void)
		{return error;};

	/**
	 * Get the native system descriptor handle of the file descriptor.
	 * @return native os descriptor.
	 */
	inline fd_t getHandle(void)
		{return fd;};
	
	/**
	 * Set the position of a file descriptor.
	 * @param offset from start of file or "end" to append.
	 */
	void	setPosition(size_t offset);

	/**
	 * Read data from descriptor.
	 * @param buffer to read into.
	 * @param count of bytes to read.
	 * @return bytes transferred, -1 if error.
	 */
	ssize_t read(void *buffer, size_t count);

	/**
	 * Write data to descriptor.
	 * @param buffer to write from.
	 * @param count of bytes to write.
	 * @param offset from start of file or "end" for current position.
	 * @return bytes transferred, -1 if error.
	 */
	ssize_t write(const void *buffer, size_t);

	/**
	 * Get status of open descriptor.
	 * @param buffer to save status info in.
	 * @return 0 on success, -1 on error.
	 */
	int stat(struct stat *buffer);

	/**
	 * Set directory prefix (chdir).
	 * @param path to change to.
	 * @return 0 on success, -1 on error.
	 */
	static int setPrefix(const char *path);

	/**
	 * Get current directory prefix (pwd).
	 * @param path to save directory into.
	 * @param size of path we can save.
	 * @return path of current directory or NULL.
	 */
	static char *getPrefix(char *path, size_t size);

	/**
	 * Stat a file.
	 * @param path of file to stat.
	 * @param buffer to save stat info.	
	 * @return 0 on success, -1 on error.
	 */
	static int stat(const char *path, struct stat *buffer);
	
	/**
	 * Create a directory.
	 * @param path of directory.
	 * @param mode of directory.
	 * @return 0 on success, -1 on error.
	 */
	static int createDir(const char *path, unsigned mode = 0660);

	/**
	 * Remove a directory.  Only if empty.
	 * @param path of directory.
	 * @return 0 on success, -1 on error.
	 */
	static int removeDir(const char *path);

	/**
	 * Remove a file.
	 * @param path of file.
	 * @return 0 on success, -1 on error.
	 */
	static int remove(const char *path);

	/**
	 * Rename a file.
	 * @param oldpath to rename from.
	 * @param newpath to rename to.
	 * @return 0 on success, -1 on error.
	 */
	static int rename(const char *oldpath, const char *newpath);

	/**
	 * Change file access mode.
	 * @param path to change.
	 * @param mode to assign.
	 */
	static int change(const char *path, unsigned mode);
	
	/**
	 * Open a native file descriptor directly.
	 * @param path of file to open.
	 * @param access mode of descriptor.
	 * @param mode of file if created.
	 * @return native file descriptor.
	 */
	static fd_t open(const char *path, access_t type, unsigned mode = 0666);

	/**
	 * Close a native file descriptor.
	 * @param descriptor to close.
	 */
	static int close(fd_t descriptor);

	/**
	 * Read data from native descriptor.
	 * @param descriptor to read from.
	 * @param buffer to read into.
	 * @param count of bytes to read.
	 * @return bytes transferred, -1 if error.
	 */
	static ssize_t read(fd_t descriptor, void *buffer, size_t count);
	
	/**
	 * write data to native descriptor.
	 * @param descriptor to write to.
	 * @param buffer to write from.
	 * @param count of bytes to write.
	 * @return bytes transferred, -1 if error.
	 */
	static ssize_t write(fd_t descriptor, const void *buffer, size_t count);
	
	/**
	 * Test path access.
	 * @param path to test.
	 * @param mode to test for.
	 * @return 0 if accessible, -1 if not.
	 */
	static int access(const char *path, unsigned mode);

	/**
	 * Read data from file descriptor.
	 * @param descriptor to read from.
	 * @param buffer to read into.
	 * @param count of bytes to read.
	 * @return bytes transferred, -1 if error.
	 */
	inline static ssize_t read(fsys& descriptor, void *buffer, size_t count)
		{return descriptor.read(buffer, count);};

	/**
	 * write data to file descriptor.
	 * @param descriptor to write to.
	 * @param buffer to write from.
	 * @param count of bytes to write.
	 * @return bytes transferred, -1 if error.
	 */
	inline static ssize_t write(fsys& descriptor, const void *buffer, size_t count)
		{return descriptor.write(buffer, count);};

	/**
	 * Set the position of a file descriptor.
	 * @param descriptor to set.
	 * @param offset from start of file or "end" to append.
	 */
	inline static void setPosition(fsys& descriptor, size_t offset)
		{descriptor.setPosition(offset);};

	/**
	 * Set the position of a native file descriptor.
	 * @param descriptor to set.
	 * @param offset from start of file or "end" to append.
	 * @return 0 if successful, -1 on error.
	 */
	static int setPosition(fd_t descriptor, size_t pos);	

	/**
	 * Open a file descriptor directly.
	 * @param path of file to open.
	 * @param access mode of descriptor.
	 * @param mode of file if created.
	 */
	static void open(fsys& descriptor, const char *path, access_t access, unsigned mode);

	/**
	 * Close a file descriptor directly.
	 * @param descriptor to close.
	 */
	static void close(fsys& descriptor);

	/**
	 * Open a directory for scanning.
	 * @param path to open.
	 * @return directory handle or NULL on error.
	 */
	static dir_t openDir(const char *path);

	/**
	 * Read a directory entry from the handle.
	 * @param directory handle to read from.
	 * @param buffer to save filename into.
	 * @param size of buffer.
	 * @return filename found or NULL if end of directory.
	 */
	static char *readDir(dir_t directory, char *buffer, size_t size);

	/**
	 * Close a directory handle.
	 * @param directory handle to close.
	 */
	static void closeDir(dir_t directory);

	/**
	 * Load a plugin into memory.
	 * @param path to plugin.
	 * @return module handle or NULL if not loaded.
	 */
	static mem_t load(const char *path);

	/**
	 * Unload a plugin from memory.
	 * @param module to unload.
	 */
	static void unload(mem_t module);

	/**
	 * Find symbol in loaded module.
	 * @param module to search.
	 * @param symbol to search for.
	 * @return address of symbol or NULL if not found.
	 */
	static void *find(mem_t module, const char *symbol);
};

END_NAMESPACE

#endif

