// Copyright (C) 2010 David Sugar, Tycho Softworks.
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

#include <config.h>
#include <ucommon/buffer.h>
#include <ucommon/string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace UCOMMON_NAMESPACE;

IOBuffer::IOBuffer()
{
	end = true;
	eol = "\r\n";
	input = output = buffer = NULL;
	timeout = Timer::inf;
}

IOBuffer::IOBuffer(size_t size, type_t mode)
{
	end = true;
	eol = "\r\n";
	input = output = buffer = NULL;
	timeout = Timer::inf;
	allocate(size, mode);
}

IOBuffer::~IOBuffer()
{
	release();
}

void IOBuffer::release(void)
{
	if(buffer) {
		flush();
		free(buffer);
		input = output = buffer = NULL;
		timeout = Timer::inf;
		end = true;
	}
}

void IOBuffer::allocate(size_t size, type_t mode)
{
	release();
	ioerror = 0;

	if(!size)
		return;

	switch(mode) {
	case BUF_RDWR:
		input = buffer = (char *)malloc(size * 2);
		if(buffer)
			output = buffer + size;
		break;
	case BUF_RD:
		input = buffer = (char *)malloc(size);
		break;
	case BUF_WR:
		output = buffer = (char *)malloc(size);
		break;
	}

	bufpos = insize = outsize = 0;
	bufsize = size;

	if(buffer)
		end = false;
}

size_t IOBuffer::getstr(char *address, size_t size)
{
	size_t count = 0;

	if(!input || !address)
		return 0;

	while(count < size) {
		if(bufpos == insize) {
			if(end)
				return count;

			insize = _pull(input, bufsize);
			bufpos = 0;
			if(insize == 0)
				end = true;
			else if(insize < bufsize && !timeout)
				end = true;

			if(!insize)
				return count;
		}
		address[count++] = input[bufpos++];
	}
	return count;
}

int IOBuffer::getch(void)
{
	if(!input)
		return EOF;

	if(bufpos == insize) {
		if(end)
			return EOF;

		insize = _pull(input, bufsize);
		bufpos = 0;
		if(insize == 0)
			end = true;
		else if(insize < bufsize && !timeout)
			end = true;

		if(!insize)
			return EOF;
	}

	return input[bufpos++];
}

size_t IOBuffer::putstr(const char *address, size_t size)
{
	size_t count = 0;

	if(!output || !address)
		return 0;

	if(!size)
		size = strlen(address);

	while(count < size) {
		if(outsize == bufsize) {
			outsize = 0;
			if(_push(output, bufsize) < bufsize) {
				output = NULL;
				end = true;		// marks a disconnection...
				return count;
			}
		}
		
		output[outsize++] = address[count++];
	}

	return count;
}

int IOBuffer::putch(int ch)
{
	if(!output)
		return EOF;

	if(outsize == bufsize) {
		outsize = 0;
		if(_push(output, bufsize) < bufsize) {
			output = NULL;
			end = true;		// marks a disconnection...
			return EOF;
		}
	}
	
	output[outsize++] = ch;
	return ch;
}

size_t IOBuffer::printf(const char *format, ...)
{
	va_list args;
	int result;
	size_t count;

	if(!flush() || !output || !format)
		return 0;

	va_start(args, format);
	result = vsnprintf(output, bufsize, format, args);
	va_end(args);
	if(result < 1)
		return 0;

	if((size_t)result > bufsize)
		result = bufsize;

	count = _push(output, result);
	if(count < (size_t)result) {
		output = NULL;
		end = true;
	}

	return count;
}

void IOBuffer::purge(void)
{
	outsize = insize = bufpos = 0;
}

bool IOBuffer::flush(void)
{
	if(!output)
		return false;

	if(!outsize)
		return true;
	
	if(_push(output, outsize) == outsize) {
		outsize = 0;
		return true;
	}
	output = NULL;
	end = true;
	return false;
}

char *IOBuffer::gather(size_t size)
{
	if(!input || size > bufsize)
		return NULL;

	if(size + bufpos > insize) {
		if(end)
			return NULL;
	
		size_t adjust = outsize - bufpos;
		memmove(input, input + bufpos, adjust);
		insize = adjust +  _pull(input, bufsize - adjust);
		bufpos = 0;

		if(insize < bufsize)
			end = true;
	}

	if(size + bufpos <= insize) {
		char *bp = input + bufpos;
		bufpos += size;
		return bp;
	}

	return NULL;	
}

char *IOBuffer::request(size_t size)
{
	if(!output || size > bufsize)
		return NULL;

	if(size + outsize > bufsize)
		flush();

	size_t offset = outsize;
	outsize += size;
	return output + offset;
}	

size_t IOBuffer::putline(const char *string)
{
	size_t count = 0;

	if(string)
		count += putstr(string);

	if(eol)
		count += putstr(eol);

	return count;
}

size_t IOBuffer::getline(char *string, size_t size)
{
	size_t count = 0;
	unsigned eolp = 0;
	const char *eols = eol;
	
	if(!eols)
		eols="\0";

	if(string)
		string[0] = 0;

	if(!input || !string)
		return 0;

	while(count < size - 1) {
		int ch = getch();
		if(ch == EOF) {
			eolp = 0;
			break;
		}

		string[count++] = ch;

		if(ch == eols[eolp]) {
			++eolp;
			if(eols[eolp] == 0)
				break;
		}
		else
			eolp = 0;	

		// special case for \r\n can also be just eol as \n 
		if(eq(eol, "\r\n") && ch == '\n') {
			++eolp;
			break;
		}
	}
	count -= eolp;
	string[count] = 0;
	return count;
}

void IOBuffer::reset(void)
{
	insize = bufpos = 0;
}

bool IOBuffer::eof(void)
{
	if(!input)
		return true;

	if(end && bufpos == insize)
		return true;

	return false;
}
	
size_t IOBuffer::_push(const char *address, size_t size)
{
	return 0;
}

size_t IOBuffer::_pull(char *address, size_t size)
{
	return 0;
}

bool IOBuffer::pending(void)
{
	if(!input)
		return false;

	if(!bufpos)
		return false;

	return true;
}

fbuf::fbuf() : IOBuffer(), fsys()
{
	timeout = 0;
}

fbuf::fbuf(const char *path, access_t access, size_t size)
{
	timeout = 0;
	open(path, access, size);
}

fbuf::fbuf(const char *path, access_t access, unsigned mode, size_t size)
{
	timeout = 0;
	create(path, access, mode, size);
}

fbuf::~fbuf()
{
	fbuf::close();
}

void fbuf::open(const char *path, access_t mode, size_t size)
{
	fbuf::close();
	if(mode != ACCESS_DIRECTORY)
		fsys::open(path, mode);
	if(getfile() == INVALID_HANDLE_VALUE)
		return;

	inpos = outpos = 0;

	switch(mode) {
	case ACCESS_RDONLY:
		allocate(size, BUF_RD);
		break;
	case ACCESS_STREAM:
	case ACCESS_WRONLY:
		allocate(size, BUF_WR);
		break;
	case ACCESS_RANDOM:
	case ACCESS_SHARED:
	case ACCESS_REWRITE:
		allocate(size, BUF_RDWR);
		break;
	case ACCESS_APPEND:
		outpos = fsys::end;
		allocate(size, BUF_WR);
	default:
		break;
	}
}

void fbuf::create(const char *path, access_t mode, unsigned cmode, size_t size)
{
	fbuf::close();
	if(mode != ACCESS_DIRECTORY)
		fsys::create(path, mode, cmode);
	if(getfile() == INVALID_HANDLE_VALUE)
		return;

	inpos = outpos = 0;

	switch(mode) {
	case ACCESS_RDONLY:
		allocate(size, BUF_RD);
		break;
	case ACCESS_STREAM:
	case ACCESS_WRONLY:
		allocate(size, BUF_WR);
		break;
	case ACCESS_RANDOM:
	case ACCESS_SHARED:
	case ACCESS_REWRITE:
		allocate(size, BUF_RDWR);
		break;
	case ACCESS_APPEND:
		outpos = fsys::end;
		allocate(size, BUF_WR);
	default:
		break;
	}
}
		
void fbuf::close()
{
	IOBuffer::release();
	fsys::close();
}

fsys::offset_t fbuf::tell(void)
{
	if(!fbuf::isopen())
		return 0;

	if(isinput())
		return inpos + _pending();

	if(outpos == fsys::end)
		return fsys::end;

	return outpos + _waiting();
}

bool fbuf::trunc(offset_t offset)
{
	int seekerr;

	if(!fbuf::isopen())
		return false;

	clear();
	reset();
	flush();

	seekerr = trunc(offset);
	if(seekerr)
		ioerror = seekerr;
	else
		inpos = outpos = offset;
	
	if(err())
		return false;

	return true;
}

bool fbuf::seek(offset_t offset)
{
	int seekerr;

	if(!fbuf::isopen())
		return false;

	clear();
	reset();
	flush();

	seekerr = seek(offset);
	if(seekerr)
		ioerror = seekerr;
	else
		inpos = outpos = offset;

	if(err())
		return false;

	return true;
}
	
size_t fbuf::_push(const char *buf, size_t size)
{
	ssize_t result;

	if(outpos == fsys::end) {
		result = fsys::write(buf, size);
		if(result < 0) {
			ioerror = error;
			result = 0;
		}
		return (size_t) result;
	}

#ifdef	HAVE_PWRITE
	result = pwrite(getfile(), buf, size, outpos);
	if(result < 0) {
		result = 0;
		ioerror = error;
	}
	outpos += result;
	return (size_t)result;
#else
	int seekerr;

	if(isinput()) {
		// if read & write separate threads, protect i/o reposition
		mutex::protect(this);
		seekerr = fsys::seek(outpos);
		if(seekerr) {
			mutex::release(this);
			ioerror = seekerr;
			return 0;
		}
	}

	result = fsys::write(buf, size);

	if(isinput()) {
		seekerr = fsys::seek(inpos);
		mutex::release(this);
		if(result >= 0 && seekerr) {
			ioerror = seekerr;
			seteof();
		}
	}

	if(result < 0) {
		result = 0;
		ioerror = error;
	}
	outpos += result;
	return (size_t)result;
#endif
}

size_t fbuf::_pull(char *buf, size_t size)
{
	ssize_t result;

#ifdef	HAVE_PWRITE
	if(isoutput()) 
		result = pread(getfile(), buf, size, inpos);
	else
		result = fsys::read(buf, size);
#else

	if(isoutput())
		mutex::protect(this);

	result = fsys::read(buf, size);

	if(isoutput())
		mutex::release(this);
#endif

	if(result < 0) {
		result = 0;
		ioerror = error;
	}
	inpos += result;
	return (size_t)result;
}

TCPServer::TCPServer(const char *service, const char *address, unsigned backlog, int protocol) :
ListenSocket(address, service, backlog, protocol)
{
	servicetag = service;
}

TCPSocket::TCPSocket(const char *service) : IOBuffer()
{
	so = INVALID_SOCKET;
	timeout = Timer::inf;
	String::set(serviceid, sizeof(serviceid), service);
	servicetag = service;	// default tag for new connections...
}

TCPSocket::TCPSocket(const char *service, const char *host, size_t size) :
IOBuffer()
{
	so = INVALID_SOCKET;
	timeout = Timer::inf;
	String::set(serviceid, sizeof(serviceid), service);
	servicetag = service;
	open(host, size);
}

TCPSocket::TCPSocket(TCPServer *server, size_t size) :
IOBuffer()
{
	String::set(serviceid, sizeof(serviceid), "0");
	servicetag = serviceid;
	so = INVALID_SOCKET;
	open(server, size);
}

TCPSocket::~TCPSocket()
{
	TCPSocket::close();
}

void TCPSocket::open(const char *host, size_t size)
{
	struct sockaddr_storage address;
	socklen_t alen = sizeof(address);
	struct addrinfo *list = Socket::getaddress(host, servicetag, SOCK_STREAM, 0);
	if(!list)
		return;

	so = Socket::create(list, SOCK_STREAM, 0);
	Socket::release(list);
	if(so == INVALID_SOCKET) {
		ioerror = errno;
		return;
	}

	if(getpeername(so, (struct sockaddr *)&address, &alen) == 0)
		snprintf(serviceid, sizeof(serviceid), "%u", Socket::getservice((struct sockaddr *)&address));

	_buffer(size);
}

void TCPSocket::open(TCPServer *server, size_t size)
{
	close();
	so = server->accept();
	if(so == INVALID_SOCKET) {
		ioerror = errno;
		return;
	}
	
	struct sockaddr_storage address;
	socklen_t alen = sizeof(address);

	servicetag = server->servicetag;

	if(getsockname(server->getsocket(), (struct sockaddr *)&address, &alen) == 0)
		snprintf(serviceid, sizeof(serviceid), "%u", Socket::getservice((struct sockaddr *)&address));	

	_buffer(size);
}

void TCPSocket::close(void)
{
	if(so == INVALID_SOCKET)
		return;

	IOBuffer::release();
	Socket::release(so);
	so = INVALID_SOCKET;
}

void TCPSocket::blocking(timeout_t timer)
{
	timeout = timer;
	if(timeout == Timer::inf)
		Socket::blocking(so, true);
	else
		Socket::blocking(so, false);
}

void TCPSocket::_buffer(size_t size)
{
	unsigned iobuf = 0;
	unsigned mss = size;
	unsigned max = 0;

	timeout = Timer::inf;
#ifdef	TCP_MAXSEG
	socklen_t alen = sizeof(max);
#endif

	if(size < 80) {
		allocate(size, BUF_RDWR);
		return;
	}

#ifdef	TCP_MAXSEG
	if(mss)
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, sizeof(max));
	getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, &alen);
#endif

	if(max && max < mss)
		mss = max;

	if(!mss) {
		if(max)
			mss = max;
		else
			mss = 536;
		goto alloc;
	}

	if(mss < 80)
		mss = 80;

	if(mss * 7 < 64000u)
		iobuf = mss * 7;
	else if(size * 6 < 64000u)
		iobuf = mss * 6;
	else
		iobuf = mss * 5;

	Socket::sendsize(so, iobuf);
	Socket::recvsize(so, iobuf);

	if(mss < 512)
		Socket::sendwait(so, mss * 4);

alloc:
	allocate(size, BUF_RDWR);
}

size_t TCPSocket::_push(const char *address, size_t len)
{
	ssize_t result = Socket::sendto(so, address, len);
	if(result < 0) {
		result = 0;
		ioerror = errno;
	}
	return (size_t)result;
}

size_t TCPSocket::_pull(char *address, size_t len)
{
	ssize_t result;

	if((timeout && timeout != Timer::inf) && !Socket::wait(so, timeout))
		return 0;

	result = Socket::recvfrom(so, address, len, 0);

	if(result < 0) {
		result = 0;
		ioerror = errno;
	}
	return (size_t)result;
}

size_t TCPSocket::peek(char *data, size_t size, timeout_t timeout)
{
	Socket::wait(timeout);
	if(!Socket::wait(timeout))
		return 0;

	return Socket::peek(data, size);
}

bool TCPSocket::pending(void)
{
	if(_pending())
		return true;
	
	if(isinput())
		return Socket::wait(so, timeout);

	return false;
}
