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

/**
 * Common socket class and address manipulation.
 * This offers a common socket base class that exposes socket functionality
 * based on what the target platform supports.  Support for multicast, IPV6
 * addressing, and manipulation of cidr policies are all supported here.
 * @file ucommon/socket.h
 */

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

/**
 * An object that can hold a ipv4 or ipv6 socket address.  This would be
 * used for tcpip socket connections.  We do not use sockaddr_storage
 * because it is not present in pre ipv6 stacks, and because the storage
 * size also includes the size of the path of a unix domain socket on
 * posix systems.
 */
struct sockaddr_internet;

/**
 * An object that holds ipv4 or ipv6 binary encoded host addresses.
 */
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

/**
 * A class to hold internet segment routing rules.  This class can be used
 * to provide a stand-alone representation of a cidr block of internet
 * addresses or chained together into some form of access control list.  The
 * cidr class can hold segments for both IPV4 and IPV6 addresses.  The class
 * accepts cidr's defined as C strings, typically in the form of address/bits
 * or address/submask.  These routines auto-detect ipv4 and ipv6 addresses.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT cidr : public LinkedObject
{
protected:
	int family;
	inethostaddr_t netmask, network;
	char name[16];
	unsigned getMask(const char *cp) const;

public:
	/**
	 * A convenience type for using a pointer to a linked list as a policy chain.
	 */
	typedef	LinkedObject policy;

	/**
	 * Create an uninitialized cidr.
	 */
	cidr();

	/**
	 * Create an unlinked cidr from a string.  The string is typically in
	 * the form base-host-address/range, where range might be a bit count
	 * or a network mask.
	 * @param string for cidr block.
	 */
	cidr(const char *string);
	
	/**
	 * Create an unnamed cidr entry on a specified policy chain.
	 * @param policy chain to link cidr to.
	 * @param string for cidr block.
	 */
	cidr(policy **policy, const char *string);

	/**
	 * Create a named cidr entry on a specified policy chain.
	 * @param policy chain to link cidr to.
	 * @param string for cidr block.
	 * @param name of this policy object.
	 */
	cidr(policy **policy, const char *string, const char *name);

	/**
	 * Construct a copy of an existing cidr.
	 * @param existing cidr we copy from.
	 */
	cidr(const cidr &existing);

	/**
	 * Find the smallest cidr entry in a list that matches the socket address.
	 * @param policy chain to search.
	 * @param address to search for.
	 * @return smallest cidr or NULL if none match.
	 */
	static cidr *find(policy *policy, const struct sockaddr *address);

	/**
	 * Get the saved name of our cidr.  This is typically used with find
	 * when the same policy name might be associated with multiple non-
	 * overlapping cidr blocks.  A typical use might to have a cidr
	 * block like 127/8 named "localdomain", as well as the ipv6 "::1".
	 * @return name of cidr.
	 */
	inline const char *getName(void) const
		{return name;};

	/**
	 * Get the address family of our cidr block object.
	 * @return family of our cidr.
	 */
	inline int getFamily(void) const
		{return family;};

	/**
	 * Get the network host base address of our cidr block.
	 * @return binary network host address.
	 */
	inline inethostaddr_t getNetwork(void) const
		{return network;};

	/**
	 * Get the effective network mask for our cidr block.
	 * @return binary network mask for our cidr.
	 */
	inline inethostaddr_t getNetmask(void) const
		{return netmask;};

	/**
	 * Get the broadcast host address represented by our cidr.
	 * @return binary broadcast host address.
	 */
	inethostaddr_t getBroadcast(void) const;

	/**
	 * Get the number of bits in the cidr bitmask.
	 * @return bit mask of cidr.
	 */
	unsigned getMask(void) const;
	
	/**
	 * Set our cidr to a string address.  Replaces prior value.
	 * @param string to set for cidr.
	 */	
	void set(const char *string);

	/**
	 * Test if a given socket address falls within this cidr.
	 * @param address of socket to test.
	 * @return true if address is within cidr.
	 */
	bool isMember(const struct sockaddr *address) const;

	/**
	 * Test if a given socket address falls within this cidr.
	 * @param address of socket to test.
	 * @return true if address is within cidr.
	 */
	inline bool operator==(const struct sockaddr *address) const
		{return isMember(address);};
	
	/**
	 * Test if a given socket address falls outside this cidr.
	 * @param address of socket to test.
	 * @return true if address is outside cidr.
	 */
	inline bool operator!=(const struct sockaddr *address) const
		{return !isMember(address);}; 
};

/**
 * A generic socket base class.  This class can be used directly or as a
 * base class for building network protocol stacks.  This common base tries
 * to handle UDP and TCP sockets, as well as support multicast, IPV4/IPV6
 * addressing, and additional addressing domains (such as Unix domain sockets).
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Socket 
{
protected:
	Socket();

	SOCKET so;

public:
	/**
	 * A generic socket address class.  This class uses the addrinfo list
	 * to store socket multiple addresses in a protocol and family
	 * independent manner.  Hence, this address class can be used for ipv4
	 * and ipv6 sockets, for assigning connections to multiple hosts, etc.
	 * The address class will call the resolver when passed host names.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT address
	{
	protected:
		struct addrinfo *list, hint;

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

		void add(const char *host, const char *svc = NULL, int family = 0);
		void add(struct sockaddr *addr);
	};

	Socket(const Socket &s);
	Socket(SOCKET so);
	Socket(struct addrinfo *addr);
	Socket(int family, int type, int protocol = 0);
	Socket(const char *iface, const char *port, int family, int type, int protocol = 0);
	virtual ~Socket();

	void cancel(void);
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
