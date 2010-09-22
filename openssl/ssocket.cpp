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
	if(ssl)	{
		SSL_free((SSL *)ssl);
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
	context *ctx = (context *)scontext;
	release();
	so = tcp->accept();
	if(so == INVALID_SOCKET) {
		ioerr = Socket::error();
		return;
	}

	if(ctx && ctx->ctx && ctx->err() == secure::OK)
		ssl = SSL_new(ctx->ctx);

	if(!ssl)
		return;

	SSL_set_fd((SSL *)ssl, so);
	
	if(SSL_accept((SSL *)ssl) > 0) {
		wbio = SSL_get_wbio((SSL *)ssl);
		rbio = SSL_get_rbio((SSL *)ssl);
	}
}

void SSocket::connect(const char *host, const char *service, secure::client_t scontext)
{
	context *ctx = (context *)scontext;

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

	if(ctx && ctx->ctx && ctx->err() == secure::OK)
		ssl = SSL_new(ctx->ctx);

	if(!ssl)
		return;

	SSL_set_fd((SSL *)ssl, getsocket());

	if(SSL_connect((SSL *)ssl) > 0) {
		wbio = SSL_get_wbio((SSL *)ssl);
		rbio = SSL_get_wbio((SSL *)ssl);
	}
}

size_t SSocket::read(char *buffer, size_t size)
{
	ssize_t result;

	if(so == INVALID_SOCKET)
		return 0;

	if(!rbio)
		result = ::recv(so, buffer, size, 0);
	else
		result = BIO_read((BIO *)rbio, buffer, size);
	
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
		result = BIO_write((BIO *)wbio, buffer, 0);
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
		unsigned nstat = 0;
		unsigned nleft;
		memset(data, 0, max);
		switch(BIO_gets((BIO *)rbio, data + nstat, max - nstat - 1)) {
		case -2:
			ioerr = EBADF;
			return 0;
		case -1:
			ioerr = Socket::error();
			return 0;
		case 0:
			break;
		default:
			nstat = 0;
			while(data[nstat]) {
				if(data[nstat] == '\n') {
					nleft = nstat;
					if(data[nleft - 1] == '\r')
						--nleft;
					data[nleft] = 0;
					return nleft;
				}
				++nstat;
			}
		}
		return nstat;
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

