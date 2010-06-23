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

#ifndef	_MSWINDOWS_
#include <fcntl.h>
#endif

void random::seed(void)
{
	time_t now;
	
	time(&now);
	srand((int)now);
}

bool random::seed(const unsigned char *buf, size_t size)
{
#ifdef	_MSWINDOWS_
	return false;
#else
	int fd = open("/dev/random", O_WRONLY);
	bool result = false;

	if(fd > -1) {
		if(write(fd, buf, size) > 0)
			result = true;
		close(fd);
	}
	return result;
#endif
}

size_t random::key(unsigned char *buf, size_t size)
{
#ifdef	_MSWINDOWS_
	if(_handle != NULL && CryptGenRandom(_handle, size, buf))
		return size;
	return 0;
#else
	int fd = open("/dev/random", O_RDONLY);
	ssize_t result = 0;

	if(fd > -1) {
		result = read(fd, buf, size);
		close(fd);
	}

	if(result < 0)
		result = 0;

	return (size_t)result;
#endif
}

size_t random::fill(unsigned char *buf, size_t size)
{
#ifdef	_MSWINDOWS_
	return key(buf, size);
#else
	int fd = open("/dev/urandom", O_RDONLY);
	ssize_t result = 0;

	if(fd > -1) {
		result = read(fd, buf, size);
		close(fd);
	}
	// ugly...would not trust it
	else {
		result = 0;
		while(result++ < (ssize_t)size)
			*(buf++) = rand() & 0xff;
	}

	if(result < 0)
		result = 0;

	return (size_t)result;
#endif
}

int random::get(void)
{
	uint16_t v;;
	fill((unsigned char *)&v, sizeof(v));
	v /= 2;
	return (int)v;
}

int random::get(int min, int max)
{
	unsigned rand;
	int range = max - min + 1;
	unsigned umax;

	if(max < min)
		return 0;

	memset(&umax, 0xff, sizeof(umax));

	do {
		fill((unsigned char *)&rand, sizeof(rand));
	} while(rand > umax - (umax % range));

	return min + (rand % range);
}

double random::real(void)
{
	unsigned umax;
	unsigned rand;

	memset(&umax, 0xff, sizeof(umax));
	fill((unsigned char *)&rand, sizeof(rand));

	return ((double)rand) / ((double)umax);
}

double random::real(double min, double max)
{
	return real() * (max - min) + min;
}

bool random::status(void)
{
#ifdef	_MSWINDOWS_
	return true;
#else
	if(fsys::isfile("/dev/random"))
		return true;

	return false;
#endif
}

