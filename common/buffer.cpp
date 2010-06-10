// Copyright (C) 2010 David Sugar, Tycho Softworks.
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

#include <config.h>
#include <ucommon/buffer.h>
#include <ucommon/string.h>

using namespace UCOMMON_NAMESPACE;

IOBuffer::IOBuffer()
{
	end = true;
	eol = "\r\n";
	input = output = buffer = NULL;
}

IOBuffer::IOBuffer(size_t size, type_t mode)
{
	end = true;
	eol = "\r\n";
	input = output = buffer = NULL;
	allocate(size, mode);
}

IOBuffer::~IOBuffer()
{
	release();
}

void IOBuffer::release(void)
{
	if(buffer) {
		flush();
		free(buffer);
		input = output = buffer = NULL;
		end = true;
	}
}

void IOBuffer::allocate(size_t size, type_t mode)
{
	release();
	if(!size)
		return;

	switch(mode) {
	case BUF_RDWR:
		input = buffer = (char *)malloc(size * 2);
		if(buffer)
			output = buffer + size;
		break;
	case BUF_RD:
		input = buffer = (char *)malloc(size);
		break;
	case BUF_WR:
		output = buffer = (char *)malloc(size);
		break;
	}

	bufpos = insize = outsize = 0;
	bufsize = size;
	ioerror = 0;

	if(buffer)
		end = false;
}

size_t IOBuffer::getchars(char *address, size_t size)
{
	size_t count = 0;

	if(!input || !address)
		return 0;

	while(count < size) {
		if(bufpos == insize) {
			if(end)
				return count;

			insize = _pull(input, bufsize);
			bufpos = 0;
			if(insize < bufsize) {
				ioerror = errno;
				end = true;
			}
			if(!insize)
				return count;
		}
		address[count++] = input[bufpos++];
	}
	return count;
}

int IOBuffer::getchar(void)
{
	if(!input)
		return EOF;

	if(bufpos == insize) {
		if(end)
			return EOF;

		insize = _pull(input, bufsize);
		bufpos = 0;
		if(insize < bufsize) {
			ioerror = errno;
			end = true;
		}
		if(!insize)
			return EOF;
	}

	return input[bufpos++];
}

size_t IOBuffer::putchars(const char *address, size_t size)
{
	size_t count = 0;

	if(!output || !address)
		return 0;

	if(!count)
		count = strlen(address);

	while(count < size) {
		if(outsize == bufsize) {
			outsize = 0;
			if(_push(output, bufsize) < bufsize) {
				ioerror = errno;
				output = NULL;
				end = true;		// marks a disconnection...
				return count;
			}
		}
		
		output[outsize++] = address[count++];
	}

	return count;
}

int IOBuffer::putchar(int ch)
{
	if(!output)
		return EOF;

	if(outsize == bufsize) {
		outsize = 0;
		if(_push(output, bufsize) < bufsize) {
			ioerror = errno;
			output = NULL;
			end = true;		// marks a disconnection...
			return EOF;
		}
	}
	
	output[outsize++] = ch;
	return ch;
}

size_t IOBuffer::printf(const char *format, ...)
{
	va_list args;
	int result;
	size_t count;

	if(!flush() || !output || !format)
		return 0;

	va_start(args, format);
	result = vsnprintf(output, bufsize, format, args);
	va_end(args);
	if(result < 1)
		return 0;

	if(result > bufsize)
		result = bufsize;

	count = _push(output, result);
	if(count < (size_t)result) {
		output = NULL;
		end = true;
	}

	return count;
}

bool IOBuffer::flush(void)
{
	if(!output)
		return false;

	if(!outsize)
		return true;
	
	if(_push(output, outsize) == outsize) {
		outsize = 0;
		return true;
	}
	output = NULL;
	end = true;
	return false;
}

char *IOBuffer::gather(size_t size)
{
	if(!input || size > bufsize)
		return NULL;

	if(size + bufpos > insize) {
		if(end)
			return NULL;
	
		size_t adjust = outsize - bufpos;
		memmove(input, input + bufpos, adjust);
		insize = adjust +  _pull(input, bufsize - adjust);
		bufpos = 0;

		if(insize < bufsize) {
			ioerror = errno;
			end = true;
		}
	}

	if(size + bufpos <= insize) {
		char *bp = input + bufpos;
		bufpos += size;
		return bp;
	}

	return NULL;	
}

char *IOBuffer::request(size_t size)
{
	if(!output || size > bufsize)
		return NULL;

	if(size + outsize > bufsize)
		flush();

	size_t offset = outsize;
	outsize += size;
	return output + offset;
}	

size_t IOBuffer::putline(const char *string)
{
	size_t count = 0;

	if(string)
		count += putchars(string);

	if(eol)
		count += putchars(eol);
}

size_t IOBuffer::getline(char *string, size_t size)
{
	size_t count = 0;
	unsigned eolp = 0;
	const char *eols = eol;
	
	if(!eols)
		eols="\0";

	if(string)
		string[0] = 0;

	if(!input || !string)
		return 0;

	while(count < size - 1) {
		int ch = getchar();
		if(EOF) {
			eolp = 0;
			break;
		}

		string[count++] = ch;

		if(ch == eols[eolp]) {
			++eolp;
			if(eols[eolp] == 0)
				break;
		}
		else
			eolp = 0;	

		// special case for \r\n can also be just eol as \n 
		if(eq(eol, "\r\n") && ch == '\n') {
			++eolp;
			break;
		}
	}
	count -= eolp;
	string[count] = 0;
	return count;
}

void IOBuffer::reset(void)
{
	insize = bufpos = 0;
}

bool IOBuffer::eof(void)
{
	if(!input)
		return true;

	if(end && bufpos == insize)
		return true;

	return false;
}
	
size_t IOBuffer::_push(const char *address, size_t size)
{
	return 0;
}

size_t IOBuffer::_pull(char *address, size_t size)
{
	return 0;
}
		
