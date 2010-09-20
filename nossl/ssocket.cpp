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

SSocket::SSocket(const char *service, secure::context_t context) :
TCPSocket(service)
{
	ssl = NULL;
	bio = NULL;
}

SSocket::SSocket(TCPServer *server, secure::context_t context, size_t size) :
TCPSocket(server, size)
{
	ssl = NULL;
	bio = NULL;
}


SSocket::~SSocket()
{
}

void SSocket::open(const char *host, size_t bufsize)
{
	TCPSocket::open(host, bufsize);
}

void SSocket::open(TCPServer *server, size_t bufsize)
{
	TCPSocket::open(server, bufsize);
}

void SSocket::close(void)
{
	TCPSocket::close();
}

void SSocket::release(void)
{
	TCPSocket::close();
}

size_t SSocket::_push(const char *address, size_t size)
{
	return TCPSocket::_push(address, size);
}

size_t SSocket::_pull(char *address, size_t size)
{
	return TCPSocket::_pull(address, size);
}

bool SSocket::_pending(void)
{
	return TCPSocket::_pending();
}

bool SSocket::_flush(void)
{
	return TCPSocket::_flush();
}

