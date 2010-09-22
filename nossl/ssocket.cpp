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

#include "local.h"

SSocket::SSocket()
{
	ssl = NULL;
	rbio = wbio = NULL;
	so = INVALID_SOCKET;
	ioerr = 0;
}

SSocket::SSocket(const TCPServer *tcp, secure::server_t context)
{
	ssl = NULL;
	rbio = wbio = NULL;
	so = INVALID_SOCKET;
	ioerr = 0;

	accept(tcp, context);
}

SSocket::SSocket(const char *host, const char *service, secure::client_t context)
{
	so = INVALID_SOCKET;
	ssl = NULL;
	rbio = wbio = NULL;
	ioerr = 0;

	connect(host, service, context);
}

SSocket::~SSocket()
{
	release();
}

void SSocket::release(void)
{
	if(so != INVALID_SOCKET)
		Socket::release(so);

	so = INVALID_SOCKET;
	ssl = NULL;
	rbio = wbio = NULL;
	ioerr = 0;
}

void SSocket::accept(const TCPServer *tcp, secure::server_t context)
{
	release();
	so = tcp->accept();
	if(so == INVALID_SOCKET)
		ioerr = Socket::error();
}

void SSocket::connect(const char *host, const char *service, secure::client_t context)
{
	release();
	struct addrinfo *list = Socket::getaddress(host, service, SOCK_STREAM, 0);
	if(!list)
		return;

	so = Socket::create(list, SOCK_STREAM, 0);
	Socket::release(list);

	if(so == INVALID_SOCKET)
		ioerr = Socket::error();
}

size_t SSocket::read(char *buffer, size_t size)
{
	if(so == INVALID_SOCKET)
		return 0;

	ssize_t result = Socket::recvfrom(so, buffer, size);
	if(result < 0) {
		ioerr = Socket::error();
		return 0;
	}
	return (size_t)result;
}

size_t SSocket::write(const char *buffer, size_t size)
{
	if(so == INVALID_SOCKET)
		return 0;
	ssize_t result = Socket::sendto(so, buffer, size);
	if(result < 0) {
		ioerr = Socket::error();
		return 0;
	}
	return (size_t)result;
}

size_t SSocket::readline(char *data, size_t max)
{
    assert(data != NULL);
    assert(max > 0);

    *data = 0;

	if(so == INVALID_SOCKET)
		return 0;

    ssize_t result = Socket::readline(so, data, max);
    if(result < 0) {
        ioerr = Socket::error();
        return 0;
    }
    return (size_t)result;
}

size_t SSocket::readline(string& s)
{
	if(!s.c_mem())
		return 0;

	size_t result = Socket::readline(so, s.c_mem(), s.size() + 1);
    String::fix(s);
    return (size_t)result;
}

size_t SSocket::printf(const char *format, ...)
{
    assert(format != NULL);

    char buf[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    return writes(buf);
}

int SSocket::_getch(void)
{
	char buf;

	if(read(&buf, 1) < 1)
		return EOF;

	return buf;
}

int SSocket::_putch(int code)
{
    char buf = code;

    if(write(&buf, 1) < 1)
        return EOF;

    return buf;
}

