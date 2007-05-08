#include <config.h>
#include <ucommon/socket.h>
#include <ucommon/string.h>
#ifndef	_MSWINDOWS_
#include <net/if.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <errno.h>

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

#if defined(HAVE_POLL) && defined(POLLRDNORM)
#define	USE_POLL
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

#ifndef	MSG_NOSIGNAL
#define	MSG_NOSIGNAL 0
#endif

typedef struct
{
	union {
		struct sockaddr addr;
		struct sockaddr_in ipv4;
#ifdef	AF_INET6
		struct sockaddr_in6 ipv6;
#endif
	};
}	inetsockaddr_t;

typedef struct
{
	union {
		struct ip_mreq	ipv4;
#ifdef	AF_INET6
		struct ipv6_mreq ipv6;
#endif
	};
}	inetmulticast_t;

using namespace UCOMMON_NAMESPACE;

typedef unsigned char   bit_t;

static void bitmask(bit_t *bits, bit_t *mask, unsigned len)
{
    while(len--)
        *(bits++) &= *(mask++);
}

static void bitimask(bit_t *bits, bit_t *mask, unsigned len)
{
    while(len--)
        *(bits++) |= ~(*(mask++));
}

static void bitset(bit_t *bits, unsigned blen)
{
    bit_t mask;

    while(blen) {
        mask = (bit_t)(1 << 7);
        while(mask && blen) {
            *bits |= mask;
            mask >>= 1;
            --blen;
        }
        ++bits;
    }
}

static unsigned bitcount(bit_t *bits, unsigned len)
{
    unsigned count = 0;
    bit_t mask, test;

    while(len--) {
        mask = (bit_t)(1<<7);
        test = *bits++;
        while(mask) {
            if(!(mask & test))
                return count;
            ++count;
            mask >>= 1;
		}
    }
    return count;
}


cidr::cidr() :
LinkedObject()
{
	family = AF_UNSPEC;
	memset(&network, 0, sizeof(network));
	memset(&netmask, 0, sizeof(netmask));
}

cidr::cidr(const char *cp) :
LinkedObject()
{
	set(cp);
}

cidr::cidr(LinkedObject **policy, const char *cp) :
LinkedObject(policy)
{
	set(cp);
}

cidr::cidr(const cidr &copy) :
LinkedObject()
{
	family = copy.family;
	memcpy(&network, &copy.network, sizeof(network));
	memcpy(&netmask, &copy.netmask, sizeof(netmask));
}

unsigned cidr::getMask(void) const
{
	switch(family)
	{
	case AF_INET:
		return bitcount((bit_t *)&netmask.ipv4, sizeof(struct in_addr));
#ifdef	AF_INET6
	case AF_INET6:
		return bitcount((bit_t *)&netmask.ipv6, sizeof(struct in6_addr));
#endif
	default:
		return 0;
	}
}

int cidr::policy(SOCKET so, cidr *paccept, cidr *preject, int prior)
{
	struct sockaddr_storage addr;
	socklen_t slen = sizeof(addr);
	int allowed = 0, denied = 0, masked;
	linked_pointer<cidr> accept = paccept;
	linked_pointer<cidr> reject = preject;

	if(so == INVALID_SOCKET)
		return false;

	if(getpeername(so, (struct sockaddr *)&addr, &slen))
		return false;

	while(accept) {
		if(accept->isMember((struct sockaddr *)&addr)) {
			masked = accept->getMask();
			if(masked > allowed)
				allowed = masked;
		}
		accept.next();
	}

    while(reject) {
        if(reject->isMember((struct sockaddr *)&addr)) {
			masked = reject->getMask();
			if(masked > denied)
				denied = masked;
        }
		reject.next();
    }

	if(allowed == denied)
		return prior;
	else if(allowed > denied) {
		if(prior < 0 && allowed > -prior)
			return allowed;
		if(prior >= 0 && allowed > prior)
			return allowed;
		if(allowed == -prior)
			return 0;
		return prior;
	}
	else {
		if(prior > 0 && denied > prior)
			return -denied;
		if(prior <= 0 && denied > -prior)
			return -denied;
		if(denied == prior)
			return 0;
		return prior;
	}
}	

bool cidr::find(cidr *policy, const struct sockaddr *s)
{
	linked_pointer<cidr> p = policy;
	while(p) {
		if(policy->isMember(s))
			return true;
		p.next();
	}
	return false;
}

bool cidr::isMember(const struct sockaddr *s) const
{
	inethostaddr_t host;
	inetsockaddr_t *addr = (inetsockaddr_t *)s;

	if(addr->addr.sa_family != family)
		return false;

	switch(family) {
	case AF_INET:
		memcpy(&host.ipv4, &addr->ipv4.sin_addr, sizeof(host.ipv4));
		bitmask((bit_t *)&host.ipv4, (bit_t *)&netmask, sizeof(host.ipv4));
		if(!memcmp(&host.ipv4, &network.ipv4, sizeof(host.ipv4)))
			return true;
		return false;
#ifdef	AF_INET6
	case AF_INET6:
		memcpy(&host.ipv6, &addr->ipv6.sin6_addr, sizeof(host.ipv6));
		bitmask((bit_t *)&host.ipv6, (bit_t *)&netmask, sizeof(host.ipv6));
		if(!memcmp(&host.ipv6, &network.ipv6, sizeof(host.ipv6)))
			return true;
		return false;
#endif
	default:
		return false;
	}
}

inethostaddr_t cidr::getBroadcast(void) const
{
	inethostaddr_t bcast;

	switch(family) {
	case AF_INET:
		memcpy(&bcast.ipv4, &network.ipv4, sizeof(network.ipv4));
		bitimask((bit_t *)&bcast.ipv4, (bit_t *)&netmask.ipv4, sizeof(bcast.ipv4));
		return bcast;
#ifdef	AF_INET6
	case AF_INET6:
		memcpy(&bcast.ipv6, &network.ipv6, sizeof(network.ipv6));
		bitimask((bit_t *)&bcast.ipv6, (bit_t *)&netmask.ipv6, sizeof(bcast.ipv6));
		return bcast;
#endif
	default:
		memset(&bcast, 0, sizeof(bcast));
		return  bcast;
	}
}

unsigned cidr::getMask(const char *cp) const
{
	unsigned count = 0, rcount = 0, dcount = 0;
	const char *sp = strchr(cp, '/');
	bool flag = false;
	const char *gp = cp;
	unsigned char dots[4];
	uint32_t mask;

	switch(family) {
#ifdef	AF_INET6
	case AF_INET6:
		if(sp)
			return atoi(++sp);
		if(!strncmp(cp, "ff00:", 5))
			return 8;
		if(!strncmp(cp, "ff80:", 5))
			return 10;
		if(!strncmp(cp, "2002:", 5))
			return 16;
		
		sp = strrchr(cp, ':');
		while(*(++sp) == '0')
			++sp;
		if(*sp)
			return 128;
		
		while(*cp && count < 128) {
			if(*(cp++) == ':') {
				count += 16;
				while(*cp == '0')
					++cp;
				if(*cp == ':') {
					if(!flag)
						rcount = count;
					flag = true;
				}			
				else
					flag = false;
			}
		}
		return rcount;
#endif
	case AF_INET:
		if(sp) {
			if(!strchr(++sp, '.'))
				return atoi(sp);
			mask = inet_addr(sp);
			return bitcount((bit_t *)&mask, sizeof(mask));
		}
		memset(dots, 0, sizeof(dots));
		dots[0] = atoi(cp);
		while(*gp && dcount < 3) {
			if(*(gp++) == '.')
				dots[++dcount] = atoi(gp);
		}
		if(dots[3])
			return 32;

		if(dots[2])
			return 24;

		if(dots[1])
			return 16;

		return 8;
	default:
		return 0;
	}
}

void cidr::set(const char *cp)
{
	char cbuf[128];
	char *ep;
	unsigned dots = 0;
#ifdef	_MSWINDOWS_
	struct sockaddr saddr;
	int slen;
	struct sockaddr_in6 *paddr;
	int ok;
#endif

#ifdef	AF_INET6
	if(strchr(cp, ':'))
		family = AF_INET6;
	else
#endif
		family = AF_INET;

	switch(family) {
	case AF_INET:
		memset(&netmask.ipv4, 0, sizeof(netmask.ipv4));
		bitset((bit_t *)&netmask.ipv4, getMask(cp));
		string::set(cbuf, sizeof(cbuf), cp);
		ep = strchr(cp, '/');
		if(ep)
			*ep = 0;

		cp = cbuf;
		while(NULL != (cp = strchr(cp, '.'))) {
			++dots;
			++cp;
		}

		while(dots++ < 3)
			string::add(cbuf, sizeof(cbuf), ".0");

#ifdef	_MSWINDOWS_
		DWORD addr = (DWORD)inet_addr(cbuf);
		memcpy(&network.ipv4, &addr, sizeof(network.ipv4));
#else
		inet_aton(cbuf, &network.ipv4);
#endif
		bitmask((bit_t *)&network.ipv4, (bit_t *)&netmask.ipv4, sizeof(network.ipv4));
		break;
#ifdef	AF_INET6
	case AF_INET6:
		memset(&netmask.ipv6, 0, sizeof(netmask));
		bitset((bit_t *)&netmask.ipv6, getMask(cp));
		string::set(cbuf, sizeof(cbuf), cp);
		ep = strchr(cp, '/');
		if(ep)
			*ep = 0;
#ifdef	_MSWINDOWS_
		struct sockaddr saddr;
    	slen = sizeof(saddr);
    	paddr = (struct sockaddr_in6 *)&saddr;
    	ok = WSAStringToAddress((LPSTR)cbuf, AF_INET6, NULL, &saddr, &slen);
    	network.ipv6 = paddr->sin6_addr;
#else
		inet_pton(AF_INET6, cbuf, &network.ipv6);
#endif
		bitmask((bit_t *)&network.ipv6, (bit_t *)&netmask.ipv6, sizeof(network.ipv6));
#endif
	default:
		break;
	}
}

Socket::address::address(const char *a, int family, int type, int protocol)
{
	struct addrinfo hint;
	char *addr = strdup(a);
	char *host = strchr(addr, '@');
	char *ep;
	char *svc = NULL;

	memset(&hint, 0, sizeof(hint));

	if(!host)
		host = addr;
	else
		++host;

	if(*host != '[') {
		ep = strchr(host, ':');
		if(ep) {
			*(ep++) = 0;
			svc = ep;
		}
		goto proc;
	}
#ifdef	AF_INET6
	if(*host == '[') {
		family = AF_INET6;
		++host;
		ep = strchr(host, ']');
		if(ep) {
			*(ep++) = 0;
			if(*ep == ':')
				svc = ++ep;
		}
	}
#endif
proc:
	hint.ai_family = family;
	hint.ai_socktype = type;
	hint.ai_protocol = protocol;
	getaddrinfo(host, svc, &hint, &list);
}

Socket::address::address(int family, const char *host, const char *svc)
{
	struct addrinfo hint;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = family;

	getaddrinfo(host, svc, &hint, &list);
}

Socket::address::address(Socket &s, const char *host, const char *svc)
{
	address(host, svc, (int)s);
}

Socket::address::address(const char *host, const char *svc, SOCKET so)
{
	struct addrinfo hint;
	struct addrinfo *ah = NULL;

	if(so)
		ah = gethint(so, &hint);

	getaddrinfo(host, svc, ah, &list);	
}

Socket::address::~address()
{
	if(list) {
		freeaddrinfo(list);
		list = NULL;
	}
}

struct sockaddr *Socket::address::getAddr(void)
{
	if(!list)
		return NULL;
	
	return list->ai_addr;
}

Socket::Socket(const Socket &s)
{
#ifdef	_MSWINDOWS_
	HANDLE pidH = GetCurrentProcess();
	HANDLE dupH;

	if(DuplicateHandle(pidH, reinterpret_cast<HANDLE>(s.so), pidH, &dupH, 0, FALSE, DUPLICATE_SAME_ACCESS))
		so = reinterpret_cast<SOCKET>(dupH);
	else
		so = INVALID_SOCKET;
#else
	so = ::dup(s.so);
#endif
}

Socket::Socket()
{
	so = INVALID_SOCKET;
}

Socket::Socket(SOCKET s)
{
	so = s;
}

Socket::Socket(struct addrinfo *addr)
{
	while(addr) {
		so = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if(so != INVALID_SOCKET) {
			if(!::connect(so, addr->ai_addr, addr->ai_addrlen))
				return;
		}
		addr = addr->ai_next;
	}
	so = INVALID_SOCKET;
}

Socket::Socket(int family, int type, int protocol)
{
	so = ::socket(family, type, protocol);
}

Socket::Socket(const char *iface, const char *port, int family, int type, int protocol)
{
	so = ::socket(family, type, protocol);
	if(so != INVALID_SOCKET)
		if(bindaddr(so, iface, port))
			release();
}

Socket::~Socket()
{
	release();
}

bool Socket::create(int family, int type, int protocol)
{
	release();
	so = ::socket(family, type, protocol);
	return so != INVALID_SOCKET;
}

void Socket::release(void)
{
	if(so != INVALID_SOCKET) {
#ifdef	_MSWINDOWS_
		::closesocket(so);
#else
		::shutdown(so, SHUT_RDWR);
		::close(so);
#endif
		so = INVALID_SOCKET;
	}
}

Socket::operator bool()
{
	if(so == INVALID_SOCKET)
		return false;
	return true;
}

bool Socket::operator!() const
{
	if(so == INVALID_SOCKET)
		return true;
	return false;
}

Socket &Socket::operator=(SOCKET s)
{
	release();
	so = s;
	return *this;
}	

size_t Socket::peek(void *data, size_t len) const
{
	ssize_t rtn = ::recv(so, (caddr_t)data, 1, MSG_DONTWAIT | MSG_PEEK);
	if(rtn < 1)
		return 0;
	return (size_t)rtn;
}

ssize_t Socket::get(void *data, size_t len, sockaddr *from)
{
	socklen_t slen = 0;
	return ::recvfrom(so, (caddr_t)data, len, 0, from, &slen);
}

ssize_t Socket::put(const void *data, size_t len, sockaddr *dest)
{
	socklen_t slen = 0;
	if(dest)
		slen = getlen(dest);
	
	return ::sendto(so, (caddr_t)data, len, MSG_NOSIGNAL, dest, slen);
}

ssize_t Socket::puts(const char *str)
{
	if(!str)
		return 0;

	return put(str, strlen(str));
}

ssize_t Socket::gets(char *data, size_t max, timeout_t timeout)
{
	bool crlf = false;
	bool nl = false;
	int nleft = max - 1;		// leave space for null byte
	int nstat, c;

	if(max < 1)
		return -1;

	data[0] = 0;
	while(nleft && !nl) {
		if(timeout) {
			if(!waitPending(timeout))
				return -1;
		}
		nstat = ::recv(so, data, nleft, MSG_PEEK);
		if(nstat <= 0)
			return -1;
		
		for(c = 0; c < nstat; ++c) {
			if(data[c] == '\n') {
				if(c > 0 && data[c - 1] == '\r')
					crlf = true;
				++c;
				nl = true;
				break;
			}
		}
		nstat = ::recv(so, (caddr_t)data, c, 0);
		if(nstat < 0)
			break;
			
		if(crlf) {
			--nstat;
			data[nstat - 1] = '\n';
		}	

		data += nstat;
		nleft -= nstat;
	}
	*data = 0;
	return ssize_t(max - nleft - 1);
}

int Socket::setTimeToLive(unsigned char ttl)
{
	struct sockaddr_storage saddr;
	struct sockaddr *addr = (struct sockaddr *)&saddr;
	int family;
	socklen_t len = sizeof(addr);

	if(so == INVALID_SOCKET)
		return -1;

	getsockname(so, (struct sockaddr *)&addr, &len);
	family = addr->sa_family;
	switch(family) {
	case AF_INET:
		return setsockopt(so, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl));
#ifdef	AF_INET6
	case AF_INET6:
		return setsockopt(so, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&ttl, sizeof(ttl));
#endif
	}
	return -1;
}

int Socket::setTypeOfService(unsigned tos)
{
	if(so == INVALID_SOCKET)
		return -1;

#ifdef	SOL_IP
	return setsockopt(so, SOL_IP, IP_TOS,(char *)&tos, (socklen_t)sizeof(tos));
#else
	return -1;
#endif
}

int Socket::setBroadcast(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;
    int opt = (enable ? 1 : 0);
    return ::setsockopt(so, SOL_SOCKET, SO_BROADCAST,
              (char *)&opt, (socklen_t)sizeof(opt));
}

int Socket::setKeepAlive(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;
#if defined(SO_KEEPALIVE) || defined(_MSWINDOWS_)
	int opt = (enable ? ~0 : 0);
	return ::setsockopt(so, SOL_SOCKET, SO_KEEPALIVE,
             (char *)&opt, (socklen_t)sizeof(opt));
#else
	return -1;
#endif
}				

int Socket::setMulticast(bool enable)
{
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);

	if(so == INVALID_SOCKET)
		return -1;

	::getsockname(so, (struct sockaddr *)&addr, &len);
	if(!enable)
		switch(addr.addr.sa_family)
		{
		case AF_INET:
			memset(&addr.ipv4.sin_addr, 0, sizeof(addr.ipv4.sin_addr));
			break;
#ifdef	AF_INET6
		case AF_INET6:
			memset(&addr.ipv6.sin6_addr, 0, sizeof(addr.ipv6.sin6_addr));
			break;
#endif
		default:
			break;
		}
	switch(addr.addr.sa_family) {
#ifdef	AF_INET6
	case AF_INET6:
		return ::setsockopt(so, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
#endif
	case AF_INET:
#ifdef	IP_MULTICAST_IF
		return ::setsockopt(so, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
#else
		return -1;
#endif
	default:
		return -1;
	}	
}

int Socket::setNonBlocking(bool enable)
{
	if(so == INVALID_SOCKET)
		return -1;

#if defined(_MSWINDOWS_)
	unsigned long flag = (enable ? 1 : 0);
	return ioctlsocket(so, FIONBIO, &flag);
#else
	long flags = fcntl(so, F_GETFL);
	if(enable)
		flags |= O_NONBLOCK;
	else
		flags &=~ O_NONBLOCK;
	return fcntl(so, F_SETFL, flags);
#endif
}

int Socket::disconnect(void)
{
	struct sockaddr_storage saddr;
	struct sockaddr *addr = (struct sockaddr *)&saddr;
	int family;
	socklen_t len = sizeof(addr);

	getsockname(so, (struct sockaddr *)&addr, &len);
	family = addr->sa_family;
	memset(addr, 0, sizeof(saddr));
#if defined(_MSWINDOWS_)
	addr->sa_family = family;
#else
	addr->sa_family = AF_UNSPEC;
#endif
	if(len > sizeof(saddr))
		len = sizeof(saddr);
	return ::connect(so, addr, len);
}

int Socket::join(struct addrinfo *node)
{
	inetmulticast_t mcast;
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);
	inetsockaddr_t *target;
	int family;
	int rtn;

	if(so == INVALID_SOCKET)
		return -1;
	
	getsockname(so, (struct sockaddr *)&addr, &len);
	while(!rtn && node && node->ai_addr) {
		target = (inetsockaddr_t *)node->ai_addr;
		family = node->ai_family;
		node = node->ai_next;
		if(family != addr.addr.sa_family)
			continue;
		switch(addr.addr.sa_family) {
#if defined(AF_INET6) && defined(IPV6_ADD_MEMBERSHIP)
		case AF_INET6:
			memcpy(&mcast.ipv6.ipv6mr_interface, &addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
			memcpy(&mcast.ipv6.ipv6mr_multiaddr, &target->ipv6.sin6_addr, sizeof(target->ipv6.sin6_addr));
			rtn = ::setsockopt(so, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
#endif
#if defined(IP_ADD_MEMBERSHIP)
		case AF_INET:
			memcpy(&mcast.ipv4.imr_interface, &addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
			memcpy(&mcast.ipv4.imr_multiaddr, &target->ipv4.sin_addr, sizeof(target->ipv4.sin_addr));
			rtn = ::setsockopt(so, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
#endif
		default:
			rtn = -1;
		}
	}
	return rtn;
}

int Socket::drop(struct addrinfo *node)
{
	inetmulticast_t mcast;
	inetsockaddr_t addr;
	socklen_t len = sizeof(addr);
	inetsockaddr_t *target;
	int family;
	int rtn;

	if(so == INVALID_SOCKET)
		return -1;
	
	getsockname(so, (struct sockaddr *)&addr, &len);
	while(!rtn && node && node->ai_addr) {
		target = (inetsockaddr_t *)node->ai_addr;
		family = node->ai_family;
		node = node->ai_next;

		if(family != addr.addr.sa_family)
			continue;

		switch(addr.addr.sa_family) {
#if defined(AF_INET6) && defined(IPV6_DROP_MEMBERSHIP)
		case AF_INET6:
			memcpy(&mcast.ipv6.ipv6mr_interface, &addr.ipv6.sin6_addr, sizeof(addr.ipv6.sin6_addr));
			memcpy(&mcast.ipv6.ipv6mr_multiaddr, &target->ipv6.sin6_addr, sizeof(target->ipv6.sin6_addr));
			rtn = ::setsockopt(so, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
#endif
#if defined(IP_DROP_MEMBERSHIP)
		case AF_INET:
			memcpy(&mcast.ipv4.imr_interface, &addr.ipv4.sin_addr, sizeof(addr.ipv4.sin_addr));
			memcpy(&mcast.ipv4.imr_multiaddr, &target->ipv4.sin_addr, sizeof(target->ipv4.sin_addr));
			rtn = ::setsockopt(so, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mcast, sizeof(mcast.ipv6));
#endif
		default:
			rtn = -1;
		}
	}
	return rtn;
}
	
int Socket::connect(struct addrinfo *node)
{
	int rtn = -1;
	int family;
	
	if(so == INVALID_SOCKET)
		return -1;

	family = getfamily(so);

	while(node) {
		if(node->ai_family == family) {
			if(!::connect(so, node->ai_addr, node->ai_addrlen)) {
				rtn = 0;
				goto exit;
			}
		}
		node = node->ai_next;
	}

exit:
#ifndef _MSWINDOWS_
	if(!rtn || errno == EINPROGRESS)
		return 0;
#endif
	return rtn;
}

int Socket::getError(void)
{
	int opt;
	socklen_t slen = sizeof(opt);

	if(getsockopt(so, SOL_SOCKET, SO_ERROR, (caddr_t)&opt, &slen))
		return -1;
	
	return opt;
}

bool Socket::isConnected(void) const
{
	char buf;

	if(so == INVALID_SOCKET)
		return false;

	if(!waitPending())
		return true;

	if(::recv(so, &buf, 1, MSG_DONTWAIT | MSG_PEEK) < 1)
		return false;

	return true;
}

bool Socket::isPending(unsigned qio) const
{
	if(getPending() >= qio)
		return true;

	return false;
}

#ifdef _MSWINDOWS_
unsigned Socket::getPending(void) const
{
	u_long opt;
	if(so == INVALID_SOCKET)
		return 0;

	ioctlsocket(so, FIONREAD, &opt);
	return (unsigned)opt;
}
#else
unsigned Socket::getPending(void) const
{
	int opt;
	if(so == INVALID_SOCKET)
		return 0;

	if(::ioctl(so, FIONREAD, &opt))
		return 0;
	return (unsigned)opt;
}
#endif

bool Socket::waitPending(timeout_t timeout) const
{
	int status;

#ifdef	USE_POLL
	struct pollfd pfd;

	pfd.fd = so;
	pfd.revents = 0;
	pfd.events = POLLIN;

	if(so == INVALID_SOCKET)
		return false;

	status = 0;
	while(status < 1) {
		if(timeout == Timer::inf)
			status = ::poll(&pfd, 1, -1);
		else
			status = ::poll(&pfd, 1, timeout);
		if(status == -1 && errno == EINTR)
			continue;
		if(status < 0)
			return false;
	}
	if(pfd.revents & POLLIN)
		return true;
	return false;
#else
	struct timeval tv;
	struct timeval *tvp = &tv;
	fd_set grp;

	if(so == INVALID_SOCKET)
		return false;

	if(timeout == Timer::inf)
		tvp = NULL;
	else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(so, &grp);
	status = ::select((int)(so + 1), &grp, NULL, NULL, tvp);
	if(status < 1)
		return false;
	if(FD_ISSET(so, &grp))
		return true;
	return false;
#endif
}

bool Socket::waitSending(timeout_t timeout) const
{
	int status;
#ifdef	USE_POLL
	struct pollfd pfd;

	pfd.fd = so;
	pfd.revents = 0;
	pfd.events = POLLOUT;

	if(so == INVALID_SOCKET)
		return false;

	status = 0;
	while(status < 1) {
		if(timeout == Timer::inf)
			status = ::poll(&pfd, 1, -1);
		else
			status = ::poll(&pfd, 1, timeout);
		if(status == -1 && errno == EINTR)
			continue;
		if(status < 0)
			return false;
	}
	if(pfd.revents & POLLOUT)
		return true;
	return false;
#else
	struct timeval tv;
	struct timeval *tvp = &tv;
	fd_set grp;

	if(so == INVALID_SOCKET)
		return false;

	if(timeout == Timer::inf)
		tvp = NULL;
	else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(so, &grp);
	status = ::select((int)(so + 1), NULL, &grp, NULL, tvp);
	if(status < 1)
		return false;
	if(FD_ISSET(so, &grp))
		return true;
	return false;
#endif
}

ListenSocket::ListenSocket(const char *iface, const char *svc, unsigned backlog) :
Socket()
{
	int family = AF_INET;
	if(strchr(iface, '/'))
		family = AF_UNIX;
	else if(strchr(iface, ':'))
		family = AF_INET6;

retry:
	so = ::socket(family, SOCK_STREAM, 0);
	if(so == INVALID_SOCKET)
		return;
		
	if(bindaddr(so, iface, svc)) {
		release();
		if(family == AF_INET && !strchr(iface, '.')) {
			family = AF_INET6;
			goto retry;
		}
		return;
	}
	if(::listen(so, backlog))
		release();
}

SOCKET ListenSocket::accept(struct sockaddr *addr)
{
	socklen_t len = 0;
	if(addr) {
		len = getlen(addr);		
		return ::accept(so, addr, &len);
	}
	else
		return ::accept(so, NULL, NULL);
}

#ifdef	_MSWINDOWS_
#undef	AF_UNIX
#endif

#ifdef	AF_UNIX

static socklen_t unixaddr(struct sockaddr_un *addr, const char *path)
{
	socklen_t len;
	unsigned slen = strlen(path);

    if(slen > sizeof(struct sockaddr_storage) - 8)
        slen = sizeof(struct sockaddr_storage) - 8;

    memset(addr, 0, sizeof(struct sockaddr_storage));
    addr->sun_family = AF_UNIX;
    memcpy(addr->sun_path, path, slen);

#ifdef	__SUN_LEN
	len = sizeof(addr->sun_len) + strlen(addr->sun_path) + 
		sizeof(addr->sun_family) + 1;
	addr->sun_len = len;
#else
    len = strlen(addr->sun_path) + sizeof(addr->sun_family) + 1;
#endif
	return len;
}

#endif

struct addrinfo *Socket::gethint(SOCKET so, struct addrinfo *hint)
{
	struct sockaddr_storage st;
	struct sockaddr *sa = (struct sockaddr *)&st;
	socklen_t slen;

	memset(hint, 0 , sizeof(struct addrinfo));
	memset(&sa, 0, sizeof(sa));
	if(getsockname(so, sa, &slen))
		return NULL;
	hint->ai_family = sa->sa_family;
	slen = sizeof(hint->ai_socktype);
	getsockopt(so, SOL_SOCKET, SO_TYPE, (caddr_t)&hint->ai_socktype, &slen);
	return hint;
}

char *Socket::hosttostr(struct sockaddr *sa, char *buf, size_t max)
{
	socklen_t sl;

#ifdef	AF_UNIX
    struct sockaddr_un *un = (struct sockaddr_un *)sa;
#endif

	switch(sa->sa_family) {
#ifdef	AF_UNIX
	case AF_UNIX:
		if(max > sizeof(un->sun_path))
			max = sizeof(un->sun_path);
		else
			--max;
		strncpy(buf, un->sun_path, max);
		buf[max] = 0;
		return buf;		
#endif
	case AF_INET:
		sl = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		sl = sizeof(struct sockaddr_in6);
		break;
	default:
		return NULL;
	}

	if(getnameinfo(sa, sl, buf, max, NULL, 0, NI_NOFQDN))
		return NULL;

	return buf;
}

socklen_t Socket::getaddr(SOCKET so, struct sockaddr_storage *sa, const char *host, const char *svc)
{
	socklen_t len = 0;
	struct addrinfo hint, *res = NULL;

#ifdef	AF_UNIX
	if(strchr(host, '/'))
		return unixaddr((struct sockaddr_un *)sa, host);
#endif

	if(!gethint(so, &hint) || !svc)
		return 0;

	if(getaddrinfo(host, svc, &hint, &res) || !res)
		goto exit;

	memcpy(sa, res->ai_addr, res->ai_addrlen);
	len = res->ai_addrlen;

exit:
	if(res)
		freeaddrinfo(res);
	return len;
}

int Socket::bindaddr(SOCKET so, const char *host, const char *svc)
{
	int rtn = -1;
	int reuse = 1;

	struct addrinfo hint, *res = NULL;

    setsockopt(so, SOL_SOCKET, SO_REUSEADDR, (caddr_t)&reuse, sizeof(reuse));

#ifdef AF_UNIX
	if(strchr(host, '/')) {
		struct sockaddr_un uaddr;
		socklen_t len = unixaddr(&uaddr, host);
		rtn = ::bind(so, (struct sockaddr *)&uaddr, len);
		goto exit;	
	};
#endif

    if(!gethint(so, &hint) || !svc)
        return -1;

	if(host && !strcmp(host, "*"))
		host = NULL;

#ifdef	SO_BINDTODEVICE
	if(host && !strchr(host, '.') && !strchr(host, ':')) {
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_ifrn.ifrn_name, host, IFNAMSIZ);
		setsockopt(so, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
		host = NULL;		
	}
#endif	

	hint.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	rtn = getaddrinfo(host, svc, &hint, &res);
	if(rtn)
		goto exit;

	rtn = ::bind(so, res->ai_addr, res->ai_addrlen);
exit:
	if(res)
		freeaddrinfo(res);
	return rtn;
}

socklen_t Socket::getlen(struct sockaddr *sa)
{
	switch(sa->sa_family)
	{
	case AF_INET:
		return sizeof(sockaddr_in);
	case AF_INET6:
		return sizeof(sockaddr_in6);
	default:
		return sizeof(sockaddr_storage);
	}
}

int Socket::getfamily(SOCKET so)
{
	struct sockaddr_storage saddr;
	socklen_t len = sizeof(saddr);
	struct sockaddr *addr = (struct sockaddr *)&saddr;

	if(getsockname(so, addr, &len))
		return AF_UNSPEC;

	return addr->sa_family;
}
