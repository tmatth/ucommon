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

#include <local.h>

#ifdef	_MSWINDOWS_
NAMESPACE_LOCAL
HCRYPTPROV _handle = NULL;
END_NAMESPACE
#endif

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

void secure::cipher(context_t context, const char *ciphers)
{
}

secure::~secure()
{
}

