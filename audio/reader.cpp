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

audioreader::audioreader(const char *path, timeout_t framing)
{
	assert(path != NULL);

    open(path, fsys::ACCESS_STREAM, sizeof(audioreader), framing);
}

void audioreader::release(void)
{
	if(state == INVALID || state == INVFILE) {
		state = IDLE;
	}
	else if(state != IDLE) {
		fs.close();
		state = IDLE;
	}
}

bool audioreader::fill(void)
{
	// if inactive, false...
	if(state != ACTIVE)
		return false;

	// if we have at least a frame left, we are fine...
	if(bufpos + maxio <= bufend)
		return true;

	// if we are at marked end of file, then set end
	if(current >= hiwater) {
		current = hiwater;
		state = END;
		bufpos = bufend = 0;
		return false;
	}

	// if variable framing, we have to resync file position...
	if(bufpos != bufend) {
		++iocount;
		fs.seek(headersize + current);
	}

	// now fill the stream buffer for remaining data
	size_t xfer = (hiwater - current);
	if(xfer > buffering)
		xfer = buffering;

	++iocount;
	bufpos = 0;
	bufend = fs.read(buffer, xfer);
	if(bufend < minio) {
		state = FAILED;
		return false;
	}
	return true;
}

audio::encoded_t audioreader::get(void)
{
	unsigned io = bufio;

	if(!fill())
		return NULL;

	encoded_t dp = buffer + bufpos;

	if(!io)
		io = encoding->getActual(dp);

	bufpos += io;
	current += io;

	return transcode(dp, ((encoded_t)(this)) + backstore, encoding, channel);
}

unsigned audioreader::get(encoded_t target)
{
	unsigned io = bufio;
	encoded_t result;

	if(!fill())
		return 0;

	encoded_t dp = buffer + bufpos;

	if(!io)
		io = encoding->getActual(dp);

	bufpos += io;
	current += io;

	result = transcode(dp, target, encoding, channel);
	if(!result)
		return 0;

	if(result != target)
		memcpy(target, dp, io);

	return io;
}

unsigned audioreader::pull(encoded_t data, unsigned size)
{
	unsigned align = 0;
	fsys::offset_t xfer;
	ssize_t result;

	if(state != ACTIVE)
		return 0;

	if(encoding->basetype() == channel->basetype())
		align = encoding->getAlignment();

	if(current == hiwater) {
		state = END;
		return 0;
	}

	if(align && !(size % align) && bufpos == 0) {
		xfer = hiwater - current;
		if(xfer > (fsys::offset_t)size)
			xfer = size;
		result = fs.read(data, xfer);
		if(result < (ssize_t)xfer) {
			state = FAILED;
			if(result < 0)
				result = 0;
		}
		current += result;
		return result;
	}
	return audiobuffer::pull(data, size);
}


