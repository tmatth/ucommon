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

SSLBuffer::SSLBuffer(TCPServer *server, secure::context_t scontext, size_t size) :
TCPBuffer(server, size)
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

SSLBuffer::SSLBuffer(const char *service, secure::context_t scontext) :
TCPBuffer(service)
{
	ssl = session((context *)scontext);
	bio = NULL;
}

SSLBuffer::~SSLBuffer()
{
	release();
}

bool SSLBuffer::_pending(void)
{
	return TCPBuffer::_pending();
}

void SSLBuffer::open(TCPServer *server, size_t size)
{
	close();

	TCPBuffer::open(server, size);

	if(!isopen() || !ssl)
		return;	

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		bio = ssl;
}

void SSLBuffer::open(const char *host, size_t size)
{
	close();

	TCPBuffer::open(host, size);

	if(!isopen() || !ssl)
		return;	

	gnutls_transport_set_ptr((SSL)ssl, (gnutls_transport_ptr_t) so);
	int result = gnutls_handshake((SSL)ssl);

	if(result >= 0)
		bio = ssl;
}

void SSLBuffer::close(void)
{
	if(bio)
		gnutls_bye((SSL)ssl, GNUTLS_SHUT_RDWR);
	bio = NULL;
	TCPBuffer::close();
}

void SSLBuffer::release(void)
{
	close();
	if(ssl) {
		gnutls_deinit((SSL)ssl);
		ssl = NULL;
	}
}

bool SSLBuffer::_flush(void)
{
	return TCPBuffer::_flush();
}

size_t SSLBuffer::_push(const char *address, size_t size)
{
	if(!bio)
		return TCPBuffer::_push(address, size);

	int result = gnutls_record_send((SSL)ssl, address, size);
	if(result < 0) {
		result = 0;
		ioerr = Socket::error();
	}
	return (size_t)result;
}

size_t SSLBuffer::_pull(char *address, size_t size)
{
    if(!bio)
        return TCPBuffer::_pull(address, size);

    int result = gnutls_record_recv((SSL)ssl, address, size);
    if(result < 0) {
        result = 0;
        ioerr = Socket::error();
    }
    return (size_t)result;
}

