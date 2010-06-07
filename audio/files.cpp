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

#ifdef	_MSWINDOWS_
#undef	HAVE_FTRUNCATE
#endif

using namespace LOCAL_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

audioinfo::audioinfo(const char *path, timeout_t framing)
{
	assert(path != NULL);

    open(path, fsys::ACCESS_RDONLY, sizeof(audioinfo), framing);
	if(state != INVALID && state != INVFILE)
		fs.close();
}

audiofile::operator bool()
{
	if(state == ACTIVE)
		return true;

	return false;
}

void audioinfo::release(void)
{
	bufpos = bufend = 0;
	state = IDLE;
}

fd_t audiofile::handle(void)
{
	if(state == ACTIVE)
		return *fs;
	return INVALID_HANDLE_VALUE;
}

void audiofile::open(const char *path, fsys::access_t access, size_t objsize, timeout_t framing)
{
	const audiocodec *format = NULL;
	const char *ext = strrchr(path, '.');
	char buf[32];
	struct stat ino;
	size_t extra = 0;

    state = INVFILE;
    filetype = RAW;
	hiwater = current = headersize = 0l;	

	if(fs.stat(path, &ino))
		return;

	fs.open(path, access);
	if(!is(fs))
		return;

	state = INVALID;

	// raw files use same encoding as channel
	if(ext && eq(ext, ".raw"))
		format = channel->basetype();
	else if(ext && eq(ext, ".au")) {
		filetype = AU;
		if(fs.read(header, 24) < 24) {
			state = FAILED;
			return;
		}
		snprintf(buf, sizeof(buf), "au/%d", msb_getlong(header + 12));
		format = audiocodec::find(buf);
		headersize = msb_getlong(header + 4);
		extra = headersize - 24;
	}
	else if(ext)
		format = audiocodec::find(ext);
	if(!format || !assign(objsize, format, framing)) {
		fs.close();
		return;
	}

	state = FAILED;
	*buffer = 0;

	if(extra) {
		if(extra >= buffering)
			extra = buffering - 1;
		if(fs.read(buffer, extra) < (ssize_t)extra)
			return;
		buffer[extra] = 0;
		fs.seek(headersize);
	}
	else
		buffer[0] = 0;

	minio = encoding->getMinimum();
	maxio = encoding->getMaximum();
	bufio = encoding->getBuffered();
	hiwater = ino.st_size - headersize;

	// fix for fixed frame alignment...
	if(bufio)
		hiwater -= (hiwater % bufio);

	bufpos = bufend = 0;
	state = ACTIVE;	
}

bool audiofile::flush(void)
{
	bufpos = bufend = 0;
	return true;
}

bool audiofile::rewind(void)
{
	if(state != ACTIVE && state != END)
		return false;

	++iocount;
	fs.seek(headersize);
	current = 0;
	bufpos = bufend = 0;	// clear buffering...
	return true;
}

bool audiofile::append(void)
{
	if(state != ACTIVE && state != END)
		return false;

	flush();
	++iocount;
	fs.seek(hiwater + headersize);
	current = hiwater;
	return true;
}

timeout_t audiofile::length(void)
{
	if(state != ACTIVE && state != END)
		return 0;

	assert(encoding != NULL);

	unsigned long frames = hiwater / maxio;
	return frames * encoding->getFraming();
}

timeout_t audiofile::locate(void)
{
	if(state != ACTIVE && state != END)
		return 0;

	assert(encoding != NULL);

	unsigned long frames = current / maxio;
	return frames * encoding->getFraming();
}

#ifdef	HAVE_FTRUNCATE
bool audiofile::trim(timeout_t pos) 
{
	if(state != ACTIVE && state != END)
		return false;

	assert(encoding != NULL);

	fd_t fd = *fs;

	if(minio == maxio) {
		flush();
		pos /= encoding->getFraming();
		pos *= maxio;
		if((fsys::offset_t)pos > current)
			pos = current;
		pos = current - pos;
		++iocount;
		fs.seek(headersize + pos);
		current = pos;
		if(ftruncate(fd, headersize + current) == -1) {
			state = FAILED;
			return false;
		}
		else {
			hiwater = current;	
			return true;
		}
	}
	return false;		
}
#else
bool audiofile::trim(timeout_t pos)
{
	return false;
}
#endif

bool audiofile::seek(timeout_t pos) 
{
	if(state != ACTIVE && state != END)
		return false;

	assert(encoding != NULL);

	if(minio == maxio) {
		pos /= encoding->getFraming();
		pos *= maxio;
		if((fsys::offset_t)pos > hiwater)
			return false;
		flush();
		++iocount;
		fs.seek(headersize + pos);
		current = pos;
		return true;
	}
	return false;		
}

timeout_t audiofile::skip(timeout_t pos)
{
	if(state != ACTIVE && state != END)
		return 0;

	assert(encoding != NULL);

	unsigned long prev = current;
	size_t len;
	unsigned count = 0;

	pos /= encoding->getFraming();

	flush();
	if(bufio) {
		pos *= bufio;
		pos += current;
		if((fsys::offset_t)pos > hiwater)
			pos = hiwater;
		++iocount;
		fs.seek(headersize + pos);
		current = pos;
		count = (current - prev) / bufio;
	}
	else {
		++iocount;
		fs.seek(headersize + current);	// make sure we are at base...
		while(pos--) {
			if(current == hiwater)
				break;
			++iocount;
			if(fs.read(buffer, minio) < (ssize_t)minio) {
				state = FAILED;
				break;
			}
			len = encoding->getActual(buffer);
			if(len > minio) {
				++iocount;
				if(fs.read(buffer + minio, len - minio) < (ssize_t)(len - minio)) {
					state = FAILED;
					break;
				}
			}
			current += len;
			++count;
		}
	}
	return count * encoding->getFraming();		
}

bool audiofile::create(audiobuffer *buffer, const char *path, timeout_t framing, const char *note, const audiocodec *format, unsigned mode)
{
	assert(buffer != NULL);
	assert(path != NULL);

	caddr_t cp = (caddr_t)buffer;
	if(buffer->status() != IDLE)
		buffer->release();

	::remove(path);
	new(cp) audiowriter(path, fsys::ACCESS_STREAM, framing, note, format, mode);
	if(buffer->status() == ACTIVE)
		return true;
	return false;
}

bool audiofile::access(audiobuffer *buffer, const char *path, timeout_t framing)
{
	assert(buffer != NULL);
	assert(path != NULL);

	caddr_t cp = (caddr_t)buffer;
	if(buffer->status() != IDLE)
		buffer->release();

	new(cp) audioreader(path, framing);
	if(buffer->status() == ACTIVE)
		return true;
	return false;
}			

bool audiofile::append(audiobuffer *buffer, const char *path, timeout_t framing)
{
	assert(buffer != NULL);
	assert(path != NULL);

	caddr_t cp = (caddr_t)buffer;
	if(buffer->status() != IDLE)
		buffer->release();

	new(cp) audiowriter(path, framing);
	buffer->append();
	if(buffer->status() == ACTIVE) 
		return true;

	return false;
}			

bool audiofile::info(audiobuffer *buffer, const char *path, timeout_t framing)
{
	assert(buffer != NULL);
	assert(path != NULL);

	caddr_t cp = (caddr_t)buffer;
	if(buffer->status() != IDLE)
		buffer->release();

	new(cp) audioinfo(path, framing);
	if(buffer->status() == ACTIVE)
		return true;
	return false;	
}			

