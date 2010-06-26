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

void random::seed(void)
{
	secure::init();
}

bool random::seed(const unsigned char *buf, size_t size)
{
	secure::init();
	return true;
}

size_t random::key(unsigned char *buf, size_t size)
{
	gcry_randomize(buf, size, GCRY_VERY_STRONG_RANDOM);
	return 0;
}

size_t random::fill(unsigned char *buf, size_t size)
{
	gcry_randomize(buf, size, GCRY_STRONG_RANDOM);
	return 0;
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
	return true;
}

