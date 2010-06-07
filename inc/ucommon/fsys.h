// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

/**
 * Thread-aware file system manipulation class.  This is used to provide
 * generic file operations that are OS independent and thread-safe in
 * behavior.  This is used in particular to wrap posix calls internally
 * to pth, and to create portable code between MSWINDOWS and Posix low-level
 * file I/O operations.
 * @file ucommon/fsys.h
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
#ifdef	_MSWINDOWS_
	WIN32_FIND_DATA *ptr;
	HINSTANCE	mem;
#else
	void	*ptr;
#endif
	int		error;

#ifdef	_MSWINDOWS_
	static int remapError(void);
#else
	inline static int remapError(void)
		{return errno;};
#endif

public:
	/**
	 * Enumerated file access modes.
	 */
	typedef enum {
		ACCESS_RDONLY,
		ACCESS_WRONLY,
		ACCESS_REWRITE,
		ACCESS_RDWR = ACCESS_REWRITE,
		ACCESS_APPEND,
		ACCESS_SHARED,
		ACCESS_DIRECTORY,
		ACCESS_STREAM,
		ACCESS_RANDOM
	} access_t;

	/**
	 * File offset type.
	 */
	typedef long offset_t;

	/**
	 * Used to mark "append" in set position operations.
	 */
	static const offset_t end;

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
	 * Create a fsys descriptor by opening an existing file or directory.
	 * @param path of file to open for created descriptor.
	 * @param access mode of file.
	 */
	fsys(const char *path, access_t access);

	/**
	 * Create a fsys descriptor by creating a file.
	 * @param path of file to create for descriptor.
	 * @param access mode of file access.
	 * @param permission mode of file.
	 */
	fsys(const char *path, access_t access, unsigned permission);

	/**
	 * Close and release a file descriptor.
	 */
	~fsys();

	/**
	 * Get the descriptor from the object by pointer reference.
	 * @return low level file handle.
	 */
	inline fd_t operator*() const
		{return fd;};

	/**
	 * Get the descriptor from the object by casting reference.
	 * @return low level file handle.
	 */
	inline operator fd_t() const
		{return fd;};

	/**
	 * Test if file descriptor is open.
	 * @return true if open.
	 */
	inline operator bool() const
		{return fd != INVALID_HANDLE_VALUE || ptr != NULL;};

	/**
	 * Test if file descriptor is closed.
	 * @return true if closed.
	 */
	inline bool operator!() const
		{return fd == INVALID_HANDLE_VALUE && ptr == NULL;};

	/**
	 * Assign file descriptor by duplicating another descriptor.
	 * @param descriptor to dup from.
	 */
	void operator=(const fsys& descriptor);

	/**
	 * Assing file descriptor from system descriptor.
	 * @param descriptor to dup from.
	 */
	void operator=(fd_t descriptor);

	/**
	 * Get the error number (errno) associated with the descriptor from
	 * the last error event from an opened file.
	 * @return error number.
	 */
	inline int getError(void) const
		{return error;};

	/**
	 * Get the native system descriptor handle of the file descriptor.
	 * @return native os descriptor.
	 */
	inline fd_t getHandle(void) const
		{return fd;};

	/**
	 * Set the position of a file descriptor.
	 * @param offset from start of file or "end" to append.
	 * @return error number or 0 on success.
	 */
	int seek(offset_t offset);

	/**
	 * Drop cached data from start of file.
	 * @param size of region to drop or until end of file.
	 * @return error number or 0 on success.
	 */
	int drop(offset_t size = 0);	

	/**
	 * Read data from descriptor or scan directory.
	 * @param buffer to read into.
	 * @param count of bytes to read.
	 * @return bytes transferred, -1 if error.
	 */
	ssize_t read(void *buffer, size_t count);

	/**
	 * Write data to descriptor.
	 * @param buffer to write from.
	 * @param count of bytes to write.
	 * @return bytes transferred, -1 if error.
	 */
	ssize_t write(const void *buffer, size_t count);

	/**
	 * Get status of open descriptor.
	 * @param buffer to save status info in.
	 * @return error number or 0 on success.
	 */
	int stat(struct stat *buffer);

	/**
	 * Truncate file to specified length.  The file pointer is positioned
	 * to the new end of file.
	 * @param offset to truncate to.
	 * @return true if truncate successful.
	 */
	int trunc(offset_t offset);

	/**
	 * Commit changes to the filesystem.
	 * @return error number or 0 on success.
	 */
	int sync(void);

	/**
	 * Set directory prefix (chdir).
	 * @param path to change to.
	 * @return error number or 0 on success.
	 */
	static int changeDir(const char *path);

	/**
	 * Get current directory prefix (pwd).
	 * @param path to save directory into.
	 * @param size of path we can save.
	 * @return error number or 0 on success.
	 */
	static int getPrefix(char *path, size_t size);

	/**
	 * Stat a file.
	 * @param path of file to stat.
	 * @param buffer to save stat info.	
	 * @return error number or 0 on success.
	 */
	static int stat(const char *path, struct stat *buffer);
	
	/**
	 * Remove a file.
	 * @param path of file.
	 * @return error number or 0 on success.
	 */
	static int remove(const char *path);

	/**
	 * Rename a file.
	 * @param oldpath to rename from.
	 * @param newpath to rename to.
	 * @return error number or 0 on success.
	 */
	static int rename(const char *oldpath, const char *newpath);

	/**
	 * Change file access mode.
	 * @param path to change.
	 * @param mode to assign.
	 * @return error number or 0 on success.
	 */
	static int change(const char *path, unsigned mode);
	
	/**
	 * Test path access.
	 * @param path to test.
	 * @param mode to test for.
	 * @return error number or 0 on success.
	 */
	static int access(const char *path, unsigned mode);

	/**
	 * Test if path is a file.
	 * @param path to test.
	 * @return true if exists and is file.
	 */
	static bool isfile(const char *path);

	/**
	 * Test if path is a directory.
	 * @param path to test.
	 * @return true if exists and is directory.
	 */
	static bool isdir(const char *path);


	/**
	 * Read data from file descriptor or directory.
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
	 * @return error number or 0 on success.
	 */
	inline static int seek(fsys& descriptor, offset_t offset)
		{return descriptor.seek(offset);};

	/**
	 * Drop cached data from a file descriptor.
	 * @param descriptor to set.
	 * @param size of region from start of file to drop or all.
	 * @return error number or 0 on success.
	 */
	inline static int drop(fsys& descriptor, offset_t size)
		{return descriptor.drop(size);};

	/**
	 * Open a file or directory.
	 * @param path of file to open.
	 * @param access mode of descriptor.
	 */
	void open(const char *path, access_t access);

	/**
	 * Assign descriptor directly.
	 * @param descriptor to assign.
	 */
	inline void assign(fd_t descriptor)
		{close(); fd = descriptor;};

	/**
	 * Assign a descriptor directly.
	 * @param object to assign descriptor to.
	 * @param descriptor to assign.
	 */
	inline static void assign(fsys& object, fd_t descriptor)
		{object.close(); object.fd = descriptor;};

	/**
	 * Open a file descriptor directly.
	 * @param path of file to create.
	 * @param access mode of descriptor.
	 * @param mode of file if created.
	 */
	void create(const char *path, access_t access, unsigned mode);

	/**
	 * Simple direct method to create a directory.
	 * @param path of directory to create.
	 * @param mode of directory.
	 * @return error number or 0 on success.
	 */
	static int createDir(const char *path, unsigned mode); 

	/**
	 * Simple direct method to remove a directory.
	 * @param path to remove.
	 * @return error number or 0 on success.
	 */
	static int removeDir(const char *path);

	/**
	 * Close a file descriptor or directory directly.
	 * @param descriptor to close.
	 */
	inline static void close(fsys& descriptor)
		{descriptor.close();};

	/**
	 * Close a fsys resource.
	 */
	void close(void);

	/**
	 * Open a file or directory.
	 * @param object to assign descriptor for opened file.
	 * @param path of file to open.
	 * @param access mode of descriptor.
	 */
	inline static void open(fsys& object, const char *path, access_t access)
		{object.open(path, access);};

	/**
	 * create a file descriptor or directory directly.
	 * @param object to assign descriptor for created file.
	 * @param path of file to create.
	 * @param access mode of descriptor.
	 * @param mode of file if created.
	 */
	inline static void create(fsys& object, const char *path, access_t access, unsigned mode)
		{object.create(path, access, mode);};

	/**
	 * Load an unmaged plugin directly.
	 * @param path to plugin.
	 * @return error number or 0 on success.
	 */
	static int load(const char *path);

	/**
	 * Load a plugin into memory.
	 * @param module for management.
	 * @param path to plugin.
	 */
	static void load(fsys& module, const char *path);

	/**
	 * unload a specific plugin.
	 * @param module to unload
	 */
	static void unload(fsys& module);
	
	/**
	 * Find symbol in loaded module.
	 * @param module to search.
	 * @param symbol to search for.
	 * @return address of symbol or NULL if not found.
	 */
	static void *find(fsys& module, const char *symbol);
};
	
/**
 * Convience type for fsys.
 */
typedef	fsys fsys_t;

END_NAMESPACE

#endif

