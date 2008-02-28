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

tcpstream::tcpstream(int family, timeout_t tv) :
	streambuf(), Socket(family, SOCK_STREAM, IPPROTO_TCP),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
}

tcpstream::tcpstream(Socket::address *list, unsigned segsize, timeout_t tv) :
	streambuf(), Socket(list->family(), SOCK_STREAM, IPPROTO_TCP),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
	open(list);
}

tcpstream::tcpstream(ListenSocket &listener, unsigned segsize, timeout_t tv) :
	streambuf(), Socket(listener.accept()),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;

#ifdef OLD_STDCPP
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
	tcpstream::release();
}

void tcpstream::release(void)
{
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	clear();
	Socket::release();
}

int tcpstream::uflow()
{
    int ret = underflow();

    if (ret == EOF)
        return EOF;

    if (bufsize != 1)
        gbump(1);

    return ret;
}

int tcpstream::underflow()
{
	ssize_t rlen = 1;
	unsigned char ch;

	if(bufsize == 1) {
        if(timeout && !Socket::waitPending(timeout)) {
            clear(ios::failbit | rdstate());
            return EOF;
        }
        else
            rlen = Socket::get(&ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
				close();
            return EOF;
        }
        return ch;
    }

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return (unsigned char)*gptr();

    rlen = (ssize_t)((gbuf + bufsize) - eback());
    if(timeout && !Socket::waitPending(timeout)) {
        clear(ios::failbit | rdstate());
        return EOF;
    }
    else
        rlen = Socket::get(eback(), rlen);
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
			close();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return (unsigned char) *gptr();
}

int tcpstream::overflow(int c)
{
    unsigned char ch;
    ssize_t rlen, req;

    if(bufsize == 1) {
        if(c == EOF)
            return 0;

		ch = (unsigned char)(c);
        rlen = Socket::put(&ch, 1);
        if(rlen < 1) {
            if(rlen < 0) 
				close();
            return EOF;
        }
        else
            return c;
    }

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = Socket::put(pbase(), req);
        if(rlen < 1) {
            if(rlen < 0) 
				close();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = (unsigned char)c;
        pbump(1);
    }
    return c;
}

void tcpstream::open(Socket::address *list, unsigned mss)
{
	close();	// close if existing is open...

	if(Socket::connect(*list))
		return;

	allocate(mss);
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
	Socket::disconnect();
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

