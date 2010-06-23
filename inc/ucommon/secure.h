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

/**
 * This library holds basic crytographic functions and secure socket support
 * for use with GNU uCommon C++.  This library might be used in conjunction
 * with openssl, gnutls, etc.  If no secure socket library is available, then
 * a stub library may be used with very basic cryptographic support.
 * @file ucommon/secure.h
 */

/**
 * Example of SSL socket code.
 * @example ssl.cpp
 */

/**
 * Example of crytographic digest code.
 * @example digest.cpp
 */

/**
 * Example of cipher code.
 * @example cipher.cpp
 */

#ifndef _UCOMMON_SECURE_H_
#define _UCOMMON_SECURE_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

#ifndef _UCOMMON_UCOMMON_H_
#include <ucommon/ucommon.h>
#endif

#define	MAX_CIPHER_KEYSIZE	512
#define	MAX_DIGEST_HASHSIZE	512

NAMESPACE_UCOMMON

/**
 * Common secure socket support.  This offers common routines needed for
 * secure/ssl socket support code.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT secure
{
public:
	/**
	 * Different error states of the security context.
	 */
	typedef enum {OK=0, INVALID, MISSING_CERTIFICATE, MISSING_PRIVATEKEY, INVALID_CERTIFICATE, INVALID_AUTHORITY, INVALID_PEERNAME, INVALID_CIPHER} error_t;

protected:
	/**
	 * Last error flagged for this context.
	 */
	error_t error;

	inline secure() {error = OK;};

public:
	/**
	 * This is derived in different back-end libraries, and will be used to
	 * clear certificate credentials.
	 */
	virtual ~secure();

	/**
	 * Convenience type to represent a security context.
	 */
	typedef	secure *context_t;

	/**
	 * Convenience type to represent a secure socket session.
	 */
	typedef	void *session_t;

	/**
	 * Covenience type to represent a secure socket buf i/o stream.
	 */
	typedef	void *bufio_t;

	/**
	 * Initialize secure stack for first use, and report if SSL support is
	 * compiled in.  This allows a program name to be passed, which may be
	 * used for some proxy systems.
	 * @param program name we are initializing for.
	 * @return true if ssl support is available, false if not.
	 */
	static bool init(const char *program = NULL);

	/**
	 * Verify a certificate chain through your certificate authority.
	 * This uses the ca loaded as an optional argument for client and
	 * server.  Optionally the hostname of the connection can also be
	 * verified by pulling the peer certificate.
	 * @param session that is connected.
	 * @param peername that we expect.
	 * @return secure error level or secure::OK if none.
	 */
	static error_t verify(session_t session, const char *peername = NULL);

	/**
	 * Create a sever context.  The certificate file used will be based on
	 * the init() method name.  This may often be /etc/ssl/certs/initname.pem.
	 * Similarly, a matching private key certificate will also be loaded.  An
	 * optional certificate authority document can be used when we are
	 * establishing a service which ssl clients have their own certificates.
	 * @param authority path to use or NULL if none.
	 * @return a security context that is cast from derived library.
	 */
	static context_t server(const char *authority = NULL);

	/**
	 * Create an anonymous client context with an optional authority to 
	 * validate.
	 * @param authority path to use or NULL if none.
	 * @return a basic client security context.
	 */
	static context_t client(const char *authority = NULL);

	/**
	 * Create a peer user client context.  This assumes a user certificate
	 * in ~/.ssl/certs and the user private key in ~/.ssl/private.  The
	 * path to an authority is also sent.
	 * @param authority path to use.
	 */
	static context_t user(const char *authority);

	/**
	 * Assign a non-default cipher to the context.
	 * @param context to set cipher for.
	 * @param ciphers to set.
	 */
	static void cipher(context_t context, const char *ciphers);

	/**
	 * Determine if the current security context is valid.
	 * @return true if valid, -1 if not.
	 */
	inline bool is(void)
		{return error == OK;};

	/**
	 * Get last error code associated with the security context.
	 * @return last error code or 0/OK if none.
	 */
	inline error_t err(void)
		{return error;};
};

/**
 * Secure socket class.  This is used to create ssl socket connections
 * for both clients and servers.  The use depends in part on the type of
 * context created and passed at construction time.  If no context is
 * passed (NULL), then this reverts to TCPSocket behavior.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT SSocket : public TCPSocket
{
protected:
	secure::session_t ssl;
	secure::bufio_t bio;
	bool verify;

public:
	SSocket(const char *service, secure::context_t context);
	SSocket(TCPServer *server, secure::context_t context, size_t size = 536);
	~SSocket();

	/**
	 * Connect a ssl client session to a specific host uri.  If the socket
	 * was already connected, it is automatically closed first.
	 * @param host and optional :port we are connecting to.
	 * @param size of buffer and tcp fragments.
	 */
	void open(const char *host, size_t size = 536);

	void open(TCPServer *server, size_t size = 536);

	void close(void);

	bool flush(void);

	void release(void);

	size_t _push(const char *address, size_t size);

	size_t _pull(char *address, size_t size);

	bool issecure(void)
		{return bio != NULL;};
};

/**
 * A generic data ciphering class.  This is used to construct crytographic
 * ciphers to encode and decode data as needed.  The cipher type is specified
 * by the key object.  This class can be used to send output streaming to
 * memory or in a fixed size buffer.  If the latter is used, a push() method
 * is called through a virtual when the buffer is full.  Since block ciphers
 * are used, buffers should be aligned to the block size.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Cipher
{
public:
	typedef	enum {ENCRYPT = 1, DECRYPT = 0} mode_t;

	/**
	 * Cipher key formed by hash algorithm.  This can generate both a
	 * key and iv table based on the algorithms used and required.  Normally
	 * it is used from a pass-phrase, though any block of data may be
	 * supplied.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT Key
	{
	private:
		friend class Cipher;
	
		const void *algotype;
		const void *hashtype;
		
		// assume 512 bit cipher keys possible...
		unsigned char keybuf[MAX_CIPHER_KEYSIZE / 8], ivbuf[MAX_CIPHER_KEYSIZE / 8];

		// generated keysize
		unsigned keysize, blksize;

	public:
		Key(const char *cipher, const char *digest, const char *text, size_t size = 0, const unsigned char *salt = NULL, unsigned rounds = 1);
		Key();

		inline unsigned size(void)
			{return keysize;};

		inline unsigned iosize(void)
			{return blksize;};
	};

	typedef	Key *key_t;

private:
	Key keys;
	size_t bufsize, bufpos;
	mode_t bufmode;
	unsigned char *bufaddr;
	void *context;
	
protected:
	virtual void push(unsigned char *address, size_t size);

	void release(void);

public:
	Cipher();

	Cipher(key_t key, mode_t mode, unsigned char *address, size_t size = 0);

	~Cipher();

	void set(key_t key, mode_t mode, unsigned char *address, size_t size = 0);

	size_t flush(void);

	size_t put(const unsigned char *data, size_t size);

	size_t puts(const char *str);
		
	inline size_t size(void)
		{return bufsize;};

	inline size_t pos(void)
		{return bufpos;};

	size_t alignment(void);
};

/**
 * A crytographic digest class.  This class can support md5 digests, sha1,
 * sha256, etc, depending on what the underlying library supports.  The
 * hash class accumulates the hash in the object.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Digest
{
private:
	void *context;
	const void *hashtype;
	unsigned bufsize;
	unsigned char buffer[MAX_DIGEST_HASHSIZE / 8];
	char text[MAX_DIGEST_HASHSIZE / 8 + 1];
	
protected:
	void release(void);

public:
	Digest(const char *type);

	Digest();

	~Digest();

	inline bool puts(const char *str)
		{return put(str, strlen(str));};

	bool put(const void *memory, size_t size);
	
	inline unsigned size() const
		{return bufsize;};
	
	const unsigned char *get(void);

	const char *c_str(void);

	void set(const char *id);

	inline void operator=(const char *id)
		{set(id);};

	inline bool operator *=(const char *text)
		{return puts(text);};

	inline bool operator +=(const char *text)
		{return puts(text);};

	inline const char *operator*()
		{return c_str();};

	inline bool operator!() const
		{return !bufsize && context == NULL;};

	inline operator bool() const
		{return bufsize > 0 || context != NULL;};
};

/**
 * Crytographically relevant random numbers.  This is used both to gather
 * entropy pools and pseudo-random values.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT random
{
public:
	/**
	 * Push entropic seed.
	 * @param buffer of random data to push.
	 * @param size of buffer.
	 * @return true if successful.
	 */
	static bool seed(const unsigned char *buffer, size_t size);

	/**
	 * Re-seed pseudo-random generation and entropy pools.
	 */
	static void seed(void);

	/**
	 * Get high-entropy random data.  This is often used to
	 * initialize keys.  This operation may block if there is
	 * insufficient entropy immediately available.
	 * @param memory buffer to fill.
	 * @param size of buffer.
	 * @return number of bytes filled.
	 */
	static size_t key(unsigned char *memory, size_t size);

	/**
	 * Fill memory with pseudo-random values.  This is used
	 * as the basis for all get and real operations and does
	 * not depend on seed entropy.
	 * @param memory buffer to fill.
	 * @param size of buffer to fill.
	 * @return number of bytes set.
	 */
	static size_t fill(unsigned char *memory, size_t size);

	/**
	 * Get a pseudo-random integer, range 0 - 32767.
	 * @return random integer.
	 */
	static int get(void);

	/**
	 * Get a pseudo-random integer in a preset range.
	 * @param min value of random integer.
	 * @param max value of random integer.
	 * @return random value from min to max.
	 */
	static int get(int min, int max);

	/**
	 * Get a pseudo-random floating point value.
	 * @return psudo-random value 0 to 1.
	 */
	static double real(void);

	/**
	 * Get a pseudo-random floating point value in a preset range.
	 * @param min value of random floating point number.
	 * @param max value of random floating point number.
	 * @return random value from min to max.
	 */
	static double real(double min, double max);

	/**
	 * Determine if we have sufficient entropy to return random
	 * values.
	 * @return true if sufficient entropy.
	 */
	bool status(void);
};

/**
 * Convenience type for secure socket.
 */
typedef	SSocket	ssl_t;

/**
 * Convenience type for generic digests.
 */
typedef	Digest digest_t;

/**
 * Convenience type for generic ciphers.
 */
typedef	Cipher cipher_t;

/**
 * Convenience type for generic cipher key.
 */
typedef Cipher::Key skey_t;

END_NAMESPACE

#endif