// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include "local.h"

using namespace UCOMMON_NAMESPACE;
using namespace LOCAL_NAMESPACE;

audiowriter::audiowriter(const char *path, timeout_t framing)
{
	open(path, fsys::ACCESS_RDWR, sizeof(audiowriter), framing);
}
	
audiowriter::audiowriter(const char *path, fsys::access_t access, timeout_t framing, const char *note, const audiocodec *format, unsigned mode)
{
	const char *ext = strrchr(path, '.');
	size_t extra = 0;

	hiwater = current = headersize = 0l;
	state = INVALID;
	filetype = RAW;

	if(note)
		extra = strlen(note) + 1;

	if(ieq(ext, ".raw")) {
		if(!format)
			format = channel->basetype();
	}
	else if(ieq(ext, ".au")) {
		filetype = AU;
		if(!format)
			format = audiocodec::find("au/1");
		memset(header, 0, 24);
		msb_setlong(header, 0x2e736e64);
		msb_setlong(header + 4, extra + 24);
		msb_setlong(header + 8, ~0l);
		msb_setlong(header + 12, format->autype());
		msb_setlong(header + 16, format->getClock());
		msb_setlong(header + 20, format->getChannels());
		headersize = 24;
		// if not supported format, then error out on create
		if(!format->autype())
			return;	
	}	
	else 
		format = audiocodec::find(ext);

	if(!format || !assign(sizeof(audiowriter), format, framing))
		return;
	
	fs.create(path, access, mode);
	if(!is(fs)) {
		state = INVFILE;
		return;
	}

	state = FAILED;
	if(headersize && fs.write(header, headersize) < headersize) 
		return;

	if(extra && fs.write(note, extra) < (ssize_t)extra)
		return;

	headersize += extra;
	minio = encoding->getMinimum();
	maxio = encoding->getMaximum();
	bufio = encoding->getBuffered();
	bufpos = bufend = 0;
	String::set((char *)buffer, (size_t)buffering, note);
	state = ACTIVE;
}

void audiowriter::release(void)
{
	if(state == IDLE)
		return;

	if(state == INVALID || state == INVFILE) {
		state = IDLE;
		return;
	}

	flush();

	switch(filetype) {
	case AU:
		msb_setlong(header + 8, hiwater);
		fs.seek(0);
		fs.write(header, 24);
		++iocount;
	default:
		break;
	};
	fs.close();
	state = IDLE;
}

bool audiowriter::flush(void)
{
	size_t result;

	if(state != ACTIVE)
		return false;

	if(bufpos == 0)
		return true;

	++iocount;
	result = fs.write(buffer, bufpos);
	bufpos = 0;
	if(result < bufpos) {
		state = FAILED;
		return false;
	}
	return true;
}

unsigned audiowriter::put(encoded_t data)
{
	unsigned io = bufio;
	encoded_t result;

	if(bufpos + maxio >= buffering)
		flush();

	if(state != ACTIVE)
		return 0;

	encoded_t target = buffer + bufpos;
	result = transcode(data, target, channel, encoding);
	if(!result)
		return 0;

	if(!io)
		io = encoding->getActual(result);	

	bufpos += io;
	current += io;

	if(current > hiwater)
		hiwater = current;

	if(result != target)
		memcpy(target, data, io);

	return io;
}

audio::encoded_t audiowriter::buf(void)
{
	if(bufpos + maxio >= buffering)
		flush();

	if(state != ACTIVE)
		return false;

	if(transbuffer(channel, encoding))
		transfer = ((encoded_t)(this)) + backstore;
	else
		transfer = buffer + bufpos;

	return transfer;
}

unsigned audiowriter::put(void)
{
	unsigned io = bufio;
	encoded_t result;
	encoded_t target = buffer + bufpos;

	result = transcode(transfer, target, channel, encoding);

	if(!result)
		return 0;

	if(!io)
		io = encoding->getActual(result);

	bufpos += io;
	current += io;

	if(current > hiwater)
		hiwater = current;

	if(result != target)
		memcpy(target, transfer, io);

	return io;
}

unsigned audiowriter::push(encoded_t data, unsigned size)
{
	unsigned align = 0;
	ssize_t result;

	if(state != ACTIVE)
		return 0;

	if(encoding->basetype() == channel->basetype())
		align = encoding->getAlignment();

	if(align && !(size % align) && bufpos == 0) {
		flush();
		result = fs.write(data, size);
		if((result < 0) || ((unsigned)result > size)) {
			state = FAILED;
			if(result < 0)
				result = 0;
		}
		current += result;
		if(current > hiwater)
			hiwater = current;
		return result;
	}
	return audiobuffer::push(data, size);
}


