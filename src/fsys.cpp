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

#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/fsys.h>
#include <stdio.h>

#ifndef	_MSWINDOWS_
#include <fcntl.h>
#endif

using namespace UCOMMON_NAMESPACE;

const size_t fsys::end = (size_t)(-1);

#ifdef	_MSWINDOWS_

int fsys::removeDir(const char *path)
{
	return _rmdir(path);
}

int fsys::createDir(const char *path, unsigned mode)
{
	if(!CreateDirectory(path, NULL))
		return -1;
	
	return _chmod(path, mode);
}

int fsys::stat(const char *path, struct stat *buf)
{
	return _stat(path, (struct _stat *)(buf));
}

int fsys::stat(struct stat *buf)
{
	int fn = _open_osfhandle(fs.fd, _O_RDONLY);
	
	int rtn = _fstat(fn, (struct _stat *)(buf));
	_close(fn);
	if(rtn)
		error = errno;
	return rtn;
}	 

int fsys::setPrefix(const char *path)
{
	return _chdir(path);
}

char *fsys::getPrefix(char *path, size_t len)
{
	return _getcwd(path, len);
}

int fsys::chmode(const char *path, unsigned mode)
{
	return _chmod(path, mode);
}

int fsys::access(const char *path, unsigned mode)
{
	return _access(path, mode);
}

#else
#ifdef	__PTH__

ssize_t fsys::read(fd_t fd, void *buf, size_t len)
{
	return pth_read(fd, buf, len);
}

ssize_t fsys::read(void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = pth_read(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

ssize_t fsys::write(fd_t fd, const void *buf, size_t len)
{
	return pth_write(fd, buf, len);
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = pth_write(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

#else

ssize_t fsys::read(fd_t fd, void *buf, size_t len)
{
	return ::read(fd, buf, len);
}

ssize_t fsys::read(void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = ::read(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

ssize_t fsys::write(fd_t fd, const void *buf, size_t len)
{
	return ::write(fd, buf, len);
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = ::write(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

#endif

int fsys::close(fd_t fd)
{
	return ::close(fd);
}

void fsys::close(fsys &fs)
{
	if(fs.fd == INVALID_HANDLE_VALUE)
		return;

	if(::close(fs.fd) == 0) {
		fs.fd = INVALID_HANDLE_VALUE;
		fs.error = 0;
	}
	else
		fs.error = errno;
}

fd_t fsys::open(const char *path, access_t access, unsigned mode)
{
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		flags = O_RDWR | O_CREAT;
		break;
	case ACCESS_CREATE:
		flags = O_RDWR | O_TRUNC | O_CREAT | O_EXCL;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_CREAT | O_APPEND;
		break;
	}
	return ::open(path, flags, mode);
}

void fsys::open(fsys &fs, const char *path, access_t access, unsigned mode)
{
	unsigned flags = 0;

	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		flags = O_RDWR | O_CREAT;
		break;
	case ACCESS_CREATE:
		flags = O_RDWR | O_TRUNC | O_CREAT | O_EXCL;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_CREAT | O_APPEND;
		break;
	}
	close(fs);
	if(fs.fd != INVALID_HANDLE_VALUE)
		return;

	fs.fd = ::open(path, flags, mode);
	if(fs.fd == INVALID_HANDLE_VALUE)
		fs.error = errno;
}

int fsys::stat(const char *path, struct stat *ino)
{
	return ::stat(path, ino);
}

int fsys::stat(struct stat *ino)
{
	int rtn = ::fstat(fd, ino);
	if(rtn)
		error = errno;
	return rtn;
}

int fsys::createDir(const char *path, unsigned mode)
{
	return ::mkdir(path, mode);
}

int fsys::removeDir(const char *path)
{
	return ::rmdir(path);
}

int fsys::setPrefix(const char *path)
{
	return ::chdir(path);
}

char *fsys::getPrefix(char *path, size_t len)
{
	return ::getcwd(path, len);
}

int fsys::chmode(const char *path, unsigned mode)
{
	return ::chmod(path, mode);
}

int fsys::access(const char *path, unsigned mode)
{
	return ::access(path, mode);
}

fsys::fsys(const fsys& copy)
{
	fd = dup(copy.fd);
	error = 0;
}

void fsys::operator=(fd_t fd)
{
	if(fsys::close(fd) == 0) {
		fd = dup(fd);
		error = 0;
	}
	else
		error = errno;
}

void fsys::operator=(fsys &from)
{
	if(fsys::close(fd) == 0) {
		fd = dup(from.fd);
		error = 0;
	}
	else
		error = errno;
}

void fsys::setPosition(size_t pos)
{
	unsigned long rpos = pos;
	int mode = SEEK_SET;

	if(rpos == end) {
		rpos = 0;
		mode = SEEK_END;
	}
	if(lseek(fd, mode, rpos))
		error = errno;
}

int fsys::setPosition(fd_t fd, size_t pos)
{
	unsigned long rpos = pos;
	int mode = SEEK_SET;

	if(rpos == end) {
		rpos = 0;
		mode = SEEK_END;
	}
	return lseek(fd, mode, rpos);
}

#endif

fsys::fsys()
{
	fd = INVALID_HANDLE_VALUE;
	error = 0;
}

fsys::fsys(const char *path, access_t access, unsigned mode)
{
	fd = fsys::open(path, access, mode);
}

fsys::~fsys()
{
	if(fsys::close(fd) == 0) {
		fd = INVALID_HANDLE_VALUE;
		error = 0;
	}
	else
		error = errno;
}

int fsys::remove(const char *path)
{
	return ::remove(path);
}


