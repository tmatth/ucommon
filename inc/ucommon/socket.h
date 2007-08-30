// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
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

#ifndef _UCOMMON_SOCKET_H_
#define	_UCOMMON_SOCKET_H_

#ifndef _UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

struct addrinfo;

#ifdef	_MSWINDOWS_
#include <ws2tcpip.h>
#include <winsock2.h>
#define	SHUT_RDWR	SD_BOTH
#define	SHUT_WR		SD_SEND
#define	SHUT_RD		SD_RECV
#else
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY      0x10
#define IPTOS_THROUGHPUT    0x08
#define IPTOS_RELIABILITY   0x04
#define IPTOS_MINCOST       0x02
#endif

typedef	struct
{
	union
	{
		struct in_addr ipv4;
#ifdef	AF_INET6
		struct in6_addr ipv6;
#endif
	};
}	inethostaddr_t;

#ifdef	AF_INET6
struct sockaddr_internet
{
	union {
		unsigned short sa_family;
		struct sockaddr_in6 ipv6;
		struct sockaddr_in ipv4;
	};
};
#else
struct sockaddr_internet
{
	union {
		unsigned short sa_family;
		struct sockaddr_in ipv4;
	};
};

struct sockaddr_storage
{
	unsigned short sa_family;
#ifdef	AF_UNIX
	char sa_data[104];
#else
	char sa_data[sizeof(struct sockaddr_in) - 2];
#endif
};
#endif

NAMESPACE_UCOMMON

class __EXPORT cidr : public LinkedObject
{
protected:
	int family;
	inethostaddr_t netmask, network;
	char name[16];
	unsigned getMask(const char *cp) const;

public:
	typedef	LinkedObject policy;

	cidr();
	cidr(const char *str);
	cidr(policy **policy, const char *str);
	cidr(policy **policy, const char *str, const char *id);
	cidr(const cidr &copy);

	static cidr *find(policy *policy, const struct sockaddr *addr);

	inline const char *getName(void) const
		{return name;};

	inline int getFamily(void) const
		{return family;};

	inline inethostaddr_t getNetwork(void) const
		{return network;};

	inline inethostaddr_t getNetmask(void) const
		{return netmask;};

	inethostaddr_t getBroadcast(void) const;

	unsigned getMask(void) const;
		
	void set(const char *c);

	bool isMember(const struct sockaddr *saddr) const;

	inline bool operator==(const struct sockaddr *saddr) const
		{return isMember(saddr);};
		
	inline bool operator!=(const struct sockaddr *saddr) const
		{return !isMember(saddr);}; 
};

class __EXPORT Socket 
{
protected:
	Socket();

	SOCKET so;

public:
	class __EXPORT address
	{
	protected:
		struct addrinfo *list;

	public:
		address(const char *a, int family = 0, int type = SOCK_STREAM, int protocol = 0);
		address(Socket &s, const char *host, const char *svc = NULL);
		address(const char *host, const char *svc = NULL, SOCKET so = INVALID_SOCKET);
		address(int family, const char *host, const char *svc = NULL);
		address();
		~address();

		struct sockaddr *getAddr(void);
		struct sockaddr *find(struct sockaddr *addr);

		inline struct addrinfo *getList(void)
			{return list;};

		inline operator struct addrinfo *()
			{return list;};

		inline operator bool()
			{return list != NULL;};

		inline bool operator!()
			{return list == NULL;};

		inline operator struct sockaddr *()
			{return getAddr();};
	};

	Socket(const Socket &s);
	Socket(SOCKET so);
	Socket(struct addrinfo *addr);
	Socket(int family, int type, int protocol = 0);
	Socket(const char *iface, const char *port, int family, int type, int protocol = 0);
	virtual ~Socket();

	void release(void);
	bool isPending(unsigned qio) const;
	bool isConnected(void) const;
	bool waitPending(timeout_t timeout = 0) const;
	bool waitSending(timeout_t timeout = 0) const;
	
	inline unsigned getPending(void) const
		{return pending(so);};

	inline int broadcast(bool enable)
		{return broadcast(so, enable);};

	inline int keepalive(bool live)
		{return keepalive(so, live);};

	inline int blocking(bool enable)
		{return blocking(so, enable);};

	inline int multicast(unsigned ttl = 1)
		{return multicast(so, ttl);};

	inline int loopback(bool enable)
		{return loopback(so, enable);};

	inline int getError(void)
		{return error(so);};

	inline int ttl(unsigned char t)
		{return ttl(so, t);};

	inline int sendsize(unsigned size)
		{return sendsize(so, size);};

	inline int recvsize(unsigned size)
		{return recvsize(so, size);};

	inline int tos(int t)
		{return tos(so, t);};

	inline int priority(int pri)
		{return priority(so, pri);};

	inline void shutdown(void)
		{::shutdown(so, SHUT_RDWR);};

	inline int connect(struct addrinfo *list)
		{return connect(so, list);};

	bool create(int family, int type, int protocol = 0);
	
	inline int disconnect(void)
		{return disconnect(so);};

	inline int join(struct addrinfo *list)
		{return join(so, list);};

	inline int drop(struct addrinfo *list)
		{return drop(so, list);};

	size_t peek(void *data, size_t len) const;

	virtual ssize_t get(void *data, size_t len, struct sockaddr *from = NULL);

	virtual ssize_t put(const void *data, size_t len, struct sockaddr *to = NULL);

	virtual ssize_t gets(char *data, size_t max, timeout_t to = Timer::inf);

	ssize_t puts(const char *str);

	operator bool();

	bool operator!() const;

	Socket &operator=(SOCKET s);

	inline operator SOCKET() const
		{return so;};

	inline SOCKET operator*() const
		{return so;};

	static unsigned pending(SOCKET so);
	static int sendsize(SOCKET so, unsigned size);
	static int recvsize(SOCKET so, unsigned size);
	static int connect(SOCKET so, struct addrinfo *list);
	static int disconnect(SOCKET so);
	static int drop(SOCKET so, struct addrinfo *list);
	static int join(SOCKET so, struct addrinfo *list);
	static int error(SOCKET so);
	static int multicast(SOCKET so, unsigned ttl = 1); // zero disables
	static int loopback(SOCKET so, bool enable);
	static int blocking(SOCKET so, bool enable);
	static int keepalive(SOCKET so, bool live);
	static int broadcast(SOCKET so, bool enable);
	static int priority(SOCKET so, int pri);
	static int tos(SOCKET so, int tos);
	static int ttl(SOCKET so, unsigned char t);
	static int getfamily(SOCKET so);
	static int bindaddr(SOCKET so, const char *host, const char *svc);
	static char *gethostname(struct sockaddr *sa, char *buf, size_t max);
	static struct addrinfo *gethint(SOCKET so, struct addrinfo *h);
	static socklen_t getaddr(SOCKET so, struct sockaddr_storage *addr, const char *host, const char *svc);
	static socklen_t getlen(struct sockaddr *addr);
	static void copy(struct sockaddr *from, struct sockaddr *to);
	static bool equal(struct sockaddr *s1, struct sockaddr *s2);
	static bool subnet(struct sockaddr *s1, struct sockaddr *s2);
	static void getinterface(struct sockaddr *iface, struct sockaddr *dest);
	static char *getaddress(struct sockaddr *addr, char *buf, socklen_t size);
	static short getservice(struct sockaddr *addr);
	static unsigned keyindex(struct sockaddr *addr, unsigned keysize);
};

class __EXPORT ListenSocket : protected Socket
{
public:
	ListenSocket(const char *iface, const char *svc, unsigned backlog = 5);

	SOCKET accept(struct sockaddr *addr = NULL);

	inline bool waitConnection(timeout_t timeout = Timer::inf) const
		{return Socket::waitPending(timeout);};

    inline operator SOCKET() const
        {return so;};

    inline SOCKET operator*() const
        {return so;};
};

typedef	Socket socket_t;
typedef	Socket socket;

inline struct addrinfo *addrinfo(socket::address &a)
	{return a.getList();};

inline struct sockaddr *addr(socket::address &a)
	{return a.getAddr();};

END_NAMESPACE

#endif
