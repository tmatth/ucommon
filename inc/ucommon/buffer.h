// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

/**
 * A common buffered I/O class is used to stream character data without 
 * need for stdlib.  This header also includes a derived buffered socket
 * class and some related operations.
 * @file ucommon/buffer.h
 */

#ifndef	_UCOMMON_BUFFER_H_
#define	_UCOMMON_BUFFER_H_

#ifndef	_UCOMMON_SOCKET_H_
#include <ucommon/socket.h>
#endif

#ifndef	_UCOMMON_FSYS_H_
#include <ucommon/fsys.h>
#endif

NAMESPACE_UCOMMON

/**
 * Common buffered I/O class.  This is used to create objects which will
 * stream character data as needed.  This class can support bidrectionalal
 * streaming as may be needed for serial devices, sockets, and pipes.  The
 * buffering mechanisms are hidden from derived classes, and two virtuals
 * are used to communicate with the physical transport. 
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT IOBuffer
{
protected:
	typedef enum {BUF_RD, BUF_WR, BUF_RDWR} type_t;

private:
	const char *eol;
	char *buffer;
	char *input, *output;
	size_t bufsize, bufpos, insize, outsize;
	bool end;

protected:
	int ioerror;
	timeout_t timeout;
	const char *format;

	/**
	 * Construct an empty (unallocated) buffer.
	 */
	IOBuffer();

	/**
	 * Construct a buffer of pre-allocated size and access type.
	 * @param size of buffer to allocate.
	 * @param access mode of buffer.
	 */
	IOBuffer(size_t size, type_t access = BUF_RDWR);

	/**
	 * Destroy object by releasing buffer memory.
	 */
	~IOBuffer();

	/**
	 * Set end of line marker.  Normally this is set to cr & lf, which 
	 * actually supports both lf alone and cr/lf termination of lines.
	 * However, putline() will always add the full cr/lf if this mode is 
	 * used.  This option only effects getline() and putline().
	 * @param string for eol for getline and putline.
	 */
	inline void seteol(const char *string)
		{eol = string;};

	/**
	 * Allocate I/O buffer(s) of specified size.  If a buffer is currently
	 * allocated, it is released.
	 * @param size of buffer to allocate.
	 * @param access mode of buffer.
	 */
	void allocate(size_t size, type_t access = BUF_RDWR);

	/**
	 * Release (free) buffer memory.
	 */
	void release(void);

	/**
	 * Request workspace in output buffer.  This returns a pointer to
	 * memory from the output buffer and advances the output position.
	 * This is sometimes used for a form of zero copy write.
	 * @param size of request area.
	 * @return data pointer or NULL if not available.
	 */
	char *request(size_t size);

	/**
	 * Gather returns a pointer to contigues input of specified size.
	 * This may require moving the input data in memory.
	 * @param size of gather space.
	 * @return data pointer to gathered data or NULL if not available.
	 */
	char *gather(size_t size);

	/**
	 * Method to push buffer into physical i/o (write).  The address is
	 * passed to this virtual since it is hidden as private.
	 * @param address of data to push.
	 * @param size of data to push.
	 * @return number of bytes written, 0 on error.
	 */
	virtual size_t _push(const char *address, size_t size);

	/**
	 * Method to pull buffer from physical i/o (read).  The address is
	 * passed to this virtual since it is hidden as private.
	 * @param address of buffer to pull data into.
	 * @param size of buffer area being pulled..
	 * @return number of read written, 0 on error or end of data.
	 */
	virtual size_t _pull(char *address, size_t size);

	/**
	 * Get current input position.  Sometimes used to help compute and report 
     * a "tell" offset.
	 * @return offset of input buffer.
	 */
	inline size_t _pending(void)
		{return bufpos;};

	/**
	 * Get current output position.  Sometimes used to help compute a
	 * "trunc" operation.
	 */
	inline size_t _waiting(void)
		{return outsize;};

	/**
	 * Get size of the I/O buffer.
	 */
	inline size_t _buffering(void)
		{return bufsize;};

public:
	/**
	 * Get a character from the buffer.  If no data is available, return EOF.
	 * @return character from buffer or eof.
	 */
	int getchar(void);

	/**
	 * Put a character into the buffer.
	 * @return character put into buffer or eof.
	 */
	int putchar(int ch);

	/**
	 * Gry memory from the buffer.
	 * @param address of characters save from buffer.
	 * @param count of characters to get from buffer.
	 * @return number of characters actually copied.
	 */
	size_t getchars(char *address, size_t count);

	/**
	 * Put memory into the buffer.  If count is 0 then put as NULL
	 * terminated string.
	 * @param address of characters to put into buffer.
	 * @param count of characters to put into buffer.
	 * @return number of characters actually written.
	 */
	size_t putchars(const char *address, size_t count = 0);

	/**
	 * Print formatted string to the buffer.  The maximum output size is
	 * the buffer size, and the operation flushes the buffer.
	 * @param format string.
	 * @return number of bytes written.
	 */
	size_t printf(const char *format, ...) __PRINTF(2, 3);

	/**
	 * Flush buffered memory to physical I/O.
	 * @return true on success, false if not active or fails.
	 */
	virtual bool flush(void);

	/**
	 * Purge any pending input or output buffer data.
	 */
	void purge(void);

	/**
	 * Reset input buffer state.  Drops any pending input.
	 */
	void reset(void);

	/**
	 * Get a string as a line of input from the buffer.  The eol character(s)
	 * are used to mark the end of a line.
	 * @param string to save input into.
	 * @param size limit of string to save.
	 * @return count of characters read or 0 if at end of data.
	 */
	size_t getline(char *string, size_t size);

	/**
	 * Put a string as a line of output to the buffer.  The eol character is 
	 * appended to the end.
	 * @param string to write.
	 * @return total characters successfully written, including eol chars.
	 */
	size_t putline(const char *string);

	/**
	 * Check if at end of input.
	 * @return true if end of data, false if input still buffered.
	 */
	bool eof();

	/**
	 * Get last error associated with a failed operation.
	 * @return last error.
	 */
	inline int err(void)
		{return ioerror;};

	/**
	 * Clear last error state.
	 */
	inline void clear(void)
		{ioerror = 0;};

	/**
	 * See if buffer open.
	 * @return true if buffer active.
	 */
	inline operator bool()
		{return buffer != NULL;};

	/**
	 * See if buffer closed.
	 * @return true if buffer inactive.
	 */
	inline bool operator!()
		{return buffer == NULL;};

	/**
	 * See if buffer open.
	 * @return true if buffer active.
	 */
	inline bool isopen(void)
		{return buffer != NULL;};

	/**
	 * See if input active.
	 * @return true if input active.
	 */
	inline bool isinput(void)
		{return input != NULL;};

	/**
	 * See if output active.
	 * @return true if output active.
	 */
	inline bool isoutput(void)
		{return output != NULL;};

	/**
	 * Set eof flag.
	 */
	inline void seteof(void)
		{end = true;};

	/**
	 * See if data is pending.
	 * @return true if data is pending.
	 */
	virtual bool pending(void);
};

/**
 * A generic file streaming class built from the I/O buffer.  This can
 * be used in place of fopen based file operations and does not require
 * libstdc++.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class fbuf : public IOBuffer, private fsys
{
private:
	offset_t	inpos, outpos;

protected:
	size_t _push(const char *address, size_t size);
	size_t _pull(char *address, size_t size);

	inline fd_t getfile(void)
		{return fd;};

public:
	/**
	 * Construct an unopened file buffer.
	 */
	fbuf();

	/**
	 * Destroy object and release all resources.
	 */
	~fbuf();

	/**
	 * Construct a file buffer that creates and opens a specific file.
	 * @param path of file to create.
	 * @param access mode of file (rw, rdonly, etc).
	 * @param permissions of the newly created file.
	 * @param size of the stream buffer.
	 */
	fbuf(const char *path, fsys::access_t access, unsigned permissions, size_t size);

	/**
	 * Construct a file buffer that opens an existing file.
	 * @param path of existing file to open.
	 * @param access mode of file (rw, rdonly, etc).
	 * @param size of the stream buffer.
	 */
	fbuf(const char *path, fsys::access_t access, size_t size);

	/**
	 * Create and open the specified file.  If a file is currently open, it
	 * is closed first.
	 * @param path of file to create.
	 * @param access mode of file (rw, rdonly, etc).
	 * @param permissions of the newly created file.
	 * @param size of the stream buffer.
	 */
	void create(const char *path, fsys::access_t access = fsys::ACCESS_APPEND, unsigned permissions = 0640, size_t size = 512);

	/**
	 * Construct a file buffer that opens an existing file.
	 * @param path of existing file to open.
	 * @param access mode of file (rw, rdonly, etc).
	 * @param size of the stream buffer.
	 */
	void open(const char *path, fsys::access_t access = fsys::ACCESS_RDWR, size_t size = 512);

	/**
	 * Close the file, flush buffers.
	 */
	void close(void);
	
	/**
	 * Seek specific offset in open file and reset I/O buffers.  If the
	 * file is opened for both read and write, both the read and write
	 * position will be reset.
	 * @param offset to seek.
	 * @return true if successful.
	 */
	bool seek(offset_t offset);

	/**
	 * Truncate the currently open file to a specific position.  All
	 * I/O buffers are reset and the file pointer is set to the end.
	 * @param offset to truncate.
	 * @return true if successful.
	 */
	bool trunc(offset_t offset);

	/**
	 * Give the current position in the currently open file.  If we are
	 * appending, this is alwayus seek::end.  If we have a file opened
	 * for both read and write, this gives the read offset.
	 * @return file offset of current i/o operations.
	 */
	offset_t tell(void);
};

/**
 * A generic tcp server class for TCPSocket.  This saves the service id
 * tag so that it can be propogated.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT TCPServer : public ListenSocket
{
private:
	friend class TCPSocket;

	const char *servicetag;

public:
	/**
	 * Create and bind a tcp server.  This mostly is used to preserve the
	 * service tag for TCP Socket when derived from a server instance.
	 * @param service tag to use.
	 * @param address of interface to bind or "*" for all.
	 * @param backlog size for pending connections.
	 * @param protocol to use (normally tcpip).	
	 */
	TCPServer(const char *service, const char *address = "*", unsigned backlog = 5, int protocol = 0);
};

/**
 * A generic tcp socket class that offers i/o buffering.  All user i/o
 * operations are directly inherited from the IOBuffer base class public
 * members.  Some additional members are added for layering ssl services.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT TCPSocket : public IOBuffer, private Socket
{
private:
	void _buffer(size_t size);

	char serviceid[16];
	const char *servicetag;

protected:
	virtual size_t _push(const char *address, size_t size);
	virtual size_t _pull(char *address, size_t size);

	/**
	 * Get the low level socket object.
	 * @return socket we are using.
	 */
	inline socket_t getsocket(void) const
		{return so;};

	/**
	 * Get the effective "service" port identifier of the socket.  If the
	 * socket was created from a listener, this will be the port number
	 * of the listening socket, and hence can be useful to determine
	 * which service connected and whether ssl is needed.  If this is
	 * a client socket, it will be the service port of the peer socket.
	 * @return service identifier associated with the socket.
	 */
	inline short getservice(void) const
		{return (short)(atol(serviceid));};

	/**
	 * Get the tag used for this socket.
	 * @return service tag associated with this socket.
	 */
	inline const char *tag(void) const
		{return servicetag;};

	/**
	 * Peek at socket input.  This might be used to get a header at
	 * connection and to see if ssl/tls service is needed.
	 * @param data buffer to store peek results.
	 * @param size of data to peek.
	 * @param timeout to wait for peek.
	 * @return number of bytes seen.
	 */
	size_t peek(char *data, size_t size, timeout_t timeout = Timer::inf);

public:
	/**
	 * Construct an unconnected tcp client and specify our service profile.
	 * @param service identifer, usually by name.
	 */
	TCPSocket(const char *service);

	/**
	 * Construct a tcp server session from a listening socket.
	 * @param server socket we are created from.
	 * @param size of buffer and tcp fragments.
	 * @param timer mode for i/o operations.
	 */
	TCPSocket(TCPServer *server, size_t size = 536);

	/**
	 * Construct a tcp client session connected to a specific host uri.
	 * @param service identifier of our client.
	 * @param host and optional :port we are connecting to.
	 * @param size of buffer and tcp fragments.
	 * @param timer mode for i/o operations.
	 */
	TCPSocket(const char *service, const char *host, size_t size = 536);

	/**
	 * Destroy the tcp socket and release all resources.
	 */
	virtual ~TCPSocket();

	/**
	 * Connect a tcp socket to a client from a listener.  If the socket was
	 * already connected, it is automatically closed first.
	 * @param server we are connected from.
	 * @param size of buffer and tcp fragments.
	 * @param timer mode for i/o operations.
	 */
	void open(TCPServer *server, size_t size = 536);

	/**
	 * Connect a tcp client session to a specific host uri.  If the socket
	 * was already connected, it is automatically closed first.
	 * @param host and optional :port we are connecting to.
	 * @param size of buffer and tcp fragments.
	 */
	void open(const char *host, size_t size = 536);

	/**
	 * Close active connection.
	 */
	void close(void);

	/**
	 * Set timeout interval and blocking.
	 * @param timeout to use.
	 */
	void blocking(timeout_t timeout = Timer::inf);	

	/**
	 * Check for pending tcp or ssl data.
	 * @return true if data pending.
	 */
	bool pending(void);
};

/**
 * Convenience type for buffered file operations.
 */
typedef	fbuf file_t;

/**
 * Convenience type for pure tcp sockets.
 */
typedef	TCPSocket tcp_t;
	 
END_NAMESPACE

#endif
