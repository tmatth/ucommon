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

/**
 * This library holds the GNU telephonic audio library, ccAudio.  This
 * library offers support for audio transcoding, for accessing of audio
 * files, tone generation and detection, and phrasebook voice libraries.
 * @file ccaudio.h
 */

#include <config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <ucommon/audio.h>

#define NAMESPACE_LOCAL namespace __ccaudio__ {
#define LOCAL_NAMESPACE __ccaudio__

NAMESPACE_LOCAL
using namespace UCOMMON_NAMESPACE;

/**
 * Internal class for streaming playable audio data.  This streams audio
 * from a raw or contained audio file.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __LOCAL audioreader : private audiofile
{
private:
	friend class audiofile;

	audioreader(const char *path, timeout_t framing);

	bool fill(void);
	encoded_t get(void);
	unsigned get(encoded_t data);
	unsigned pull(encoded_t data, unsigned size);

	/**
	 * Close play file.
	 */
	void release(void);
};

/**
 * Internal class for audio recording to a file.  This streams audio
 * to a raw or contained audio file.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __LOCAL audiowriter : private audiofile
{
private:
	friend class audiofile;

	encoded_t transfer;

	audiowriter(const char *path, fsys::access_t access, timeout_t framing, const char *note, const audiocodec *format, unsigned mode);
	audiowriter(const char *path, timeout_t framing);

	bool flush(void);
	encoded_t buf(void);
	unsigned put(encoded_t data);
	unsigned put(void);
	unsigned push(encoded_t data, unsigned size);

	/**
	 * Perform any writeback to header and close active file.
	 */
	void release(void);
};

class __LOCAL audioinfo : private audiofile
{
private:
	friend class audiofile;

	audioinfo(const char *path, timeout_t framing);

	void release(void);
};

END_NAMESPACE


