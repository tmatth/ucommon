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
#include <ucommon/string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#ifdef	HAVE_DIRENT_H
#include <dirent.h>
#endif

using namespace UCOMMON_NAMESPACE;

const size_t fsys::end = (size_t)(-1);

#ifdef	_MSWINDOWS_

int remapError(void)
{
	DWORD err = GetLastError();

	switch(err)
	{
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_NAME:
	case ERROR_BAD_PATHNAME:
		return ENOENT;
	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;
	case ERROR_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return ENOMEM;
	case ERROR_INVALID_DRIVE:
	case ERROR_BAD_UNIT:
	case ERROR_BAD_DEVICE:
		return ENODEV;
	case ERROR_NOT_SAME_DEVICE:
		return EXDEV;
	case ERROR_NOT_SUPPORTED:
	case ERROR_CALL_NOT_IMPLEMENTED:
		return ENOSYS;
	case ERROR_END_OF_MEDIA:
	case ERROR_EOM_OVERFLOW:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_DISK_FULL:
		return ENOSPC;
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_NET_NAME:
		return EACCES;
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_CANNOT_MAKE:
	case ERROR_NOT_OWNER:
		return EPERM;
	case ERROR_NO_PROC_SLOTS:
		return EAGAIN;
	case ERROR_BROKEN_PIPE:
	case ERROR_NO_DATA:
		return EPIPE;
	case ERROR_OPEN_FAILED:
		return EIO;
	case ERROR_NOACCESS:
		return EFAULT;
	case ERROR_IO_DEVICE:
	case ERROR_CRC:
	case ERROR_NO_SIGNAL_SENT:
		return EIO;
	case ERROR_CHILD_NOT_COMPLETE:
	case ERROR_SIGNAL_PENDING:
	case ERROR_BUSY:
		return EBUSY;
	default:
		return EINVAL;
	}
}

int fsys::createPrefix(const char *path, unsigned mode)
{
	if(!CreateDirectory(path, NULL))
		return remapError();
	
	return change(path, mode);
}

int fsys::stat(const char *path, struct stat *buf)
{
	if(_stat(path, (struct _stat *)(buf)))
		return remapError();
}

int fsys::stat(struct stat *buf)
{
	int fn = _open_osfhandle((long int)(fd), _O_RDONLY);
	
	int rtn = _fstat(fn, (struct _stat *)(buf));
	_close(fn);
	if(rtn)
		error = remapError();
	return rtn;
}	 

int fsys::setPrefix(const char *path)
{
	if(_chdir(path))
		return remapError();
	return 0;
}

char *fsys::getPrefix(char *path, size_t len)
{
	if(_getcwd(path, len))
		return remapError();
	return 0;
}

int fsys::change(const char *path, unsigned mode)
{
	if(_chmod(path, mode))
		return remapError();
	return 0;
}

int fsys::access(const char *path, unsigned mode)
{
	if(_access(path, mode))
		return remapError();
	return 0;
}

void fsys::close(void)
{
	error = 0;
	if(fd == INVALID_HANDLE_VALUE)
		return;

	if(ptr) {
		if(::FindClose(fd)) {
			delete ptr;
			ptr = NULL;
			fd = INVALID_HANDLE_VALUE;
		}
		else
			error = remapError();
	}
	else if(::CloseHandle(fd))
		fd = INVALID_HANDLE_VALUE;
	else
		error = remapError();
}

ssize_t fsys::read(void *buf, size_t len)
{
	ssize_t rtn = -1;
	DWORD count;

	if(ReadFile(fd, (LPVOID) buf, (DWORD)len, &count, NULL))
		rtn = count;
	else		
		error = remapError();
	
	return rtn;
}

ssize_t fsys::write(const void *buf, size_t len)
{
	ssize_t rtn = -1;
	DWORD count;

	if(WriteFile(fd, (LPVOID) buf, (DWORD)len, &count, NULL))
		rtn = count;
	else		
		error = remapError();

	return rtn;
}

fd_t fsys::open(const char *path, access_t access)
{
	bool append = false;
	DWORD amode;
	DWORD cmode;
	DWORD smode = 0;
	DWORD attr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		amode = GENERIC_READ;
		cmode = OPEN_EXISTING;
		smode = FILE_SHARE_READ;
		break;
	case ACCESS_WRONLY:
		amode = GENERIC_WRITE;
		cmode = CREATE_ALWAYS; 
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		smode = FILE_SHARE_READ;
		break;
	case ACCESS_CREATE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = CREATE_NEW;	
		smode = FILE_SHARE_READ;	
		break;
	case ACCESS_APPEND:
		amode = GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		append = true;
		break;
	case ACCESS_SHARED:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		smode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	HANDLE fd = CreateFile(path, amode, smode, NULL, cmode, attr, NULL);
	if(fd != INVALID_HANDLE_VALUE && append)
		setPosition(fd, end);
	if(fd != INVALID_HANDLE_VALUE)
		change(path, mode);
	return fd;	
}

void fsys::open(fsys &fs, const char *path, access_t access, unsigned mode)
{
	bool append = false;
	DWORD amode;
	DWORD cmode;
	DWORD smode = 0;
	DWORD attr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		amode = GENERIC_READ;
		cmode = OPEN_EXISTING;
		smode = FILE_SHARE_READ;
		break;
	case ACCESS_WRONLY:
		amode = GENERIC_WRITE;
		cmode = CREATE_ALWAYS; 
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		smode = FILE_SHARE_READ;
		break;
	case ACCESS_CREATE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = CREATE_NEW;		
		smode = FILE_SHARE_READ;
		break;
	case ACCESS_SHARED:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		smode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	case ACCESS_APPEND:
		amode = GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		append = true;
		break;
	}

	close(fs);
	if(fs.fd != INVALID_HANDLE_VALUE)
		return;

	fs.fd = CreateFile(path, amode, smode, NULL, cmode, attr, NULL);
	if(fs.fd != INVALID_HANDLE_VALUE && append)
		fs.setPosition(end);
	else
		fs.error = remapError();
	if(fs.fd != INVALID_HANDLE_VALUE)
		change(path, mode);
}

fsys::fsys(const fsys& copy)
{
	error = 0;
	fd = INVALID_HANDLE_VALUE;
	ptr = NULL;

	if(copy.fd == INVALID_HANDLE_VALUE)
		return;

	HANDLE pHandle = GetCurrentProcess();
	if(!DuplicateHandle(pHandle, copy.fd, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		fd = INVALID_HANDLE_VALUE;
		error = remapError();
	}
}

void fsys::operator=(fd_t from)
{
	HANDLE pHandle = GetCurrentProcess();

	if(fd != INVALID_HANDLE_VALUE) {
		if(!CloseHandle(fd)) {
			error = remapError();
			return;
		}
	}
	if(DuplicateHandle(pHandle, from, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		error = 0;
	else {
		fd = INVALID_HANDLE_VALUE;
		error = remapError();
	}
}

void fsys::operator=(fsys &from)
{
	HANDLE pHandle = GetCurrentProcess();

	if(fd != INVALID_HANDLE_VALUE) {
		if(!CloseHandle(fd)) {
			error = remapError();
			return;
		}
	}
	if(DuplicateHandle(pHandle, from.fd, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		error = 0;
	else {
		fd = INVALID_HANDLE_VALUE;
		error = remapError();
	}
}

void fsys::setPosition(size_t pos)
{
	DWORD rpos = pos;
	int mode = FILE_BEGIN;

	if(rpos == end) {
		rpos = 0;
		mode = FILE_END;
	}
	if(SetFilePointer(fd, rpos, NULL, mode) == INVALID_SET_FILE_POINTER)
		error = remapError();
}

#else

inline int remapError(void)
	{return errno;};

#ifdef	__PTH__

ssize_t fsys::read(void *buf, size_t len)
{
	if(ptr) {
		dirent *entry = ::readdir((DIR *)dir);	
	
		*buf = 0;
		if(!entry)
			return 0;
	
		String::set(buf, len, entry->d_name);
		return strlen(buf);
	}

	int rtn = pth_read(fd, buf, len);

	if(rtn < 0)
		error = remapError();
	return rtn;
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(ptr) {
		error = EBADF;
		return -1;
	}

	int rtn = pth_write(fd, buf, len);
	if(rtn < 0)
		error = remapError();
	return rtn;
}

#else

ssize_t fsys::read(void *buf, size_t len)
{
	if(ptr) {
		dirent *entry = ::readdir((DIR *)ptr);	
	
		if(!entry)
			return 0;
	
		String::set((char *)buf, len, entry->d_name);
		return strlen(entry->d_name);
	}

	int rtn = ::read(fd, buf, len);


	if(rtn < 0)
		error = remapError();
	return rtn;
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(ptr) {
		error = EBADF;
		return -1;
	}

	int rtn = ::write(fd, buf, len);

	if(rtn < 0)
		error = remapError();
	return rtn;
}

#endif

void fsys::close(void)
{
	error = 0;
	if(ptr) {
		if(::closedir((DIR *)ptr))
			error = remapError();
		ptr = NULL;
	} 
	else if(fd != INVALID_HANDLE_VALUE) {
		if(::close(fd) == 0)
			fd = INVALID_HANDLE_VALUE;
		else
			error = remapError();
	}
}

void fsys::create(const char *path, access_t access, unsigned mode)
{
	unsigned flags = 0;
	int rtn;

	close();

	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY | O_CREAT;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_SHARED:
	case ACCESS_REWRITE:
		flags = O_RDWR | O_CREAT;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_APPEND | O_CREAT;
		break;
	case ACCESS_DIRECTORY:
		::mkdir(path, mode);
		open(path, access);
		return; 
	}
	fd = ::open(path, flags, mode);
	if(fd == INVALID_HANDLE_VALUE)
		error = remapError();
}

int fsys::createPrefix(const char *path, unsigned mode)
{
	if(::mkdir(path, mode))
		return remapError();
	return 0;
}

void fsys::open(const char *path, access_t access)
{
	unsigned flags = 0;
	int rtn;

	close();

	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY;
		break;
	case ACCESS_SHARED:
	case ACCESS_REWRITE:
		flags = O_RDWR;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_APPEND;
		break;
	case ACCESS_DIRECTORY:
		ptr = opendir(path);
		if(!ptr)
			error = remapError();
		return; 
	}
	fd = ::open(path, flags);
	if(fd == INVALID_HANDLE_VALUE)
		error = remapError();
}

int fsys::stat(const char *path, struct stat *ino)
{
	return ::stat(path, ino);
}

int fsys::stat(struct stat *ino)
{
	int rtn = ::fstat(fd, ino);
	if(rtn)
		error = remapError();
	return rtn;
}

int fsys::setPrefix(const char *path)
{
	if(::chdir(path))
		return remapError();
	return 0;
}

char *fsys::getPrefix(char *path, size_t len)
{
	return ::getcwd(path, len);
}

int fsys::change(const char *path, unsigned mode)
{
	if(::chmod(path, mode))
		return remapError();
	return 0;
}

int fsys::access(const char *path, unsigned mode)
{
	if(::access(path, mode))
		return remapError();
	return 0;
}

fsys::fsys(const fsys& copy)
{
	fd = INVALID_HANDLE_VALUE;
	ptr = NULL;
	error = 0;

	if(copy.fd != INVALID_HANDLE_VALUE)
		fd = dup(copy.fd);
	else
		fd = INVALID_HANDLE_VALUE;
	error = 0;
	ptr = NULL;
}

void fsys::operator=(fd_t from)
{
	close();
	if(fd == INVALID_HANDLE_VALUE && from != INVALID_HANDLE_VALUE) {
		fd = dup(from);
		if(fd == INVALID_HANDLE_VALUE)
			error = remapError();
	}
}

void fsys::operator=(fsys &from)
{
	close();
	if(fd == INVALID_HANDLE_VALUE && from.fd != INVALID_HANDLE_VALUE) {
		fd = dup(from.fd);
		if(fd == INVALID_HANDLE_VALUE)
			error = remapError();
	}
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
		error = remapError();
}

#endif

fsys::fsys()
{
	fd = INVALID_HANDLE_VALUE;
	ptr = NULL;
	error = 0;
}

fsys::fsys(const char *path, access_t access)
{
	fd = INVALID_HANDLE_VALUE;
	ptr = NULL;
	error = 0;
	open(path, access);
}


fsys::fsys(const char *path, access_t access, unsigned mode)
{
	fd = INVALID_HANDLE_VALUE;
	ptr = NULL;
	error = 0;
	create(path, access, mode);
}

fsys::~fsys()
{
	close();
}

int fsys::remove(const char *path)
{
	if(::remove(path))
		return remapError();
	return 0;
}

int fsys::rename(const char *oldpath, const char *newpath)
{
	if(::rename(oldpath, newpath))
		return remapError();
	return 0;
}

#if	defined(_MSWINDOWS_)

typedef struct
{
	HANDLE dir;
	WIN32_FIND_DATA entry;
}	dnode_t;

dir_t fsys::openDir(const char *path)
{
	char tpath[256];
	DWORD attr = GetFileAttributes(path);
	dnode_t *node;

	if((attr == (DWORD)~0l) || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		return NULL;
	
	snprintf(tpath, sizeof(tpath), "%s%s", path, "\\*");
	node = new dnode_t;
	node->dir = FindFirstFile(tpath, &node->entry);
	if(node->dir == INVALID_HANDLE_VALUE) {
		delete node;
		return NULL;
	}
	return node;
}	

char *fsys::readDir(dir_t d, char *buf, size_t len)
{
	dnode_t *node = (dnode_t *)d;
	*buf = 0;

	if(!node || node->dir == INVALID_HANDLE_VALUE)
		return NULL;

	snprintf(buf, len, node->entry.cFileName);
	if(!FindNextFile(node->dir, &node->entry)) {
		FindClose(node->dir);
		node->dir = INVALID_HANDLE_VALUE;
	}
	return buf;
}
#endif

int fsys::load(const char *path)
{
	fsys module;
	int rtn;

	load(module, path);
	if(module.ptr) {
		module.ptr = 0;
		return 0;
	}
	return remapError();
}

#ifdef	_MSWINDOWS_

void fsys::load(fsys& module, const char *path)
{
	module.error = 0;
	module.ptr = (void *)LoadLibrary(path);
	if(!module.ptr)
		fsys.error = remapError();
}

void fsys::unload(fsys& module)
{
	if(module.ptr)
		FreeLibrary((HINSTANCE)module.ptr);
	module.ptr = NULL;
}

void *fsys::find(fsys& module, const char *sym)
{
	return (void *)GetProcAddress((HINSTANCE)module.ptr, sym);
}

#elif defined(HAVE_DLFCN_H) 
#include <dlfcn.h>

#ifndef	RTLD_GLOBAL
#define	RTLD_GLOBAL	0
#endif

void fsys::load(fsys& module, const char *path)
{
	module.error = 0;
	module.ptr = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	if(module.ptr)
		module.error = remapError();
}

void fsys::unload(fsys& module)
{
	if(module.ptr)
		dlclose(module.ptr);
	module.ptr = NULL;
}

void *fsys::find(fsys& module, const char *sym)
{
	if(!module.ptr)
		return NULL;

	return dlsym(module.ptr, (char *)sym);
}

#elif HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>

void fsys::load(fsys& module, const char *path)
{
	NSObjectFileImage oImage;
	NSSymbol sym = NULL;
	NSModule mod;
	void (*init)(void);

	module.ptr = NULL;
	module.error = 0;

	if(NSCreateObjectFileImageFromFile(path, &oImage) != NSObjectFileImageSuccess) {
		module.error = remapError();
		return;
	}

	mod = NSLinkModule(oImage, path, NSLINKMODULE_OPTION_BINDNOW | NSLINKMODULE_OPTION_RETURN_ON_ERROR);
	NSDestroyObjectFileImage(oImage);
	if(mod == NULL) {
		module.error = remapError();
		return;
	}

	sym = NSLookupSymbolInModule(mod, "__init");
	if(sym) {
		init = (void (*)(void))NSAddressOfSymbol(sym);
		init();
	}
	module.ptr = (void *)mod;
}

void fsys::unload(fsys& module)
{
	if(!module.ptr)
		return;

	NSModule mod = (NSModule)module.ptr;
	NSSymbol sym;
	void (*fini)(void);
	module.ptr = NULL;

	sym = NSLookupSymbolInModule(mod, "__fini");
	if(sym != NULL) {
		fini = (void (*)(void))NSAddressOfSymbol(sym);
		fini();
	}
	NSUnlinkModule(mod, NSUNLINKMODULE_OPTION_NONE);
}

void *fsys::find(fsys& module, const char *sym)
{
	if(!module.ptr)
		return NULL;

	NSModule mod = (NSModule)module.ptr;
	NSSymbol sym;

	sym = NSLookupSymbolInModule(mod, sym);
	if(sym != NULL) {
		return NSAddressOfSymbol(sym);
	
	return NULL;
}

#elif HAVE_SHL_LOAD
#include <dl.h>

void fsys::load(fsys& module, const char *path)
{
	module.error = 0;
	module.ptr = (void *))shl_load(path, BIND_IMMEDIATE, 0l);
	if(!module.ptr)
		module.error = remapError();
}

void *fsys::find(fsys& module, const char *sym)
{
	shl_t image = (shl_t)module.ptr;

	if(shl_findsym(&image, sym, 0, &value) == 0)
		return (void *)value;

	return NULL;
}

void fsys::unload(fsys& module)
{
	shl_t image = (shl_t)module.ptr;
	if(addr)
		shl_unload(image);
	module.ptr = NULL;
}

#else

mem_t fsys::load(const char *path)
{
	return NULL;
}

void fsys::unload(mem_t addr)
{
}

void *fsys::find(mem_t addr, const char *sym)
{
	return NULL;
}

#endif
