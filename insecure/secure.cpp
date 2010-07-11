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

#ifdef	_MSWINDOWS_
NAMESPACE_LOCAL
HCRYPTPROV _handle = NULL;
END_NAMESPACE
#endif

secure::~secure()
{
}

bool secure::init(const char *progname)
{
	Thread::init();
	if(progname)
		Socket::init(progname);
	else
		Socket::init();

#ifdef	_MSWINDOWS_
	if(_handle != NULL)
		return false;

	if(CryptAcquireContext(&_handle, NULL, NULL, PROV_RSA_FULL, 0))
		return false;
	if(GetLastError() == NTE_BAD_KEYSET) {
		if(CryptAcquireContext(&_handle, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
			return false;
	}
			
	_handle = NULL;
#endif
	
	return false;
}

secure::context_t secure::server(const char *ca)
{
	return NULL;
}

secure::context_t secure::client(const char *ca)
{
	return NULL;
}

void secure::cipher(context_t context, const char *ciphers)
{
}

void secure::uuid(char *str)
{
	static mutex_t mutex;
	static unsigned char buf[16];
	static Timer::tick_t prior = 0l;
	static unsigned short seq;
	Timer::tick_t current = Timer::ticks();

	mutex.lock();

	// get our (random) node identifier...
	if(!prior)	
		Random::fill(buf + 10, 6);
	
	if(current == prior)
		++seq;
	else
		Random::fill((unsigned char *)&seq, sizeof(seq));

	buf[8] = (seq >> 8) & 0xff;
	buf[9] = seq & 0xff;
	buf[3] = current & 0xff;
	buf[2] = (current >> 8) & 0xff;
	buf[1] = (current >> 16) & 0xff;
	buf[0] = (current >> 24) & 0xff;
	buf[5] = (current >> 32) & 0xff;
	buf[4] = (current >> 40) & 0xff;
	buf[7] = (current >> 48) & 0xff;
	buf[6] = (current >> 56) & 0xff;

	buf[6] &= 0x0f;
	buf[6] |= 0x10;
	buf[8] |= 0x80;
	String::hexdump(buf, str, "4-2-2-2-6");
	mutex.unlock();
}

String secure::uuid(void)
{
	char buf[38];
	uuid(buf);
	return String(buf);
}

