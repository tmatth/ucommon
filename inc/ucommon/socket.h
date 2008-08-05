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

#include <errno.h>
#include <stdio.h>

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY      0x10
#define IPTOS_THROUGHPUT    0x08
#define IPTOS_RELIABILITY   0x04
#define IPTOS_MINCOST       0x02
#endif

#ifdef	AF_UNSPEC
#define	DEFAULT_FAMILY	AF_UNSPEC
#else
#define	DEFAULT_FAMILY	AF_INET
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

#if defined(AF_INET6) || defined(__CYGWIN__)
struct sockaddr_internet
{
	union {
#ifdef	AF_INET6
		struct sockaddr_in6 ipv6;
#endif
		struct sockaddr_in ipv4;
		struct sockaddr	address;
	};
};
#else
struct sockaddr_internet
{
	union {
		struct sockaddr_in ipv4;
		struct sockaddr address;
	};
};

struct sockaddr_storage
{
#ifdef	AF_UNIX
	char sa_data[128];
#else
	char sa_data[sizeof(struct sockaddr_in)];
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
	cidr(const cidr& existing);

	/**
	 * Find the smallest cidr entry in a list that matches the socket address.
	 * @param policy chain to search.
	 * @param address to search for.
	 * @return smallest cidr or NULL if none match.
	 */
	static cidr *find(policy *policy, const struct sockaddr *address);

	/**
	 * Get the largest container cidr entry in a list that matches the
	 * socket address.
	 * @param policy chain to search.
	 * @param address to search for.
	 * @return largest cidr or NULL if none match.
	 */
	static cidr *container(policy *policy, const struct sockaddr *address);

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
	socket_t so;

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
		struct addrinfo *list;

	public:
		/**
		 * Construct a socket address.
		 * @param address or hostname.
		 * @param family of socket address.  Needed when hostnames are used.
		 * @param type of socket (stream, dgram, etc).
		 * @param protocol number of socket.
		 */
		address(int family, const char *address, int type = SOCK_STREAM, int protocol = 0);

		/**
		 * Construct a socket address for an existing socket.
		 * @param socket to use for family of socket address.
		 * @param hostname or ip address.  The socket family is used for hostnames.
		 * @param service port or name we are referencing or NULL.
		 */
		address(Socket& socket, const char *hostname, const char *service = NULL);

		/**
		 * Construct a socket address for a socket descriptor.
		 * @param socket descriptor to use for family.
		 * @param hostname or address to use.
		 * @param service port or name we are referencing or NULL.
		 */
		address(socket_t socket, const char *hostname, const char *service = NULL);

		/**
		 * Construct a socket address from host and service.
		 * @param hostname or address to use.
		 * @param service port or 0.
		 * @param family of socket address.
		 */
		address(const char *hostname, unsigned service = 0, int family = DEFAULT_FAMILY);

		/**
		 * Construct an empty address.
		 */
		address();

		/**
		 * Copy constructor.
		 * @param reference to object to copy from.
		 */
		address(const address& reference);

		/**
		 * Destroy address.  Deallocate addrinfo structure.
		 */
		~address();

		/**
		 * Get the first socket address in our address list.
		 * @return first socket address or NULL if none.
		 */
		struct sockaddr *getAddr(void);

		/**
		 * Get the first socket address of specified family from our list.
		 * @param family to seek.
		 * @return first socket address of family or NULL if none.
		 */
		struct sockaddr *get(int family);

		/**
		 * Get the family of a socket address by first entry.
		 * @return family of first socket address or 0 if none.
		 */
		int getfamily(void);

		/**
		 * Find a specific socket address in our address list.
		 * @return matching address from list or NULL if not found.
		 */
		struct sockaddr *find(struct sockaddr *addr);

		/**
		 * Get the full socket address list from the object.
		 * @return addrinfo list we resolved or NULL if none.
		 */
		inline struct addrinfo *getList(void)
			{return list;};

		/**
		 * Get the full socket address list by casted reference.
		 * @return addrinfo list we resolved or NULL if none.
		 */
		inline operator struct addrinfo *()
			{return list;};

		/**
		 * Return the full socket address list by pointer reference.
		 * @return addrinfo list we resolved or NULL if none.
		 */
		inline struct addrinfo *operator*()
			{return list;};

		/**
		 * Test if the address list is valid.
		 * @return true if we have an address list.
		 */
		inline operator bool()
			{return list != NULL;};

		/**
		 * Test if we have no address list.
		 * @return true if we have no address list.
		 */
		inline bool operator!()
			{return list == NULL;};

		/**
		 * Get the first socket address by casted reference.
		 * @return first socket address we resolved or NULL if none.
		 */
		inline operator struct sockaddr *()
			{return getAddr();};

		/**
		 * Clear current object.
		 */
		void clear(void);

		/**
		 * Set the host addresses to form a new list.
		 * @param hostname or address to resolve.
		 * @param service name or port number, or NULL if not used.
		 * @param family of hostname.
		 * @param type of socket.
		 */
		void set(const char *hostname, const char *service = NULL, int family = 0, int type = SOCK_STREAM);

		/**
		 * Append additional host addresses to our list.
		 * @param hostname or address to resolve.
		 * @param service name or port number, or NULL if not used.
		 * @param family of hostname.
		 * @param type of socket.
		 */
		void add(const char *hostname, const char *service = NULL, int family = 0, int type = SOCK_STREAM);

		/**
		 * Set an entry for host binding.
		 * @param address or hostname.
		 * @param family of socket address.  Needed when hostnames are used.
		 * @param type of socket (stream, dgram, etc).
		 * @param protocol number of socket.
		 */
		void set(int family, const char *address, int type = SOCK_STREAM, int protocol = 0);

		/**
		 * Add an individual socket address to our address list.
		 * @param address to add.
		 */
		void add(sockaddr *address);

		/**
		 * Remove an individual socket address from our address list.
		 * @param address to remove.
		 * @return true if found and removed, false if not found.
		 */
		bool remove(struct sockaddr *address);

		/**
		 * Insert an individual socket address to our address list only if
		 * unique.
		 * @param address to insert into list.
		 * @return true if inserted, false if duplicate.
		 */
		bool insert(struct sockaddr *address);

		/**
		 * Insert unique members from another socket address list to ours.
		 * @param address list to insert into list.
		 * @param address family to scan for or 0 for all.
		 * @return count of addresses added.
		 */
		unsigned insert(struct addrinfo *address, int family = 0);

		/**
		 * Remove members from another socket address list from ours.
		 * @param address list to remove from list.
		 * @param address family to scan for or 0 for all.
		 * @return count of addresses removed.
		 */
		unsigned remove(struct addrinfo *address, int family = 0);

		/**
		 * Copy an existing addrinfo into our object.  This is also used
		 * to support the copy constructor.
		 * @param address list to copy from.
		 */
		void copy(const struct addrinfo *address);

		/**
		 * Set an individual socket address for our address list.
		 * @param address to add.
		 */
		void set(struct sockaddr *address);

		/**
		 * Set a socket address from host and service.
		 * @param hostname or address to use.
		 * @param service port or 0.
		 * @param family of socket address.
		 */
		void set(const char *hostname, unsigned service = 0, int family = DEFAULT_FAMILY);

		/**
		 * Duplicate a socket address.
		 * @param address to duplicate.
		 * @return duplicate address object.
		 */
		static struct sockaddr *dup(struct sockaddr *address);

		/**
		 * Convert address object into ipv4 address.
		 * @param address to convert.
		 * @return new ipv4 address or NULL if not ipv4.
		 */
		static struct sockaddr_in *ipv4(struct sockaddr *address);

#ifdef	AF_INET6
		/**
		 * Convert address object into ipv6 address.
		 * @param address to convert.
		 * @return new ipv6 address or NULL if not ipv6.
		 */
		static struct sockaddr_in6 *ipv6(struct sockaddr *address);
#endif
	};

	friend class address;

	/**
	 * Create a socket object for use.
	 */
	Socket();

	/**
	 * Create socket as duped handle of existing socket.
	 * @param existing socket to dup.
	 */
	Socket(const Socket& existing);

	/**
	 * Create socket from existing socket descriptor.
	 * @param socket descriptor to use.
	 */
	Socket(socket_t socket);

	/**
	 * Create and connect a socket to an address from an address list.  The
	 * type of socket created is based on the type we are connecting to.
	 * @param address list to connect with.
	 */ 
	Socket(struct addrinfo *address);

	/**
	 * Create an unbound socket of a specific type.
	 * @param family of our new socket.
	 * @param type (stream, udp, etc) of our new socket.
	 * @param protocol number of our new socket.'
	 */
	Socket(int family, int type, int protocol = 0);

	/**
	 * Create a bound socket.
	 * @param address to bind or "*" for all
	 * @param port number of service to bind.
	 * @param family to bind as.
	 * @param type of socket to bind (stream, udp, etc).
	 * @param protocol of socket to bind.
	 * @param backlog for listening socket.
	 */
	Socket(const char *address, const char *port, int family = AF_UNSPEC, int type = 0, int protocol = 0, int backlog = 0);

	/**
	 * Shutdown, close, and destroy socket.
	 */
	virtual ~Socket();

	/**
	 * Cancel pending i/o by shutting down the socket.
	 */
	void cancel(void);

	/**
	 * Cancel pending i/o by shutting down the socket.
	 * @param socket to shutdown.
	 */
	static void cancel(socket_t socket);

	/**
	 * Shutdown and close the socket.
	 */
	void release(void);

	/**
	 * See the number of bytes in the receive queue.
	 * @param value to test for.
	 * @return true if at least that many bytes waiting in receive queue.
	 */
	bool isPending(unsigned value) const;

	/**
	 * Test if socket is connected.
	 * @return true if connected.
	 */
	bool isConnected(void) const;

	/**
	 * Test for pending input data.  This function can wait up to a specified
	 * timeout for data to appear.
	 * @param timeout or 0 if none.
	 * @return true if input data waiting.
	 */
	bool waitPending(timeout_t timeout = 0) const;

	/**
	 * Test for pending input data.  This function can wait up to a specified
	 * timeout for data to appear.
	 * @param socket to test.
	 * @param timeout or 0 if none.
	 * @return true if input data waiting.
	 */
	static bool wait(socket_t socket, timeout_t timeout = 0);

	/**
	 * Test for output data sent.  This function can wait up to a specified
	 * timeout for data to appear sent.
	 * @param timeout or 0 if none.
	 * @return false if cannot send more output/out of buffer space.
	 */
	bool waitSending(timeout_t timeout = 0) const;
	
	/**
	 * Get the number of bytes of data in the socket receive buffer.
	 * @return bytes pending.
	 */
	inline unsigned getPending(void) const
		{return pending(so);};

	/**
	 * Set socket for unicast mode broadcasts.
	 * @param enable broadcasting if true.
	 * @return 0 on success, -1 if error.
	 */
	inline int broadcast(bool enable)
		{return broadcast(so, enable);};

	/**
	 * Set socket for keepalive packets.
	 * @param enable keep-alive if true.
	 * @return 0 on success, -1 if error.
	 */
	inline int keepalive(bool enable)
		{return keepalive(so, enable);};

	/**
	 * Set socket blocking I/O mode.
	 * @param enable true for blocking I/O.
	 * @return 0 on success, -1 if error.
	 */ 
	inline int blocking(bool enable)
		{return blocking(so, enable);};

	/**
	 * Set multicast mode and multicast broadcast range.
	 * @param ttl to set for multicast socket or 0 to disable multicast.
	 * @return 0 on success, -1 if error.
	 */
	inline int multicast(unsigned ttl = 1)
		{return multicast(so, ttl);};

	/**
	 * Set loopback to read multicast packets we broadcast.
	 * @param enable true to loopback, false to ignore.
	 * @return 0 on success, -1 if error.
	 */
	inline int loopback(bool enable)
		{return loopback(so, enable);};

	/**
	 * Get socket error code.
	 * @return socket error code.
	 */
	inline int getError(void)
		{return error(so);};

	/**
	 * Set the time to live before packets expire.
	 * @param time to live to set.
	 * @return 0 on success, -1 on error.
	 */
	inline int ttl(unsigned char time)
		{return ttl(so, time);};

	/**
	 * Set the size of the socket send buffer.
	 * @param size of send buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	inline int sendsize(unsigned size)
		{return sendsize(so, size);};

	/**
	 * Set the size to wait before sending.
	 * @param size of send wait buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	inline int sendwait(unsigned size)
		{return sendwait(so, size);};


	/**
	 * Set the size of the socket receive buffer.
	 * @param size of recv buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	inline int recvsize(unsigned size)
		{return recvsize(so, size);};

	/**
	 * Get the type of a socket.
	 * @param socket descriptor.
	 * @return socket type.
	 */
	static int gettype(socket_t socket);

	/**
	 * Get the type of a socket.
	 * @return socket type.
	 */
	inline int gettype(void)
		{return gettype(so);};

	/**
	 * Set the type of service field of outgoing packets.  Some useful
	 * values include IPTOS_LOWDELAY to minimize delay for interactive 
	 * traffic, IPTOS_THROUGHPUT to optimize thoughput, OPTOS_RELIABILITY
	 * to optimize for reliability, and IPTOS_MINCOST for low speed use.
	 * @param type of service value.
	 * @return 0 on success or -1 on error.
	 */
	inline int tos(int type)
		{return tos(so, type);};

	/**
	 * Set packet priority, 0 to 6 unless privileged.  Should be set before
	 * type-of-service.
	 * @param scheduling priority for packet scheduling.
	 * @return 0 on success, -1 on error.
	 */
	inline int priority(int scheduling)
		{return priority(so, scheduling);};

	/**
	 * Shutdown the socket communication channel.
	 */
	inline void shutdown(void)
		{::shutdown(so, SHUT_RDWR);};

	/**
	 * Connect our socket to a remote host from an address list.
	 * For TCP sockets, the entire list may be tried.  For UDP, connect
	 * is only a state and the first valid entry in the list is used.
	 * @param list of addresses to connect to.
	 * @return 0 on success, -1 on error.
	 */
	inline int connectto(struct addrinfo *list)
		{return connectto(so, list);};
	
	/**
	 * Disconnect a connected socket.  Depending on the implimentation, this
	 * might be done by connecting to AF_UNSPEC, connecting to a 0 address,
	 * or connecting to self.
	 * @return 0 on success, -1 on error.
	 */
	inline int disconnect(void)
		{return disconnect(so);};

	/**
	 * Join socket to multicast group.
	 * @param list of groups to join.
	 * @return 0 on success, -1 on error.
	 */
	inline int join(struct addrinfo *list)
		{return join(so, list);};

	/**
	 * Drop socket from multicast group.
	 * @param list of groups to drop.
	 * @return 0 on success, -1 on error.
	 */
	inline int drop(struct addrinfo *list)
		{return drop(so, list);};

	/**
	 * Peek at data waiting in the socket receive buffer.
	 * @param data pointer to save data in.
	 * @param number of bytes to peek.
	 * @return number of bytes actually read, or 0 if no data waiting.
	 */
	size_t peek(void *data, size_t number) const;

	/**
	 * Read data from the socket receive buffer.  This is a virtual so that
	 * the ssl layer can override the core get method.
	 * @param data pointer to save data in.
	 * @param number of bytes to read.
	 * @param address of peer data was received from.
	 * @return number of bytes actually read, 0 if none, -1 if error.
	 */
	virtual ssize_t get(void *data, size_t number, struct sockaddr_storage *address = NULL);

	/**
	 * Write data to the socket send buffer.  This is a virtual so that the ssl
	 * layer can override the core put method.
	 * @param data pointer to write data from.
	 * @param number of bytes to write.
	 * @param address of peer to send data to if not connected.
	 * @return number of bytes actually sent, 0 if none, -1 if error.
	 */
	virtual ssize_t put(const void *data, size_t number, struct sockaddr *address = NULL);

	/**
	 * Read a newline of text data from the socket and save in NULL terminated
	 * string.  This uses an optimized I/O method that takes advantage of
	 * socket peeking.  As such, it has to be rewritten to be used in a ssl
	 * layer socket.
	 * @param data to save input line.
	 * @param size of input line buffer.
	 * @param timeout to wait for a complete input line.
	 * @return number of bytes read, 0 if none, -1 if error.
	 */
	virtual ssize_t gets(char *data, size_t size, timeout_t timeout = Timer::inf);

	/**
	 * Read a newline of text data from the socket and save in NULL terminated
	 * string.  This uses an optimized I/O method that takes advantage of
	 * socket peeking.  As such, it has to be rewritten to be used in a ssl
	 * layer socket.
	 * @param socket to read from.
	 * @param data to save input line.
	 * @param size of input line buffer.
	 * @param timeout to wait for a complete input line.
	 * @return number of bytes read, 0 if none, -1 if error.
	 */
	static ssize_t readline(socket_t socket, char *data, size_t size, timeout_t timeout = Timer::inf);

	/**
	 * Print formatted string to socket.
	 * @param socket to write to.
	 * @param format string.
	 * @return number of bytes sent, -1 if error.
	 */
	static ssize_t printf(socket_t socket, const char *format, ...) __PRINTF(2,3);

	/**
	 * Write a null terminated string to the socket.
	 * @param string to write.
	 * @return number of bytes sent, 0 if none, -1 if error.
	 */
	ssize_t puts(const char *string);

	/**
	 * Test if socket is valid.
	 * @return true if valid socket.
	 */
	operator bool();

	/**
	 * Test if socket is invalid.
	 * @return true if socket is invalid.
	 */
	bool operator!() const;

	/**
	 * Assign socket from a socket descriptor.  Release existing socket if
	 * one present.
	 * @param socket descriptor to assign to object.
	 */
	Socket& operator=(socket_t socket);

	/**
	 * Get the socket descriptor by casting.
	 * @return socket descriptor of object.
	 */
	inline operator socket_t() const
		{return so;};

	/**
	 * Get the socket descriptor by pointer reference.
	 * @return socket descriptor of object.
	 */
	inline socket_t operator*() const
		{return so;};

	/**
	 * Get the number of bytes pending in the receive buffer of a socket
	 * descriptor.
	 * @param socket descriptor.
	 * @return number of pending bytes.
	 */
	static unsigned pending(socket_t socket);

	/**
	 * Set the send size of a socket descriptor.
	 * @param socket descriptor.
	 * @param size of send buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	static int sendsize(socket_t socket, unsigned size);

	/**
	 * Set the size to wait before sending.
	 * @param socket descriptor.
	 * @param size of send wait buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	static int sendwait(socket_t socket, unsigned size);

	/**
	 * Set the receive size of a socket descriptor.
	 * @param socket descriptor.
	 * @param size of receive buffer to set.
	 * @return 0 on success, -1 on error.
	 */
	static int recvsize(socket_t socket, unsigned size);

	/**
	 * Connect socket descriptor to a remote host from an address list.
	 * For TCP sockets, the entire list may be tried.  For UDP, connect
	 * is only a state and the first valid entry in the list is used.
	 * @param socket descriptor.
	 * @param list of addresses to connect to.
	 * @return 0 on success, -1 on error.
	 */
	static int connectto(socket_t socket, struct addrinfo *list);

	/**
	 * Disconnect a connected socket descriptor.
	 * @param socket descriptor.
	 * @return 0 on success, -1 on error.
	 */
	static int disconnect(socket_t socket);

	/**
	 * Drop socket descriptor from multicast group.
	 * @param socket descriptor.
	 * @param list of groups to drop.
	 * @return 0 on success, -1 on error.
	 */
	static int drop(socket_t socket, struct addrinfo *list);

	/**
	 * Join socket descriptor to multicast group.
	 * @param socket descriptor.
	 * @param list of groups to join.
	 * @return 0 on success, -1 on error.
	 */
	static int join(socket_t socket, struct addrinfo *list);

	/**
	 * Get socket error code of socket descriptor.
	 * @param socket descriptor.
	 * @return socket error code.
	 */
	static int error(socket_t socket);

	/**
	 * Set multicast mode and multicast broadcast range for socket descriptor.
	 * @param socket descriptor.
	 * @param ttl to set for multicast socket or 0 to disable multicast.
	 * @return 0 if success, -1 if error.
	 */
	static int multicast(socket_t socket, unsigned ttl = 1);

	/**
	 * Set loopback to read multicast packets socket descriptor broadcasts.
	 * @param socket descriptor.
	 * @param enable true to loopback, false to ignore.
	 * @return 0 if success, -1 if error.
	 */
	static int loopback(socket_t socket, bool enable);

	/**
	 * Set socket blocking I/O mode of socket descriptor.
	 * @param socket descriptor.
	 * @param enable true for blocking I/O.
	 * @return 0 if success, -1 if error.
	 */ 
	static int blocking(socket_t socket, bool enable);

	/**
	 * Set socket for keepalive packets for socket descriptor.
	 * @param socket descriptor.
	 * @param enable keep-alive if true.
	 * @return 0 if success, -1 if error.
	 */
	static int keepalive(socket_t socket, bool enable);

	/**
	 * Set socket for unicast mode broadcasts on socket descriptor.
	 * @param socket descriptor.
	 * @param enable broadcasting if true.
	 * @return 0 if success, -1 if error.
	 */
	static int broadcast(socket_t socket, bool enable);

	/**
	 * Set packet priority of socket descriptor.
	 * @param socket descriptor.
	 * @param scheduling priority for packet scheduling.
	 * @return 0 on success, -1 on error.
	 */
	static int priority(socket_t socket, int scheduling);

	/**
	 * Set type of service of socket descriptor.
	 * @param socket descriptor.
	 * @param type of service.
	 * @return 0 on success, -1 on error.
	 */
	static int tos(socket_t socket, int type);

	/**
	 * Set the time to live for the socket descriptor.
	 * @param socket descriptor.
	 * @param time to live to set.
	 * @return 0 on success, -1 on error.
	 */
	static int ttl(socket_t socket, unsigned char time);
	
	/**
	 * Get the address family of the socket descriptor.
	 * @return address family.
	 */
	static int getfamily(socket_t socket);

	/**
	 * Get the address family of a socket address object.
	 * @param address to examine.
	 * @return address family.
	 */
	inline static int getfamily(struct sockaddr_storage& address)
		{return ((struct sockaddr *)&address)->sa_family;};

	/**
	 * Get the address family of an internet socket address object.
	 * @param address to examine.
	 * @return address family.
	 */
	inline static int getfamily(struct sockaddr_internet& address)
		{return address.address.sa_family;};

	/**
	 * Get data waiting in receive queue.
	 * @param socket to get from.
	 * @param buffer to save.
	 * @param size of data buffer to request.
	 * @param flags for i/o operation (MSG_OOB, MSG_PEEK, etc).
	 * @param address of source.
	 * @return number of bytes received, -1 if error.
	 */
	static ssize_t recvfrom(socket_t socket, void *buffer, size_t size, int flags = 0, struct sockaddr_storage *address = NULL);

	/**
	 * Send data on socket.
	 * @param socket to send to.
	 * @param buffer to send.
	 * @param size of data buffer to send.
	 * @param flags for i/o operation (MSG_OOB, MSG_PEEK, etc).
	 * @param address of destination, NULL if connected.
	 * @return number of bytes sent, -1 if error.
	 */
	static ssize_t sendto(socket_t socket, const void *buffer, size_t size, int flags = 0, struct sockaddr *address = NULL);

	/**
	 * Bind the socket descriptor to a known interface and service port.
	 * @param socket descriptor to bind.
	 * @param address to bind to or "*" for all.
	 * @param service port to bind.
	 * @param protocol to use or 0 if default.
	 * @return 0 on success, -1 if error.
	 */
	static int bindto(socket_t socket, const char *address, const char *service, int protocol = 0);

	/**
	 * Bind the socket descriptor to a known interface listen on service port.
	 * @param socket descriptor to bind.
	 * @param address of interface to bind to.
	 * @param backlog for service.
	 * @return 0 on success, -1 if error.
	 */
	static int listento(socket_t socket, struct sockaddr *address, int backlog = 5);

	/**
	 * Bind the socket descriptor to a known interface.
	 * @param socket descriptor to bind.
	 * @param address of interface to bind to.
	 * @return 0 on success, -1 if error.
	 */
	static int bindto(socket_t socket, struct sockaddr *address);

	/**
	 * Accept a socket connection from a remote host.
	 * @param socket descriptor to accept from.
	 * @param address of socket accepting.
	 * @return new socket accepted.
	 */
	static socket_t acceptfrom(socket_t socket, struct sockaddr_storage *address = NULL);

	/**
	 * Create a socket object unbound.
	 * @param family of socket.
	 * @param type of socket.
	 * @param protocol of socket.
	 * @return socket descriptor created or INVALID_SOCKET.
	 */
	static socket_t create(int family, int type, int protocol);

	/**
	 * Create a bound socket for a service.
	 * @param iface to bind.
	 * @param service port to bind.
	 * @param family to select or AF_UNSPEC
	 * @param type of socket to create.
	 * @param protocol of socket to create.
	 * @param backlog for listener.
	 * @return socket desciptor created or INVALID_SOCKET.
	 */
	static socket_t create(const char *iface, const char *service, int family = AF_UNSPEC, int type = 0, int protocol = 0, int backlog = 0);

	/**
	 * Create a connected socket for a service.
	 * @param address of service for connect.
	 * @return socket desciptor.
	 */
	static socket_t create(Socket::address &address);

	/**
	 * Release (close) a socket.
	 * @param socket to close.
	 */
	static void release(socket_t socket);

	/**
	 * Send to socket object.
	 * @param socket object to send thru.
	 * @param buffer to send.
	 * @param size of buffer to send.
	 * @param address to send to or NULL if connected.
	 * @return number of bytes send, -1 if error.
	 */
	inline static ssize_t sendto(Socket& socket, const char *buffer, size_t size, struct sockaddr *address)
		{return socket.put(buffer, size, address);};
	
	/**
	 * receive from socket object.
	 * @param socket object to recv from.
	 * @param buffer to recv.
	 * @param size of buffer to recv.
	 * @param address receving from or NULL if connected.
	 * @return number of bytes received, -1 if error.
	 */
	inline static ssize_t recvfrom(Socket& socket, char *buffer, size_t size, struct sockaddr_storage *address)
		{return socket.get(buffer, size, address);};

	/**
	 * Connect a socket.
	 * @param socket object to connect.
	 * @param address list to connect to.
	 */
	inline static void connectto(Socket& socket, Socket::address &address)
		{socket.connectto(address);};

	/**
	 * Disconnect a connected socket.
	 * @param socket object to disconnect.
	 */
	inline static void disconnect(Socket& socket)
		{socket.disconnect();};

	/**
	 * Accept connection through socket.
	 * @param socket object to accept from.
	 * @param address accepting from.
	 * @return socket accepted.
	 */
	inline static Socket acceptfrom(Socket& socket, struct sockaddr_storage *address)
		{return Socket(acceptfrom(socket.so, address));};

	/**
	 * Lookup and return the host name associated with a socket address.
	 * @param address to lookup.
	 * @param buffer to save hostname into.
	 * @param size of buffer to save hostname into.
	 * @return buffer or NULL if lookup fails.
	 */
	static char *gethostname(struct sockaddr *address, char *buffer, size_t size);

	/**
	 * Create an address info lookup hint based on the family and type
	 * properties of a socket descriptor.
	 * @param socket descriptor.
	 * @param hint buffer.
	 * @return hint buffer.
	 */
	static struct addrinfo *gethint(socket_t socket, struct addrinfo *hint);

	/**
	 * Lookup a host name and service address based on the addressing family
	 * and socket type of a socket descriptor.  Store the result in a socket
	 * address structure.
	 * @param socket descriptor.
	 * @param address that is resolved.
	 * @param hostname to resolve.
	 * @param service port.
	 * @return socket address size.
	 */
	static socklen_t getaddr(socket_t socket, struct sockaddr_storage *address, const char *hostname, const char *service);

	/**
	 * Get the size of a socket address.
	 * @param address of socket.
	 * @return size to use for this socket address object.
	 */
	static socklen_t getlen(struct sockaddr *address);

	/**
	 * Copy a socket address from one structure to another.  The size of
	 * the address is determined by getlen.
	 * @param from address of original.
	 * @param to address to save.
	 */
	static void copy(struct sockaddr *from, struct sockaddr *to);

	/**
	 * Compare socket addresses.  Test if the address and service matches
	 * or if there is no service, then just the host address values.
	 * @param address1 to compare.
	 * @param address2 to compare.
	 * @return true if same family and equal.
	 */
	static bool equal(struct sockaddr *address1, struct sockaddr *address2);

	/**
	 * Compare socket host addresses.  Test if the host addressmatches
	 * or if there is no service, then just the host address values.
	 * @param address1 to compare.
	 * @param address2 to compare.
	 * @return true if same family and equal.
	 */
	static bool equalhost(struct sockaddr *address1, struct sockaddr *address2);


	/**
	 * See if both addresses are in the same subnet.  This is only relevent
	 * to IPV4 and class domain routing.
	 * @param address1 to test.
	 * @param address2 to test.
	 * @return true if in same subnet.
	 */
	static bool subnet(struct sockaddr *address1, struct sockaddr *address2);

	/**
	 * Get the socket address of the interface needed to reach a destination
	 * address.
	 * @param address of interface found.
	 * @param destination address.
	 * @return 0 on success, -1 on error.
	 */
	static int getinterface(struct sockaddr *address, struct sockaddr *destination);

	/**
	 * Get the hostname of a socket address.
	 * @param address to lookup.
	 * @param buffer to save hostname in.
	 * @param size of hostname buffer.
	 * @return buffer if found or NULL if not.
	 */
	static char *getaddress(struct sockaddr *address, char *buffer, socklen_t size);

	/**
	 * Get the service port of a socket.
	 * @param address of socket to examine.
	 * @return service port number.
	 */
	static short getservice(struct sockaddr *address);

	/**
	 * Convert a socket address and service into a hash map index.
	 * @param address to convert.
	 * @param size of map index.
	 * @return key index path.
	 */
	static unsigned keyindex(struct sockaddr *address, unsigned size);

	/**
	 * Convert a socket host address into a hash map index.
	 * @param address to convert.
	 * @param size of map index.
	 * @return key index path.
	 */
	static unsigned keyhost(struct sockaddr *address, unsigned size);


	/**
	 * Initialize socket subsystem.
	 */
	static void init(void);

	/**
	 * Set default socket family preference for query options when the
	 * socket type is otherwise not specified.
	 * @param query family to select.
	 */
	static void family(int query);

	/**
	 * Set the default socket behavior for v6-v4 mapping.  This also
	 * effects v6 address lookup as to whether v4 remapped addresses
	 * can be used if no v6 address is found.
	 * @param enable true to set mapping.  This is default.
	 */
	static void v4mapping(bool enable);

	/**
	 * Convert socket into FILE handle for reading.
	 * @param socket descriptor to convert.
	 * @param mode of access, true for write mode.
	 * @return file handle to use.
	 */
	static FILE *open(socket_t socket, bool mode = false);

	/**
	 * Get file handle for reading from a socket object.
	 * @param mode of access, true for write mode.
	 * @return file handle.
	 */
	inline FILE *open(bool mode = false)
		{return open(so, mode);};

	/**
	 * Cleanly close a connected socket descriptor mapped to a file handle.
	 * @param file handle to close.
	 */
	static void close(FILE *file);

	/**
	 * Return error code of last socket operation,
	 * @return errno style error code.
	 */
	static int error(void);

	/**
	 * Simple function to validate that a given IP address string is a "zero"
	 * address.  Such address strings are used for example in SIP to indicate
	 * "hold" by re-inviting peers to a null address.  Supports IPV4 and
	 * IPV6 addresses.
	 * @param address string to check.
	 * @return true if zero/null address.
	 */
	static bool isNull(const char *string);

	/**
	 * Simple function to validate that a given IP address string is a numeric
	 * address.  This can be used to verify an address is not a "host" name.
	 * Supports IPV4 and IPV6 address strings.
	 * @param address string to check.
	 * @return true if zero/null address.
	 */
	static bool isNumeric(const char *string);
};

/**
 * A bound socket used to listen for inbound socket connections.  This class 
 * is commonly used for TCP listener sockets.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT ListenSocket : protected Socket
{
public:
	/**
	 * Create and bind a listener socket.
	 * @param address to bind on or "*" for all.
	 * @param service port to bind listener.
	 * @param backlog size for buffering pending connections.
	 * @param protocol for socket if not TCPIP.
	 */
	ListenSocket(const char *address, const char *service, unsigned backlog = 5, int protocol = 0);

	/**
	 * Accept a socket connection.
	 * @param address to save peer connecting.  
	 * @return socket descriptor of connected socket.
	 */
	socket_t accept(struct sockaddr_storage *address = NULL);

	/**
	 * Wait for a pending connection.
	 * @param timeout to wait.
	 * @return true when acceptable connection is pending.
	 */
	inline bool waitConnection(timeout_t timeout = Timer::inf) const
		{return Socket::waitPending(timeout);};

	/**
	 * Get the socket descriptor of the listener.
	 * @return socket descriptor.
	 */
    inline operator socket_t() const
        {return so;};

	/**
	 * Get the socket descriptor of the listener by pointer reference.
	 * @return socket descriptor.
	 */
    inline socket_t operator*() const
        {return so;};	
};

/**
 * A convenience class for socket.
 */
typedef	Socket socket;

/**
 * A convenience function to convert a socket address list into an addrinfo.
 * @param address list object.
 * @return addrinfo list or NULL if empty.
 */
inline struct addrinfo *addrinfo(socket::address& address)
	{return address.getList();};

/**
 * A convenience function to convert a socket address list into a socket 
 * address.
 * @param address list object.
 * @return first socket address in list or NULL if empty.
 */
inline struct sockaddr *addr(socket::address& address)
	{return address.getAddr();};

END_NAMESPACE

#endif
