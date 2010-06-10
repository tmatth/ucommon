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
	 * Set end of line marker.  Normally \r\n which supports both \n and
	 * \r\n termination of lines.
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
	bool flush(void);

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
};

END_NAMESPACE

#endif
