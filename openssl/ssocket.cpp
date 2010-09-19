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

SSocket::SSocket(const char *service, secure::context_t scontext) :
TCPSocket(service)
{
	context *ctx = (context *)scontext;
	ssl = NULL;
	bio = NULL;

	if(ctx && ctx->ctx && ctx->err() == secure::OK)
		ssl = SSL_new(ctx->ctx);
}

SSocket::SSocket(TCPServer *server, secure::context_t scontext, size_t size) :
TCPSocket(server, size)
{
	context *ctx = (context *)scontext;
	ssl = NULL;
	bio = NULL;
	
	if(ctx && ctx->ctx && ctx->err() == secure::OK)
		ssl = SSL_new(ctx->ctx);

	if(!isopen() || !ssl)
		return;

	SSL_set_fd((SSL *)ssl, getsocket());
	
	if(SSL_accept((SSL *)ssl) > 0)
		bio = SSL_get_wbio((SSL *)ssl);
}
	
SSocket::~SSocket()
{
	release();
}

void SSocket::open(TCPServer *server, size_t bufsize)
{
	close();
	TCPSocket::open(server, bufsize);

	if(!isopen() || !ssl)
		return;

	SSL_set_fd((SSL *)ssl, getsocket());
	
	if(SSL_accept((SSL *)ssl) > 0)
		bio = SSL_get_wbio((SSL *)ssl);
}

void SSocket::open(const char *host, size_t bufsize)
{
	close();
	TCPSocket::open(host, bufsize);

	if(!isopen() || !ssl)
		return;

	SSL_set_fd((SSL *)ssl, getsocket());

	if(SSL_connect((SSL *)ssl) > 0)
		bio = SSL_get_wbio((SSL *)ssl);
}	

void SSocket::close(void)
{
	if(bio) {
		SSL_shutdown((SSL *)ssl);
		bio = NULL;
	}

	TCPSocket::close();
}

void SSocket::release(void)
{
	close();
	if(ssl)	{
		SSL_free((SSL *)ssl);
		ssl = NULL;
	}
}

size_t SSocket::_push(const char *address, size_t size)
{
	if(!bio)
		return TCPSocket::_push(address, size);

	int result = SSL_write((SSL *)ssl, address, size);
	if(result < 0) {
		result = 0;
		ioerr = Socket::error();
	}
	return (ssize_t)result;
}

bool SSocket::pending(void)
{
	if(so == INVALID_SOCKET)
		return false;

	if(_pending())
		return true;

	if(ssl && SSL_pending((SSL *)ssl))
		return true;

	if(iowait && iowait != Timer::inf)
		return Socket::wait(so, iowait);

	return Socket::wait(so, 0);
}

size_t SSocket::_pull(char *address, size_t size)
{
	if(!bio)
		return TCPSocket::_pull(address, size);

	if(SSL_pending((SSL *)ssl) == 0 && iowait && iowait != Timer::inf && !Socket::wait(so, iowait))
		return 0;

	int result = SSL_read((SSL *)ssl, address, size);
	if(result < 0) {
		result = 0;
		ioerr = Socket::error();
	}
	return (size_t) result;
}

bool SSocket::flush(void)
{
	int result;

	if(TCPSocket::flush()) {
		if(bio)
			result = BIO_flush((BIO *)bio);
		return true;
	}

	return false;
}

