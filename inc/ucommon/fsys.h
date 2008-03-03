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

typedef	void *dir_t;

class __EXPORT fsys
{
protected:
	fd_t	fd;
	int		error;

	typedef enum {
		ACCESS_CREATE,
		ACCESS_RDONLY,
		ACCESS_WRONLY,
		ACCESS_REWRITE,
		ACCESS_APPEND,
		ACCESS_SHARED
	} access_t;

public:
	static const size_t end;

	fsys();
	fsys(const fsys& copy);
	fsys(const char *path, access_t type, unsigned mode = 0666);
	~fsys();

	inline operator bool()
		{return fd != INVALID_HANDLE_VALUE;};

	inline bool operator!()
		{return fd == INVALID_HANDLE_VALUE;};

	inline void operator=(fsys& fd);
	inline void operator=(fd_t fd);

	inline int getError(void)
		{return error;};

	inline fd_t getHandle(void)
		{return fd;};
	
	void	setPosition(size_t pos);
	ssize_t read(void *buf, size_t count, size_t offset = end);
	ssize_t write(const void *buf, size_t count, size_t offset = end);
	int stat(struct stat *buf);

	static int setPrefix(const char *path);
	static char *getPrefix(char *path, size_t len);
	static int stat(const char *path, struct stat *buf);

	static int createDir(const char *path, unsigned mode);
	static int removeDir(const char *path);
	static int remove(const char *path);
	static int rename(const char *oldpath, const char *newpath);
	static int change(const char *path, unsigned mode);
	static fd_t open(const char *path, access_t type, unsigned mode = 0666);
	static int close(fd_t fd);
	static ssize_t read(fd_t fd, void *buf, size_t count, size_t offset = end);
	static ssize_t write(fd_t fd, const void *buf, size_t count, size_t offset = end);
	static int access(const char *path, unsigned mode);

	inline static ssize_t read(fsys& fd, void *buf, size_t count, size_t offset = end)
		{return fd.read(buf, count, offset);};

	inline static ssize_t write(fsys& fd, const void *buf, size_t count)
		{return fd.write(buf, count);};

	inline static void setPosition(fsys& fd, size_t pos)
		{fd.setPosition(pos);};

	static int setPosition(fd_t fd, size_t pos);	
	static void open(fsys& fd, const char *path, access_t access, unsigned mode);
	static void close(fsys& fd);

	static dir_t openDir(const char *path);
	static char *readDir(dir_t dir, char *buf, size_t len);
	static void closeDir(dir_t dir);

	static void *load(const char *path);
	static void unload(void *addr);
	static void *find(void *addr, const char *sym);
};

END_NAMESPACE

#endif

