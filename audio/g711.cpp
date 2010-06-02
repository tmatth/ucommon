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

NAMESPACE_LOCAL
using namespace UCOMMON_NAMESPACE;

class __LOCAL _alaw : public audiocodec
{
private:
	unsigned samples;

	_alaw(const _alaw *original, timeout_t framing = 0);

	unsigned getSize(void) const;
	audiocodec *create(caddr_t mem, timeout_t framing) const;
	unsigned getMaximum(void) const;
	unsigned getActual(encoded_t frame) const;
	unsigned getBuffered(void) const;
	unsigned getAlignment(void) const;
	const char *getName(void) const;
	bool test(const char *id) const;
	unsigned autype(void) const;
	unsigned encode(linear_t source, encoded_t target);
	unsigned decode(encoded_t source, linear_t target);
	const audiocodec *basetype(void) const;

public:
	_alaw();
};

class __LOCAL _ulaw : public audiocodec
{
private:
	unsigned samples;

	_ulaw(const _ulaw *original, timeout_t framing = 0);

	unsigned getSize(void) const;
	audiocodec *create(caddr_t mem, timeout_t framing) const;
	unsigned getMaximum(void) const;
	unsigned getActual(encoded_t frame) const;
	unsigned getBuffered(void) const;
	unsigned getAlignment(void) const;
	const char *getName(void) const;
	bool test(const char *id) const;
	unsigned autype(void) const;
	unsigned encode(linear_t source, encoded_t target);
	unsigned decode(encoded_t source, linear_t target);
	const audiocodec *basetype(void) const;

public:
	_ulaw();
};


static _alaw _alaw_encoding;
static _ulaw _ulaw_encoding;

_alaw::_alaw() :
audiocodec()
{
	samples = 0;
	type = ALAW;
}

_alaw::_alaw(const _alaw *original, timeout_t framesize) :
audiocodec(original)
{
	if(framesize)
		framing = framesize;

	samples = framing * 8;
	type = ALAW;
}

const audiocodec *_alaw::basetype(void) const
{
	return &_alaw_encoding;
}

unsigned _alaw::getSize(void) const
{
	return sizeof(_alaw);
}

audiocodec *_alaw::create(caddr_t mem, timeout_t framesize) const
{
	if(mem)
		return static_cast<audiocodec*>(new(mem) _alaw(this, framesize));

	return static_cast<audiocodec*>(new _alaw(this, framesize));
}

unsigned _alaw::getActual(encoded_t frame) const
{
	return samples;
}

unsigned _alaw::getMaximum(void) const
{
	return samples;
}

unsigned _alaw::getBuffered(void) const
{
	return samples;
}

unsigned _alaw::decode(encoded_t source, linear_t target)
{
	audio::a2u(source, samples);
	audio::expand(source, samples, target);
	return samples;
}

unsigned _alaw::encode(linear_t source, encoded_t target)
{
	audio::repack(target, samples, source);
	audio::u2a(target, samples);
	return samples;
}

const char *_alaw::getName(void) const
{
	return "pcma/8000";
}

unsigned _alaw::getAlignment(void) const
{
	return 1;
}

unsigned _alaw::autype(void) const
{
	return 27;
}

bool _alaw::test(const char *id) const
{
	if(ieq(id, ".al") || ieq(id, ".ala") || ieq(id, ".alaw"))
		return true;

	if(ieq(id, "rtp/8"))
		return true;

	if(ieq(id, "pcma/8000"))
		return true;

	if(ieq(id, "pcma"))
		return true;

	if(eq(id, "au/27"))
		return true;

	if(eq(id, "wav/6"))
		return true;	

	return false;
}

#ifdef  _MSWINDOWS_
#define DLL_SUFFIX  ".dll"
#define LIB_PREFIX  "_libs"
#else
#define LIB_PREFIX  ".libs"
#define DLL_SUFFIX  ".so"
#endif

_ulaw::_ulaw() :
audiocodec()
{
	fsys dir;
    char buffer[256];
    char path[256];
	char *ep;
	unsigned el;

	samples = 0;
	type = ULAW;

	// this offers a convenient hooking point for loading plugins...

	String::set(path, sizeof(path), UCOMMON_PLUGINS "/codecs");
	fsys::open(dir, path, fsys::ACCESS_DIRECTORY);
	el = strlen(path);
    while(is(dir) && fsys::read(dir, buffer, sizeof(buffer)) > 0) {
		ep = strrchr(buffer, '.');
		if(!ep || !ieq(ep, DLL_SUFFIX))
			continue;
		snprintf(path + el, sizeof(path) - el, "/%s", buffer);
//		fprintf(stdout, "loading %s" DLL_SUFFIX, buffer);
		if(fsys::load(path))
			fprintf(stderr, "failed loading %s", path);
	}
	fsys::close(dir);
}

_ulaw::_ulaw(const _ulaw *original, timeout_t framesize) :
audiocodec(original)
{
	if(framesize)
		framing = framesize;

	samples = framing * 8;
}

const audiocodec *_ulaw::basetype(void) const
{
	return &_ulaw_encoding;
}

unsigned _ulaw::getSize(void) const
{
	return sizeof(_ulaw);
}

audiocodec *_ulaw::create(caddr_t mem, timeout_t framesize) const
{
	if(mem)
		return static_cast<audiocodec*>(new(mem) _ulaw(this, framesize));

	return static_cast<audiocodec*>(new _ulaw(this, framesize));
}

unsigned _ulaw::getActual(encoded_t frame) const
{
	return samples;
}

unsigned _ulaw::getMaximum(void) const
{
	return samples;
}

unsigned _ulaw::getBuffered(void) const
{
	return samples;
}

unsigned _ulaw::decode(encoded_t source, linear_t target)
{
	audio::expand(source, samples, target);
	return samples;
}

unsigned _ulaw::encode(linear_t source, encoded_t target)
{
	audio::repack(target, samples, source);
	return samples;
}

const char *_ulaw::getName(void) const
{
	return "pcmu/8000";
}

unsigned _ulaw::autype(void) const
{
	return 1;
}

unsigned _ulaw::getAlignment(void) const
{
	return 1;
}

bool _ulaw::test(const char *id) const
{
	if(ieq(id, ".ul") || ieq(id, ".ula") || ieq(id, ".ulaw"))
		return true;

	if(eq(id, "rtp/0"))
		return true;

	if(ieq(id, "pcmu/8000"))
		return true;

	if(ieq(id, "pcmu"))
		return true;

	if(eq(id, "au/1"))
		return true;

	if(eq(id, "wav/7"))
		return true;	

	return false;
}

END_NAMESPACE
