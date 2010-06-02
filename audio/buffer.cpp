// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2009 David Sugar, Tycho Softworks.
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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "local.h"

using namespace UCOMMON_NAMESPACE;
using namespace LOCAL_NAMESPACE;

audiobuffer *audiobuffer::alloc(const audiocodec *encoding, timeout_t framing)
{
	assert(encoding != NULL);

	unsigned buf, max, extra;

	if(framing) {
		max = buf = framing * encoding->getClock() / 1000l;
		extra = 60 * encoding->getClock() / 500l;
	}
	else {
		buf = encoding->getFraming() * encoding->getClock() / 1000l;
		max = extra = encoding->getMaximum();
	}
	
	unsigned tonesize = 60 * encoding->getClock() / 500l + sizeof(tonegen);
	unsigned streamsize = sizeof(audiowriter);

	if(sizeof(audioreader) > streamsize)
		streamsize = sizeof(audioreader);

	if(buf > max)
		streamsize += buf;
	else
		streamsize += max;

	if(streamsize < tonesize)
		streamsize = tonesize;

	// alignments...
	streamsize += encoding->getSize();
	streamsize += (sizeof(void*) * 4);

	caddr_t cp = (caddr_t)cpr_memalloc(streamsize + extra);
	audiobuffer *frame = new(cp) audiobuffer();

	// assure backstore buffer is physically aligned in memory...
	frame->state = IDLE;
	frame->allocsize = streamsize + extra;
	frame->backstore = frame->allocsize;
	frame->channelbase = encoding;
	frame->limit = framing;
	frame->iocount = 0;
	return frame;
}

fd_t audiobuffer::handle(void)
{
	return INVALID_HANDLE_VALUE;
}

audiobuffer *audiobuffer::create(timeout_t maxframe)
{
	return alloc(audiocodec::find("pcmu"), maxframe);
}

audiobuffer *audiobuffer::create(const audiocodec *encoding, timeout_t maxframe)
{
	return alloc(encoding, maxframe);
}

unsigned audiobuffer::framesize(void)
{
	if(channel)
		return channel->getMaximum();
	return 0;
}

unsigned audiobuffer::framesize(encoded_t data)
{
	if(channel)
		return channel->getActual(data);

	return 0;
}

timeout_t audiobuffer::frametime(void)
{
	if(limit && encoding)
		return encoding->getFraming();

	if(limit)
		return limit;

	if(channel)
		return channel->getFraming();
	
	return 0;
}
	
bool audiobuffer::assign(size_t objsize, const audiocodec *format, timeout_t framing)
{
	caddr_t cp = (caddr_t)this;
	unsigned size;
	audiocodec *ch;

	assert(channel != NULL);
	assert(objsize > 0);

	iocount = 0;
	offset = objsize;
	backstore = allocsize;

	// we can create active copy of channel conversion if NULL format...
	// used by tonegen and phrasebook...

	if(!format) {
		size = offset;
		if(size % sizeof(void *))
			size += (sizeof(void *) - (size % sizeof(void *)));
		cp += size;
		if(!framing)
			framing = channel->getFraming();
		if(!framing)
			framing = limit;
		ch = channel->create(cp, framing);
		if(!ch) {
			state = INVALID;
			return false;
		}
		channel = ch;
		if(size % sizeof(void *))
			size += (sizeof(void *) - (size % sizeof(void *)));
		size = channel->getSize();
		cp += size;

		backstore = allocsize - channel->getMaximum();
		goto buffering;
	}

	// if native format, no need for extra buffering or embedding, use native
	// format of channel for streaming...

	if(channel->basetype() == format->basetype())
		goto alloc;
	
	// if direct conversion between non-supported frame types requested, reject...	
	if(channel && channel->type != ULAW && channel->type != ALAW && format->type != ULAW && format->type != ALAW) {
		state = INVALID;
		return false;
	}

	// we cannot sync or resample different clock rates...
	if(channel->getClock() != format->getClock()) {
		state = INVALID;
		return false;
	}
	
	// framing mismatch and not a streaming codec...
	if(format->getFraming() > limit) {
		if(channel && format->framing && format->getFraming() != framing) {
			state = INVALID;
			return false;
		}
	}

	// insufficient room for embedding codec...
	if(format->getSize() > backstore - offset) {
		state = INVALID;
		return false;
	}

alloc:
	cp += offset;
	
	if(!framing)
		framing = format->getFraming();
	if(!framing)
		framing = limit;

	encoding = format->create(cp, framing);
	cp += encoding->getSize();

	ch = channel->create(cp, framing);
	if(!ch || !encoding) {
		state = INVALID;
		return false;
	}
	channel = ch;
	cp += channel->getSize();

	backstore = allocsize - channel->getMaximum();
	
	// requires more space than we have...
	if(encoding->getMaximum() > backstore - offset - encoding->getSize()) {
		state = INVALID;
		return false;
	}

buffering:
	convert = backstore - (channel->getClock() * frametime() / 500l);

	unsigned count;
	unsigned bufmax = convert - offset - channel->getSize();

	buffering = 0;

	if(encoding) 
		bufmax -= encoding->getSize();

	if(limit) {
		count = limit / frametime();
		buffering = count * framesize();
	}

	if(!buffering || buffering > bufmax) {
		count = bufmax / framesize();
		buffering = count * framesize();
	}

	buffer = (audio::encoded_t)cp;
	return true;
}

audio::linear_t audiobuffer::expand(encoded_t data)
{
	linear_t target = getConvert();
	if(channel->type == LINEAR)
		return (linear_t)data;

	channel->decode(data, target);
	return target;
}	

void audiobuffer::release(void)
{
	buffer = NULL;
	backstore = allocsize;
	backpos = backio = 0;
	encoding = NULL;
	iocount = 0;

	if(channel)
		channelbase = channel->basetype();
}

unsigned audiobuffer::push(encoded_t data, unsigned size)
{
	encoded_t buf = ((encoded_t)(this)) + backstore;
	unsigned count = 0;
	unsigned min = channel->getMinimum();
	unsigned req;

	while(size--) {
		while(backpos < min) {
			++count;
			buf[backpos++] = *(data++);
		}

		req = channel->getActual(buf);
		while(backpos < req) {
			++count;
			buf[backpos++] = *(data++);
		}
		
		backpos = 0;
		if(!put(buf))
			break;
	}
	return count;
}

unsigned audiobuffer::pull(encoded_t data, unsigned size)
{
	encoded_t buf = ((encoded_t)(this)) + backstore;
	unsigned count = 0;

	while(size--) {
		if(backpos == backio) {
			backio = get(buf);
			backpos = 0;
			if(!backio)
				return count;
		}
		while(size-- && backpos < backio) {
			++count;
			*(data++) = buf[backpos++];
		}
	}
	return count;
}

unsigned audiobuffer::text(char *str, size_t textlimit)
{
	if(buffer)
		String::set(str, textlimit, (const char *)buffer);
	else
		*str = 0;
	return strlen(str);
}

audio::encoded_t audiobuffer::transcode(encoded_t source, encoded_t temp, audiocodec *from, audiocodec *to)
{
	if(from->basetype() == to->basetype())
		return source;
	
	if(from->type == ULAW && to->type == ALAW) {
		audio::u2a(source, channel->getBuffered());
		return source;
	}

	if(from->type == ALAW && to->type == ULAW) {
		audio::a2u(source, channel->getBuffered());
		return source;
	}

	// future work here...
	return NULL;
}

bool audiobuffer::transbuffer(audiocodec *from, audiocodec *to)
{
	if(!from || !to || from->basetype() == to->basetype())
		return false;

	if(from->type == ULAW && to->type == ALAW)
		return false;

	if(from->type == ALAW && to->type == ULAW)
		return false;

	return true;
}
