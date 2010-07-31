// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2009 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <ucommon/audio.h>
#include <cstdio>
#include <config.h>

static const  char *delfile = NULL;
static bool	verbose = false;

using namespace UCOMMON_NAMESPACE;

static const char *note = NULL;
static const audiocodec *oformat = NULL;
static const audiocodec *encoding = NULL;
static audiobuffer *input = NULL, *output = NULL;
static timeout_t framing = 0;
static timeout_t buffering = 120;

extern "C" {
#ifdef  _MSWINDOWS_
	static BOOL WINAPI down(DWORD ctrltype)
	{
		if(delfile) {
			remove(delfile);
			delfile = NULL;
		}
		exit(ctrltype);
		return TRUE;
	}
#else
	static void down(int signo)
	{
		if(delfile)
			remove(delfile);
		exit(signo);
	}
#endif
}

extern "C" {
	static void stop(void)
	{
		if(input)
			audio::release(input);
		if(output)
			audio::release(output);
		input = output = NULL;
	}
}

static void codecs(void)
{
	linked_pointer<audiocodec> ac = audiocodec::first();

	while(is(ac)) {
		printf("%s %ldmsec\n", ac->getName(), ac->getFraming());
		ac.next();
	}
	exit(0);
}

static void usage(void)
{
	printf("Usage: audiotool [options] command files...\n"
		"Options:\n");
	shell::help();
	printf("Commands:\n"
		"  append file files            Append audio to existing file\n"
		"  create newfile files         Create new file from existing ones\n"
		"  info files                   Display info for specified audio files\n"
#ifndef	_MSWINDOWS_
		"  pipe files                   Pipe raw transcoded audio to stdout\n"
#endif
		"  text file                    Display annotation for audio file if set\n"
		"  verify file                  Verify readability of an audio file\n"
	);
	printf("Report bugs to sipwitch-devel@gnu.org\n");
    exit(0);
}
	
static void version(void)
{
	printf("audiotool " VERSION "\n"
		"Copyright (C) 2009-2010 David Sugar, Tycho Softworks\n"
		"This is free software; see the source for copying conditions.  There is NO\n"
		"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
    exit(0);
}

static void text(const char *fn)
{
	char buf[256];

	audiofile::info(input, fn, framing);
	switch(input->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: missing or inaccessible\n", fn);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", fn);
		exit(2);
	default:
		break;
	}
	input->text(buf, sizeof(buf));
	if(buf[0])
		printf("%s\n", buf);
	exit(0);
}

static void info(const char *fn)
{
	char buf[256];
	const audiocodec *encoding;
	const char *cp;

	audiofile::info(input, fn, framing);
	switch(input->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: missing or inaccessible\n", fn);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", fn);
		exit(2);
	default:
		break;
	}

	encoding = input->getEncoding();
	cp = strrchr(fn, '/');
	if(!cp)
		cp = strrchr(fn, '\\');
	if(!cp)
		cp = strrchr(fn, ':');
	if(cp)
		++cp;
	else
		cp = fn;

	unsigned used = input->getBuffering() + input->getBackstore() + input->getConversion();
	printf("audiofile %s\n", cp);

	if(verbose) {
		printf("available %u\n", input->getAvailable());
		printf("buffering %u\n", input->getBuffering());
		printf("backstore %u\n", input->getBackstore());
		printf("converter %u\n", input->getConversion());
		printf("underused %u\n", input->getAvailable() - used);
		printf("framesize %u\n", input->framesize());
	}

	printf("encoding  %s\n", encoding->getName());
	printf("framing   %u msec\n", (unsigned)encoding->getFraming());
		input->text(buf, sizeof(buf));

	printf("length    %02lu:%02lu:%02lu.%03lu\n", 
		(input->length() /3600000l),
		(input->length() / 60000l) % 60l,
		(input->length() / 1000l) % 60l, 
		input->length() % 1000l);

	if(buf[0])
		printf("label     %s\n", buf);
}

static void append(const char *to, char **argv)
{
	unsigned count = 0, len;
	audio::encoded_t data;
	const char *from;

	audiofile::append(output, to, framing);
	switch(output->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: cannot access\n", to);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", to);
		exit(2);
	default:
		break;
	}

	while(*argv) {
		from = *(argv++);

		audiofile::access(input, from, framing);
		switch(input->status()) {
		case audio::INVFILE:
			fprintf(stderr, "*** %s: missing or inaccessible\n", from);
			exit(1);
		case audio::INVALID:
			fprintf(stderr, "*** %s: invalid or unsupported encoding\n", from);
			exit(2);
		default:
			break;
		}

		while(NULL != (data = input->get())) {
			len = output->put(data);
			if(!len) {
				fprintf(stderr, "*** %s: output failed\n", to);
				exit(3);
			}
			++count;
		}

		output->release();
		switch(input->status()) {
		case audio::END:
			break;
		default:
			fprintf(stderr, "*** %s: input failed\n", from);
			exit(3);
		}
	}

	if(verbose) {
		printf("converted %u frames\n", count);
		printf("reading %lu contexts\n", input->getContexts());
		printf("writing %lu contexts\n", output->getContexts());
	}

	exit(0);
}

static void create(const char *to, char **argv)
{
	unsigned count = 0, len;
	audio::encoded_t data;
	const char *from;

	::remove(to);
	audiofile::create(output, to, framing, note, oformat);
	switch(output->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: cannot create\n", to);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", to);
		exit(2);
	default:
		break;
	}

	while(*argv) {
		from = *(argv++);

		audiofile::access(input, from, framing);
		switch(input->status()) {
		case audio::INVFILE:
			fprintf(stderr, "*** %s: missing or inaccessible\n", from);
			exit(1);
		case audio::INVALID:
			fprintf(stderr, "*** %s: invalid or unsupported encoding\n", from);
			exit(2);
		default:
			break;
		}

		while(NULL != (data = input->get())) {
			len = output->put(data);
			if(!len) {
				fprintf(stderr, "*** %s: output failed\n", to);
				exit(3);
			}
			++count;
		}
		output->release();

		switch(input->status()) {
		case audio::END:
			break;
		default:
			fprintf(stderr, "*** %s: input failed\n", from);
			exit(3);
		}
	}

	if(verbose) {
		printf("converted %u frames\n", count);
		printf("reading %lu contexts\n", input->getContexts());
		printf("writing %lu contexts\n", output->getContexts());
	}

	exit(0);
}

#ifndef	_MSWINDOWS_
static void pipe(const char *fn)
{
	audio::encoded_t data;
	size_t len = 0;

	audiofile::access(input, fn, framing);
	switch(input->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: missing or inaccessible\n", fn);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", fn);
		exit(2);
	default:
		break;
	}

	while(NULL != (data = input->get())) {
		len = input->framesize(data);
		if(len) {
			if(::write(1, data, len) < (ssize_t)len) {
				fprintf(stderr, "*** %s: output failed\n", fn);
				exit(3);
			}
		}		
	}

	switch(input->status()) {
	case audio::END:
		return;
	default:
		fprintf(stderr, "*** %s: input failed\n", fn);
		exit(3);
	}
}
#endif

static void verify(const char *fn)
{
	unsigned long count = 0;

	audiofile::access(input, fn, framing);
	switch(input->status()) {
	case audio::INVFILE:
		fprintf(stderr, "*** %s: missing or inaccessible\n", fn);
		exit(1);
	case audio::INVALID:
		fprintf(stderr, "*** %s: invalid or unsupported encoding\n", fn);
		exit(2);
	default:
		break;
	}

	while(input->get()) {
		++count;
	}

	if(verbose) {
		printf("read %lu frames\n", count);
		printf("used %lu contexts\n", input->getContexts());
	}

	switch(input->status()) {
	case audio::END:
		exit(0);
	default:
		exit(3);
	}
}

int main(int argc, char **argv)
{
    char *cp = NULL;

	shell::flagopt helpflag('h', "--help", "display this list");
	shell::flagopt althelp('?', NULL, NULL);
		shell::numericopt bufopt('b', "--buffering", "audio buffering (120)", "msec", 120);
	shell::flagopt codecflag('c', "--codecs", "list codecs");
	shell::stringopt encopt('e', "--encoding", "channel encoding (pcmu)", "codec", "pcmu");
	shell::numericopt frameopt('f', "--framing", "audio framing (20)", "msec", 20); 
	shell::stringopt noteopt('n', "--note", "text annotation for new files", "\"text\"", "");
	shell::stringopt outopt('o', "--output", "encoding for new files (pcmu)", "codec", "pcmu");
	shell::flagopt vflag('v', "--verbose", "display extra debugging info");
	shell::flagopt verflag(0, "--version", "display software version");
	shell args(argc, argv);

	audio::init();

	if(is(helpflag) || is(althelp))
		usage();

	if(is(verflag))
		version();

	if(is(codecflag))
		codecs();

	if(is(bufopt)) {
		buffering = (timeout_t) *bufopt;
		if(buffering < 60 || buffering % 10)
			shell::errexit(1, "*** audiotool: --buffering: must be >= 60 msec and multiple of 10msec\n");
	}

	if(is(frameopt)) {
		framing = (timeout_t) *frameopt;
		if(framing % 10 || framing < 10)
			shell::errexit(1, "*** audiotool: --framing: must be multiple of 10 msec\n");
	}

	if(is(encopt)) {
		encoding = audiocodec::find(*encopt);
		if(!encoding)
			shell::errexit(1, "*** audiotool: --encoding: %s: unknown encoding\n", *encopt);
	}

	if(is(outopt)) {
		oformat = audiocodec::find(*outopt);
		if(!oformat)
			shell::errexit(1, "*** audiotool: --output: %s: unknown encoding\n", *outopt);
	}

	if(is(noteopt))
		note = *noteopt;

	atexit(&stop);
	if(!encoding)
		encoding = audiocodec::find("pcmu");

	input = audiobuffer::create(encoding, buffering);
	output = audiobuffer::create(encoding, buffering);
	
#ifdef  _MSWINDOWS_
    SetConsoleCtrlHandler(down, TRUE);
#else
    signal(SIGINT, down);
    signal(SIGTERM, down);
    signal(SIGQUIT, down);
    signal(SIGABRT, down);
#endif	

	argv = args.argv();

	if(argv && *argv)
		cp = *argv;
	else
		usage();

	// check commands...

	while(*cp == '-')
		++cp;

	if(eq(cp, "create")) {
		++argv;
		if(NULL == *argv || NULL == argv[1])
			goto nofiles;
		cp = *(argv++);
		create(cp, argv);
		goto done;
	}

	if(eq(cp, "append")) {
		++argv;
		if(NULL == *argv || NULL == argv[1])
			goto nofiles;
		cp = *(argv++);
		append(cp, argv);
		goto done;
	}
	
	if(eq(cp, "info")) {
		if(NULL == argv[1])
			goto nofiles;
        while(*++argv) {
            info(*argv);
        }
		goto done;
    }

	if(eq(cp, "text")) {
		if(NULL == argv[1])
			goto nofiles;
		if(argv[2])
			goto toomany;
		text(argv[1]);
	}

	if(eq(cp, "verify")) {
		if(NULL == argv[1])
			goto nofiles;
		if(argv[2])
			goto toomany;
		verify(argv[1]);
	}

#ifndef	_MSWINDOWS_
	if(eq(cp, "pipe")) {
		if(NULL == argv[1])
			goto nofiles;
        while(*++argv) {
            pipe(*argv);
        }
		goto done;
    }
#endif

    fprintf(stderr, "*** audiotool: %s: unknown command\n", cp);
    exit(4);

nofiles:
	fprintf(stderr, "*** audiotool: %s: no file(s) to process\n", cp);
	exit(4);

toomany:
	fprintf(stderr, "*** audiotool: %s: too many filenames used\n", cp);
	exit(4);

done:
	return 0;
}

