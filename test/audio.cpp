// Copyright (C) 2010 David Sugar, Tycho Softworks.
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

#include <config.h>
#include <ucommon/audio.h>

using namespace UCOMMON_NAMESPACE;

static const audiocodec *ac = NULL;
static unsigned relcount = 0;
static uint8_t buftest;

class debugbuffer : public audiobuffer
{
private:
	debugbuffer() {
		buffer = &buftest;
		state = ACTIVE;
		// we assume overlay data remains correct at construction...
		assert(channel == ac);
		assert(backpos == 0);
		assert(backstore != 0);
	}

	void release(void) {
		// we assume overlay data remains intact for life of object...
		assert(channel->basetype() == ac->basetype());
		assert(backpos == 0);
		assert(backstore != 0);
		++relcount;
		audiobuffer::release();
	}

	unsigned offset(void) {
		return sizeof(debugbuffer);
	}

public:
	static void attach(audiobuffer *buffer) {
		caddr_t cp = (caddr_t)buffer;
		new(cp) debugbuffer();
	}

	static encoded_t bufof(audiobuffer *buffer) {
		return ((debugbuffer *)buffer)->getBuffer();
	}
};

int main(int argc, char **argv)
{
	ac = audiocodec::get("pcmu");
	assert(ac != NULL);

	audiobuffer *buffer = audiobuffer::create(ac, 0);
	assert(buffer != NULL);

	debugbuffer::attach(buffer);

	assert(debugbuffer::bufof(buffer) == &buftest);

	buffer->release();
	assert(relcount == 1);
	assert(debugbuffer::bufof(buffer) == NULL);

	audio::release(buffer);
	assert(relcount == 2);
}
	

