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

static LinkedObject *list = NULL;

audiocodec::audiocodec() :
LinkedObject(&list)
{
	framing = 0;
	rate = 8000;
}

audiocodec::audiocodec(const audiocodec *original)
{
	framing = original->getFraming();
	type = original->type;
	rate = original->rate;
}

unsigned long audiocodec::getClock(void) const
{
	return 8000l;
}

unsigned audiocodec::getChannels(void) const
{
	return 1;
}

unsigned audiocodec::autype(void) const
{
	return 0;
}

unsigned audiocodec::getMinimum(void) const
{
	return getMaximum();
}

unsigned audiocodec::getAlignment(void) const
{
	return 0;
}

timeout_t audiocodec::getFraming(void) const
{
	if(framing)
		return framing;
	return global;
}

unsigned audiocodec::getBuffering(void) const
{
	return getFraming() * 16;
}

unsigned audiocodec::encode(linear_t source, encoded_t dest)
{
	return 0;
}

unsigned audiocodec::decode(encoded_t source, linear_t dest)
{
	return 0;
}

void audiocodec::reset(void)
{
}

audiocodec *audiocodec::first(void)
{
	audio::init();
	return static_cast<audiocodec *>(list);
}

const audiocodec *audiocodec::find(const char *id, const audiocodec *channel)
{
	audio::init();

	if(channel && channel->test(id))
		return channel;

	linked_pointer<const audiocodec> cp = list;
	while(is(cp)) {
		if(cp->test(id)) {
			if(channel && cp->type != ULAW && cp->type != ALAW && channel->type != ULAW && channel->type != ALAW) {
				cp.next();
				continue;
			}
			if(!channel || cp->framing == 0 || cp->getFraming() == channel->getFraming())
				return *cp;
		}
		cp.next();
	}
	return NULL;
}

audiocodec *audiocodec::get(const char *id, const audiocodec *channel)
{
	audio::init();

	linked_pointer<const audiocodec> cp = list;

	if(channel && channel->test(id))
		return channel->create();

	while(is(cp)) {
		if(cp->test(id)) {
			if(channel && cp->type != ULAW && cp->type != ALAW && channel->type != ULAW && channel->type != ALAW) {
				cp.next();
				continue;
			}
			if(!channel)
				return cp->create();
			if(cp->framing == 0 || cp->getFraming() == channel->getFraming())
				return cp->create(NULL, channel->getFraming());
		}
		cp.next();
	}
	return NULL;
}


