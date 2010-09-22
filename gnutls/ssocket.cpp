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
	rbio = wbio = 0;
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
	rbio = wbio = NULL;
	ssl = NULL;
	ioerr = 0;

	connect(host, service, context);
}

SSocket::~SSocket()
{
	release();
}

void SSocket::release(void)
{
	if(ssl) {
		gnutls_deinit((SSL)ssl);
		ssl = NULL;
	}

	if(so != INVALID_SOCKET) {
		Socket::release(so);
		so = INVALID_SOCKET;
	}

	ssl = NULL;
	rbio = wbio = NULL;
	ioerr = 0;
}

void SSocket::accept(const TCPServer *tcp, secure::server_t scontext)
{
	release();
	so = tcp->accept();
	if(so == INVALID_SOCKET) {
		ioerr = Socket::error();
		return;
	}

	ssl = context::session((context *)scontext);
	if(!ssl)
		return;

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		rbio = wbio = ssl;
}

void SSocket::connect(const char *host, const char *service, secure::client_t scontext)
{
	release();
	struct addrinfo *list = Socket::getaddress(host, service, SOCK_STREAM, 0);
	if(!list)
		return;

	so = Socket::create(list, SOCK_STREAM, 0);
	Socket::release(list);

	if(so == INVALID_SOCKET) {
		ioerr = Socket::error();
		return;
	}

	ssl = context::session((context *)scontext);
	if(!ssl)
		return;

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		rbio = wbio = ssl;
}

size_t SSocket::read(char *buffer, size_t size)
{
	ssize_t result;

	if(so == INVALID_SOCKET)
		return 0;

	if(!rbio)
		result = ::recv(so, buffer, size, 0);
	else
		result = gnutls_record_recv((SSL)rbio, buffer, size);
	
	if(result < 0) {
		ioerr = Socket::error();
		return 0;
	}
	return (size_t)result;
}

size_t SSocket::write(const char *buffer, size_t size)
{
	ssize_t result;

	if(so == INVALID_SOCKET)
		return 0;

	if(wbio)
		result = gnutls_record_send((SSL)wbio, buffer, 0);
	else
		result = ::send(so, buffer, size, 0);

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

	if(max < 1) 
		return 0;

	if(rbio) {
		size_t pos = 0;
		while(pos < max - 1) {
			ssize_t result = read(data + pos, 1);
			if(result < 0) {
				data[pos] = 0;
				return 0;
			}
			if(result == 0) {
				data[pos] = 0;
				return pos;
			}
			
			if(data[pos] == '\n') {
				size_t count = pos + 1;
				if(pos && data[pos - 1] == '\r')
					--pos;
				data[pos] = 0;
				return count;
			}
		}
		data[pos] = 0;
		return pos;
	}

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

    ssize_t result = readline(s.c_mem(), s.size() + 1);
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

