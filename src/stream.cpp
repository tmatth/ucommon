// Copyright (C) 1999-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

#if defined(OLD_STDCPP) || defined(NEW_STDCPP)

#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/socket.h>
#include <ucommon/stream.h>

using namespace UCOMMON_NAMESPACE;
using namespace std;

tcpstream::tcpstream(ListenSocket &listener, size_t segsize, timeout_t tv) :
	streambuf(), Socket(listener.accept()),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;

#ifdef	OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
	if(so == INVALID_SOCKET) {
		clear(ios::failbit | rdstate());
		return;
	}
	allocate(segsize);
}

tcpstream::~tcpstream()
{
	close();
}

void tcpstream::close(void)
{
	if(bufsize)
		sync();

	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;
	bufsize = 0;
	clear();
	Socket::release();
}

void tcpstream::allocate(unsigned mss)
{
	unsigned size = mss;
	unsigned max = 0;
	unsigned bufsize;
	socklen_t alen = sizeof(max);
	
	if(mss == 1)
		goto allocate;

#ifdef  TCP_MAXSEG
	if(mss)
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, sizeof(max));
	getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, &alen);
#endif

	if(max && max < mss)
		mss = max;

	if(!mss) {
		if(max)
			mss = max;
		else
			mss = 536;
		goto allocate;
	}

#ifdef  TCP_MAXSEG
	setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

	if(mss < 80)
		mss = 80;

	if(mss * 7 < 64000u)
		bufsize = mss * 7;
	else if(mss * 6 < 64000u)
		bufsize = mss * 6;
	else
		bufsize = mss * 5;

	sendsize(bufsize);
	recvsize(bufsize);

	if(mss < 512)
		sendwait(mss * 4);

allocate:
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;

	if(size < 2) {
		bufsize = 1;
		return;
	}

	gbuf = new char[size];
	pbuf = new char[size];
	assert(gbuf != NULL && pbuf != NULL);
	bufsize = size;
	clear();
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
    setb(gbuf, gbuf + size, 0);
#endif
    setg(gbuf, gbuf + size, gbuf + size);
    setp(pbuf, pbuf + size);
}

int tcpstream::sync(void)
{
	overflow(EOF);
	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	return 0;
}

#endif

