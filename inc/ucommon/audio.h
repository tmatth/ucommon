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
 * This is the GNU telephonic audio library for the GNU uCommon C++
 * framework.  This library will offer support for audio transcoding, for
 * accessing of audio files, tone generation and detection, and phrasebook
 * voice libraries.
 * @file ucommon/audio.h
 */

#ifndef _UCOMMON_AUDIO_H_
#define _UCOMMON_AUDIO_H_

#ifndef _UCOMMON_UCOMMON_H_
#include <ucommon/ucommon.h>
#endif

NAMESPACE_UCOMMON

/**
 * Common audio class for GNU telephonic audio.  This holds many common
 * and useful functions that can be inherited into other classes.  This
 * class also defines common data types and structures used in the
 * remainder of GNU ccAudio.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class  __EXPORT audio
{
public:
	typedef uint8_t *encoded_t;
	typedef int16_t *linear_t;
	typedef enum {ULAW, ALAW, LINEAR, STREAM, PACKET, FRAMED, NATIVE} type_t;
	typedef enum {IDLE = 0, ACTIVE, END, FAILED, INVALID, INVFILE, INVIO = FAILED} state_t;

	static volatile timeout_t global;

	/**
	 * Common access to audio frames.  This offers access to and manipulation
	 * of audio media sources such as files, devices, and streams.  Most
	 * audio sources are optimally accessed in dsp frames, and hence this
	 * class is optimized for frame oriented access to media.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __EXPORT framer
	{
	protected:
		state_t state;

	public:
		
		virtual timeout_t frametime(void) = 0;
		virtual unsigned framesize(void) = 0;
		virtual unsigned framesize(encoded_t data) = 0;
		virtual encoded_t get(void);	// gives audiobuf mem...
		virtual unsigned get(encoded_t data);
		virtual unsigned put(encoded_t data);
		virtual encoded_t buf(void);
		virtual unsigned put(void);
		virtual unsigned push(encoded_t data, unsigned size);
		virtual unsigned pull(encoded_t data, unsigned size);
		virtual bool fill(void);
		virtual bool flush(void);
		virtual bool rewind(void);
		virtual bool append(void);
		virtual bool seek(timeout_t position);
		virtual bool trim(timeout_t backup = 0);
		virtual timeout_t skip(timeout_t offset);
		virtual timeout_t locate(void);
		virtual timeout_t length(void);
		virtual void release(void);
		virtual operator bool();
		
		inline state_t status(void)
			{return state;};
	};

	typedef audio::framer *framer_t;

	static void release(framer_t buffer);
	static void u2a(encoded_t data, unsigned samples);
	static void a2u(encoded_t data, unsigned samples);
	static linear_t expand(encoded_t ulaw, unsigned samples, linear_t target = NULL);
	static encoded_t repack(encoded_t ulaw, unsigned samples, linear_t source = NULL);
	static int16_t dbm(float dbm);
	static float dbm(int16_t linear);
	static void init(void);
};

/**
 * Convert audio media.  This class is derived into media encoding format 
 * specific derived codec classes to support transcoding audio, usually
 * to and from ulaw or linear audio frames.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT audiocodec : public LinkedObject, public audio
{
protected:
	friend class audiobuffer;

	audiocodec();
	audiocodec(const audiocodec *original);

	timeout_t framing;		// framing for this codec...
	unsigned long rate;		// rate for this codec...
	
public:
	type_t type;

	/**
	 * Derived copy operation.  This is used to create a new local 
	 * instance with initialized state inside another object.  Commonly
	 * used to embed a codec into an audio buffer.
	 * @param destination address to create object in, NULL if new.
	 * @param framing to use if inserting a streaming codec.
	 * @return address of object created.
	 */
	virtual audiocodec *create(caddr_t destination = NULL, timeout_t framing = 0) const = 0;

	/**
	 * Basic codec type we are derived from.  This is a pointer to a static
	 * object and is a kind of "type" system.
	 * @return base object type pointer.
	 */
	virtual const audiocodec *basetype(void) const = 0;

	/**
	 * Get minimum i/o transfer size to determine frame size when variable
	 * framing is used.
	 * @return same as maximum if fixed framing, else minimum io.
	 */
	virtual unsigned getMinimum(void) const;

	/**
	 * Get actual size of frame from frame header data.
	 * @param encoded audio frame.
	 * @return size of encoded frame.
	 */
	virtual unsigned getActual(audio::encoded_t encoded) const = 0;

	/**
	 * Get maximum frame size for a given frame time.
	 * @return maximum size of frame for framing, or 0 if invalid.
	 */
	virtual unsigned getMaximum(void) const = 0;

	/**
	 * Get constant frame size if fixed, otherwise 0 if variable framing.
	 * @return constant frame size or 0.
	 */
	virtual unsigned getBuffered(void) const = 0;

	/**
	 * Get time impulse of frame.
	 * @return actual framing that will be used.
	 */
	virtual timeout_t getFraming(void) const;

	/**
	 * Get the effective clock speed.
	 * @return effective clock rate.
	 */
	virtual unsigned long getClock(void) const;

	/**
	 * Get channel count for format.
	 * @return number of channels.
	 */
	virtual unsigned getChannels(void) const;

	/**
	 * Get autype number for .au files, if exists.
	 * @return autype or 0 if not valid for au files.
	 */
	virtual unsigned autype(void) const;

	/**
	 * Get data alignment requirements for direct i/o operations.
	 * @return data alignment or 0.
	 */
	virtual unsigned getAlignment(void) const;

	/**
	 * Get the buffer size we use.
	 * @return size of buffer.
	 */
	virtual unsigned getBuffering(void) const;

	/**
	 * Get codec name.
	 * @return name of codec.
	 */
	virtual const char *getName(void) const = 0;

	/**
	 * Test by name or extension matching.
	 * @param name to check.
	 * @return true if this is us.
	 */
	virtual bool test(const char *name) const = 0;

	/**
	 * Get effective size of codec, including any additional buffering
	 * needed to support the object if staged buffering is needed.
	 * @return size of object + buffer.
	 */
	virtual unsigned getSize(void) const = 0;

	/**
	 * Convert linear data to codec format.
	 * @param linear samples to convert.
	 * @param encoded result.
	 * @return bytes encoded out.
	 */
	virtual unsigned encode(linear_t linear, encoded_t encoded);

	/**
	 * Convert codec to linear data format.
	 * @param encoded samples to convert.
	 * @param linear result.
	 * @return bytes encoded out.
	 */
	virtual unsigned decode(encoded_t encoded, linear_t linear);

	/**
	 * Reset state information.  This is used to clear lookahead states
	 * when switching files or streams.
	 */
	virtual void reset(void);

	/**
	 * Find codec by family name.
	 * @param name or extension to search for.
	 * @param channel codec property to optionally match with.
	 * @return codec object to use.
	 */
	static const audiocodec *find(const char *name, const audiocodec *channel = NULL);

	/**
	 * Get an instance of a audiocodec by family name.
	 * @param name or extension to search for.
	 * @param channel codec property to optionally match with.
	 * @return codec instance to use.
	 */
	static audiocodec *get(const char *name, const audiocodec *channel = NULL);

	/**
	 * Get a listing of installed plugins.  This can be walked with
	 * the linked_pointer<> template.
	 * @return list of plugins.
	 */
	static audiocodec *first(void);
};

/**
 * Generic audio buffering and transcoding for audio sources and sinks.
 * This is used to create working storage to support access to and
 * conversion from different audio sources, such as files or tone generators.
 * These can all manipulate this common audiobuffer base class that has space
 * for audio conversion buffers.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT audiobuffer : public audio::framer, public audio
{
private:
	static audiobuffer *alloc(const audiocodec *channel, timeout_t framing = 0);

	size_t offset;

protected:
	unsigned long iocount;
	timeout_t limit;
	encoded_t buffer;
	unsigned backstore, allocsize, backpos, backio, convert, buffering;
	union {
		audiocodec *channel;
		const audiocodec *channelbase;
	};
	audiocodec *encoding;

	bool assign(size_t size, const audiocodec *format = NULL, timeout_t framing = 0);

	inline encoded_t getBuffer(void)
		{return buffer;};

	inline linear_t getConvert(void)
		{return (linear_t)(((caddr_t)(this)) + convert);};

	encoded_t transcode(encoded_t source, encoded_t target, audiocodec *from, audiocodec *to);
	bool transbuffer(audiocodec *from, audiocodec *to);

public:
	timeout_t frametime(void);
	unsigned framesize(void);
	unsigned framesize(encoded_t data);

	/**
	 * Get number of i/o context switches performed for profiling.
	 * @return count of i/o operations since last allocate.
	 */
	inline unsigned long getContexts(void)
		{return iocount;};

	inline unsigned getConversion(void)
		{return backstore - convert;};

	inline unsigned getAvailable(void)
		{return allocsize - (unsigned)(buffer - ((encoded_t)(this)));};

	inline unsigned getBuffering(void)
		{return buffering;};

	inline const audiocodec *getChannel(void)
		{return channel;};

	inline const audiocodec *getEncoding(void)
		{return encoding;};

	inline unsigned getBackstore(void)
		{return allocsize - backstore;};

	inline unsigned getAllocated(void)
		{return allocsize;};
	
	/**
	 * Unpack data pointer using convert buffer as working space.  This
	 * offers a convenient way to get a linear sample out of an existing
	 * encoded frames without having to allocate an additional buffer.  The
	 * encoded data is assumed to be in the format of the frame.
	 */
	linear_t expand(encoded_t data);

	virtual void release(void);
	virtual fd_t handle(void);

	unsigned text(char *str, size_t size);
	unsigned pull(encoded_t data, unsigned len);
	unsigned push(encoded_t data, unsigned len);

	static audiobuffer *create(timeout_t limit);
	static audiobuffer *create(const audiocodec *channel, timeout_t framing);
};

/**
 * Basic audio file access class.  This holds common methods used for
 * manipulating audio files through frame buffering.  Additional reader
 * and writer objects are derived to handle file access specific to each.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT audiofile : public audiobuffer
{
protected:
	fsys::offset_t hiwater, current, headersize;
	size_t maxio, minio, bufio;
	unsigned bufpos, bufend;

	uint8_t header[40];	// header buffer
	enum {RAW, AU, RIFF, WAV} filetype;
	fsys fs;

	void open(const char *path, fsys::access_t access, size_t objsize, timeout_t framing);
	bool rewind(void);
	bool append(void);
	bool flush(void);
	timeout_t length(void);
	timeout_t locate(void);
	bool seek(timeout_t position);
	bool trim(timeout_t backup);
	timeout_t skip(timeout_t offset);
	fd_t handle(void);
	operator bool();

public:
	static bool info(audiobuffer *buffer, const char *path, timeout_t framing = 0);

	/**
	 * Access an existing audio file to play.
	 * @param buffer to stream frames through.
	 * @param path of file to access.
	 * @param preferred I/O framing.
	 * @return true if success.
	 */
	static bool access(audiobuffer *buffer, const char *path, timeout_t preferred = 0);

	/**
	 * Create a new file to record audio into.
	 * @param buffer to stream frames through.
	 * @param path of file to access.
	 * @param preferred I/O framing.
	 * @param note to attach if format allows, NULL if none.
	 * @param encoding of target or 0 for default.
	 * @param mode of access for new file.
	 * @return true if success.
	 */
	static bool create(audiobuffer *buffer, const char *path, timeout_t preferred = 0, const char *note = NULL, const audiocodec *encoding = NULL, unsigned mode = 0640);

	/**
	 * Append audio to an existing file.
	 * @param buffer to use for appending frames.
	 * @param path of file to append.
	 * @param preferred I/O framing.
	 * @return true if success.
	 */
	static bool append(audiobuffer *buffer, const char *path, timeout_t preferred = 0);
};

/**
 * Tone generator class for producing convertable tones.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT tonegen : private audiobuffer
{
};

END_NAMESPACE

#endif


