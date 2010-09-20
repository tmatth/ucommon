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

using namespace UCOMMON_NAMESPACE;

fbuf::fbuf() : BufferProtocol(), fsys()
{
}

fbuf::fbuf(const char *path, access_t access, size_t size)
{
	open(path, access, size);
}

fbuf::fbuf(const char *path, access_t access, unsigned mode, size_t size)
{
	create(path, access, mode, size);
}

fbuf::~fbuf()
{
	fbuf::close();
}

int fbuf::_err(void)
{
	return error;
}

void fbuf::_clear(void)
{
	error = 0;
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
	BufferProtocol::release();
	fsys::close();
}

fsys::offset_t fbuf::tell(void)
{
	if(!fbuf::isopen())
		return 0;

	if(isinput())
		return inpos + unread();

	if(outpos == fsys::end)
		return fsys::end;

	return outpos + unsaved();
}

bool fbuf::trunc(offset_t offset)
{
	int seekerr;

	if(!fbuf::isopen())
		return false;

	_clear();
	reset();
	flush();

	seekerr = trunc(offset);
	if(!seekerr)
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

	_clear();
	reset();
	flush();

	seekerr = seek(offset);
	if(!seekerr)
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
		if(result < 0)
			result = 0;

		return (size_t) result;
	}

#ifdef	HAVE_PWRITE
	result = pwrite(getfile(), buf, size, outpos);
	if(result < 0) 
		result = 0;
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

	if(result < 0)
		result = 0;
	inpos += result;
	return (size_t)result;
}

TCPServer::TCPServer(const char *service, const char *address, unsigned backlog, int protocol) :
ListenSocket(address, service, backlog, protocol)
{
	servicetag = service;
}

TCPSocket::TCPSocket(const char *service) : BufferProtocol()
{
	so = INVALID_SOCKET;
	String::set(serviceid, sizeof(serviceid), service);
	servicetag = service;	// default tag for new connections...
}

TCPSocket::TCPSocket(const char *service, const char *host, size_t size) :
BufferProtocol()
{
	so = INVALID_SOCKET;
	String::set(serviceid, sizeof(serviceid), service);
	servicetag = service;
	open(host, size);
}

TCPSocket::TCPSocket(TCPServer *server, size_t size) :
BufferProtocol()
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
	if(so == INVALID_SOCKET)
		return;

	if(getpeername(so, (struct sockaddr *)&address, &alen) == 0)
		snprintf(serviceid, sizeof(serviceid), "%u", Socket::getservice((struct sockaddr *)&address));

	_buffer(size);
}

void TCPSocket::open(TCPServer *server, size_t size)
{
	close();
	so = server->accept();
	if(so == INVALID_SOCKET)
		return;
	
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

	BufferProtocol::release();
	Socket::release(so);
	so = INVALID_SOCKET;
}

void TCPSocket::_buffer(size_t size)
{
	unsigned iobuf = 0;
	unsigned mss = size;
	unsigned max = 0;

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

int TCPSocket::_err(void)
{
	return ioerr;
}

void TCPSocket::_clear(void)
{
	ioerr = 0;
}

bool TCPSocket::_blocking(void)
{
	if(iowait)
		return true;
	
	return false;
}

size_t TCPSocket::_push(const char *address, size_t len)
{
	if(ioerr)
		return 0;

	ssize_t result = writeto(address, len);
	if(result < 0)
		result = 0;

	return (size_t)result;
}

size_t TCPSocket::_pull(char *address, size_t len)
{
	ssize_t result;

	result = readfrom(address, len);

	if(result < 0)
		result = 0;
	return (size_t)result;
}

bool TCPSocket::_pending(void)
{
	if(unread())
		return true;
	
	if(isinput() && iowait && iowait != Timer::inf)
		return Socket::wait(so, iowait);

	return Socket::wait(so, 0);
}
