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

static SSL session(context *ctx)
{
	SSL ssl = NULL;
	if(ctx && ctx->xcred && ctx->err() == secure::OK) {
		gnutls_init(&ssl, ctx->connect);
		switch(ctx->connect) {
		case GNUTLS_CLIENT:
			gnutls_priority_set_direct(ssl, "PERFORMANCE", NULL);
			break;
		case GNUTLS_SERVER:
			gnutls_priority_set(ssl, context::priority_cache);
			gnutls_certificate_server_set_request(ssl, GNUTLS_CERT_REQUEST);
			gnutls_session_enable_compatibility_mode(ssl);
		default:
			break;
		}
		gnutls_credentials_set(ssl, ctx->xtype, ctx->xcred);
	}
	return ssl;
}

SSocket::SSocket(TCPServer *server, secure::context_t scontext, size_t size) :
TCPSocket(server, size)
{
	ssl = session((context *)scontext);
	bio = NULL;

	if(!isopen() || !ssl)
		return;

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		bio = ssl;
}

SSocket::SSocket(const char *service, secure::context_t scontext) :
TCPSocket(service)
{
	ssl = session((context *)scontext);
	bio = NULL;
}

SSocket::~SSocket()
{
	release();
}

bool SSocket::_pending(void)
{
	return TCPSocket::_pending();
}

void SSocket::open(TCPServer *server, size_t size)
{
	close();

	TCPSocket::open(server, size);

	if(!isopen() || !ssl)
		return;	

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		bio = ssl;
}

void SSocket::open(const char *host, size_t size)
{
	close();

	TCPSocket::open(host, size);

	if(!isopen() || !ssl)
		return;	

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		bio = ssl;
}

void SSocket::close(void)
{
	if(bio)
		gnutls_bye((SSL)ssl, GNUTLS_SHUT_RDWR);
	bio = NULL;
	TCPSocket::close();
}

void SSocket::release(void)
{
	close();
	if(ssl) {
		gnutls_deinit((SSL)ssl);
		ssl = NULL;
	}
}

bool SSocket::_flush(void)
{
	return TCPSocket::_flush();
}

size_t SSocket::_push(const char *address, size_t size)
{
	if(!bio)
		return TCPSocket::_push(address, size);

	int result = gnutls_record_send((SSL)ssl, address, size);
	if(result < 0) {
		result = 0;
		ioerr = Socket::error();
	}
	return (size_t)result;
}

size_t SSocket::_pull(char *address, size_t size)
{
    if(!bio)
        return TCPSocket::_pull(address, size);

    int result = gnutls_record_recv((SSL)ssl, address, size);
    if(result < 0) {
        result = 0;
        ioerr = Socket::error();
    }
    return (size_t)result;
}

