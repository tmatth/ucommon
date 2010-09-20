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

SSLBuffer::SSLBuffer(const char *service, secure::context_t context) :
TCPBuffer(service)
{
	ssl = NULL;
	bio = NULL;
}

SSLBuffer::SSLBuffer(const TCPServer *server, secure::context_t context, size_t size) :
TCPBuffer(server, size)
{
	ssl = NULL;
	bio = NULL;
}


SSLBuffer::~SSLBuffer()
{
}

void SSLBuffer::open(const char *host, size_t bufsize)
{
	TCPBuffer::open(host, bufsize);
}

void SSLBuffer::open(const TCPServer *server, size_t bufsize)
{
	TCPBuffer::open(server, bufsize);
}

void SSLBuffer::close(void)
{
	TCPBuffer::close();
}

void SSLBuffer::release(void)
{
	TCPBuffer::close();
}

size_t SSLBuffer::_push(const char *address, size_t size)
{
	return TCPBuffer::_push(address, size);
}

size_t SSLBuffer::_pull(char *address, size_t size)
{
	return TCPBuffer::_pull(address, size);
}

bool SSLBuffer::_pending(void)
{
	return TCPBuffer::_pending();
}

bool SSLBuffer::_flush(void)
{
	return TCPBuffer::_flush();
}

