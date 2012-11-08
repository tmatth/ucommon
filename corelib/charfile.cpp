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

#ifndef _MSC_VER
#include <sys/stat.h>
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/thread.h>
#include <ucommon/fsys.h>
#include <ucommon/string.h>
#include <ucommon/memory.h>
#include <ucommon/shell.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

charfile cstdin(stdin);
charfile cstdout(stdout);
charfile cstderr(stderr);

charfile::charfile(const char *file, const char *mode)
{
    fp = NULL;
    pid = INVALID_PID_VALUE;
    tmp = NULL;
    open(file, mode);
}

charfile::charfile(const char *file, char **argv, const char *mode, char **envp)
{
    fp = NULL;
    pid = INVALID_PID_VALUE;
    tmp = NULL;
    open(file, argv, mode, envp);
}

charfile::charfile()
{
    fp = NULL;
    tmp = NULL;
    pid = INVALID_PID_VALUE;
}

charfile::charfile(FILE *file)
{
    fp = file;
    tmp = NULL;
    pid = INVALID_PID_VALUE;
}

charfile::~charfile()
{
    close();
}

bool charfile::is_tty(void) const
{
#ifdef  _MSWINDOWS_
    if(_isatty(_fileno(fp)))
        return true;
#else
    if(isatty(fileno(fp)))
        return true;
#endif
    return false;
}

void charfile::open(const char *path, char **argv, const char *mode, char **envp)
{
    close();
    fd_t fd;
    fd_t stdio[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};

#ifdef  _MSWINDOWS_
    int _fmode = 0;
#endif

    if(eq_case(mode, "w")) {
        if(fsys::pipe(fd, stdio[0]))
            return;
        fsys::inherit(fd, false);
        pid = shell::spawn(path, argv, envp, stdio);
        if(pid == INVALID_PID_VALUE) {
            fsys::release(stdio[0]);
            fsys::release(fd);
            return;
        }
        fsys::release(stdio[0]);
#ifdef  _MSWINDOWS_
        _fmode = _O_WRONLY;
#endif
    }
    else if(eq_case(mode, "r")) {
        if(fsys::pipe(stdio[1], fd))
            return;
        fsys::inherit(fd, false);
        pid = shell::spawn(path, argv, envp, stdio);
        if(pid == INVALID_PID_VALUE) {
            fsys::release(stdio[1]);
            fsys::release(fd);
            return;
        }
        fsys::release(stdio[1]);
#ifdef  _MSWINDOWS_
        _fmode = _O_RDONLY;
#endif
    }
    else if(eq_case(mode, "r+") || eq_case(mode, "rw")) {
#if defined(HAVE_SOCKETPAIR)
        int pair[2];
        if(socketpair(AF_LOCAL, SOCK_STREAM, 0, pair))
            return;

        fd = pair[0];
        stdio[0] = stdio[1] = pair[1];
        fsys::inherit(fd, false);
        pid = shell::spawn(path, argv, envp, stdio);
        if(pid == INVALID_PID_VALUE) {
            fsys::release(pair[1]);
            fsys::release(fd);
            return;
        }
        fsys::release(pair[1]);
#elif defined(_MSWINDOWS_)
        static int count;
        char buf[96];

        snprintf(buf, sizeof(buf), "\\\\.\\pipe\\pair-%ld-%d",
            GetCurrentProcessId(), count++);
        fd = CreateNamedPipe(buf, PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_NOWAIT,
            PIPE_UNLIMITED_INSTANCES, 1024, 1024, 0, NULL);
        if(fd == INVALID_HANDLE_VALUE)
            return;
        fd_t child = CreateFile(buf, GENERIC_READ|GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(child == INVALID_HANDLE_VALUE) {
            CloseHandle(fd);
            ::remove(buf);
            return;
        }

        fsys::inherit(child, true);
        fsys::inherit(fd, false);

        DWORD pmode = PIPE_NOWAIT;

        SetNamedPipeHandleState(fd, &pmode, NULL, NULL);
        stdio[0] = child;
        stdio[1] = child;
        pid = shell::spawn(path, argv, envp, stdio);
        if(pid == INVALID_PID_VALUE) {
            CloseHandle(child);
            CloseHandle(fd);
            ::remove(buf);
            return;
        }
        CloseHandle(child);
        tmp = strdup(buf);
#endif
    }
    else
        return;

#ifdef  _MSWINDOWS_
    int fdd = _open_osfhandle((intptr_t)fd, _fmode);
    fp = _fdopen(fdd, mode);
#else
    fp = fdopen(fd, mode);
#endif
    if(!fp)
        fsys::release(fd);
}

void charfile::open(const char *path, const char *mode)
{
    if(fp)
        fclose(fp);

    fp = fopen(path, mode);
}

int charfile::cancel(void)
{
    int result = 0;
    if(pid != INVALID_PID_VALUE)
        result = shell::cancel(pid);
    pid = INVALID_PID_VALUE;
    close();
    return result;
}

int charfile::close(void)
{
    int error = 0;
    if(pid != INVALID_PID_VALUE)
        error = shell::wait(pid);

    if(tmp) {
        ::remove(tmp);
        free(tmp);
        tmp = NULL;
    }

    if(fp)
        fclose(fp);
    fp = NULL;
    pid = INVALID_PID_VALUE;
    return error;
}

size_t charfile::scanf(const char *format, ...)
{
    if(!fp)
        return 0;

    va_list args;
    va_start(args, format);
    size_t result = vfscanf(fp, format, args);
    va_end(args);
    if(result == (size_t)EOF)
        result = 0;
    return result;
}

size_t charfile::printf(const char *format, ...)
{
    if(!fp)
        return 0;

    va_list args;
    va_start(args, format);
    size_t result = vfprintf(fp, format, args);
    va_end(args);
    if(result == (size_t)EOF)
        result = 0;
    return result;
}

size_t charfile::put(const char *data)
{
    if(!fp)
        return 0;

    int result = fputs(data, fp);
    if(result < 0)
        result = 0;

    return (size_t)result;
}

size_t charfile::readline(char *address, size_t size)
{
    address[0] = 0;

    if(!fp)
        return 0;

    if(!fgets(address, size, fp) || feof(fp))
        return 0;

    size_t result = size = strlen(address);

    if(address[size - 1] == '\n') {
        --size;
        if(size && address[size - 1] == '\r')
            --size;
    }
    address[size] = 0;
    return result;
}

size_t charfile::readline(String& s)
{
    if(!s.c_mem())
        return true;

    if(!fgets(s.c_mem(), s.size(), fp) || feof(fp)) {
        s.clear();
        return false;
    }

    String::fix(s);
    size_t result = s.len();

    if(s[-1] == '\n')
        --s;

    if(s[-1] == '\r')
        --s;

    return result;
}

bool charfile::eof(void) const
{
    if(!fp)
        return false;

    return feof(fp) != 0;
}

int charfile::err(void) const
{
    if(!fp)
        return EBADF;

    return ferror(fp);
}

int charfile::_getch(void)
{
    if(!fp)
        return EOF;

    return fgetc(fp);
}

int charfile::_putch(int code)
{
    if(!fp)
        return EOF;

    return fputc(code, fp);
}

size_t charfile::load(stringlist_t *list, size_t count)
{
    if(!list || !fp)
        return 0;

    size_t used = 0;
    size_t size = list->size() - 64;

    char *tmp = (char *)malloc(size);
    while(!count || used < count) {
        if(feof(fp))
            break;

        if(!readline(tmp, size))
            break;

        ++used;
        list->add(tmp);
    }
    free(tmp);
    return used;
}

size_t charfile::save(const stringlist_t *list, size_t count)
{
    size_t used = 0;
    if(!list || !fp)
        return 0;

    StringPager::iterator sp = list->begin();
    while(is(sp) && (!count || used < count)) {
        if(fprintf(fp, "%s\n", sp->get()) < 1)
            break;
        ++used;
        sp.next();
    }
    return used;
}

String str(charfile& so, strsize_t size)
{
    String s(size);
    so.readline(s.c_mem(), s.size());
    String::fix(s);
    return s;
}

