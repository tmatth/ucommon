// Copyright (C) 2010 David Sugar, Tycho Softworks.
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
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <ucommon/ucommon.h>

using namespace UCOMMON_NAMESPACE;

static shell::flagopt helpflag('h',"--help",	_TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::flagopt rflag('r',	"--reverse", _TEXT("reverse order of arguments"));
static shell::flagopt lines('l',	"--lines", _TEXT("list arguments on separate lines"));
static shell::charopt delim('d',	"--delim", _TEXT("set deliminter between arguments"));
static shell::stringopt quote('q',	"--quote",	_TEXT("set quote for each argument"), "string", "");

static char prefix[80] = {0, 0};
static char suffix[80] = {0, 0};

static void output(bool middle, const char *arg)
{
	if(is(lines))
		shell::printf("%s%s%s\n", prefix, arg, suffix);
	else if(middle)
		shell::printf("%c%s%s%s", *delim, prefix, arg, suffix);
	else
		shell::printf("%s%s%s", prefix, arg, suffix);
}

extern "C" int main(int argc, char **argv)
{	
	unsigned count = 0;
	char *ep;
	bool middle = false;

	// default bind based on argv0, so we do not have to be explicit...
	// shell::bind("args");
	shell args(argc, argv);

	if(is(helpflag) || is(althelp)) {
		printf("%s\n", _TEXT("Usage: %s [options] arguments..."), args.argv0());
		printf("%s\n\n", _TEXT("Echo command line arguments"));
		printf("%s\n", _TEXT("Options:"));
		shell::help();
		printf("\n%s\n", _TEXT("Report bugs to dyfet@gnu.org"));
		exit(0);
	}

	if(!args())
		return 0;

	if((*quote)[0]) {
		if(!(*quote)[1]) {
			prefix[0] = (*quote)[0];
			suffix[0] = (*quote)[0];
		}
		else if(!(*quote)[2]) {
			prefix[0] = (*quote)[0];
			suffix[0] = (*quote)[1];
		}
		else if((*quote)[0] == '<') {
			String::set(prefix, sizeof(prefix), *quote);
			snprintf(suffix, sizeof(suffix), "</%s", *quote + 1);
		}
		else if((*quote)[0] == '(') {
			String::set(prefix, sizeof(prefix), quote);
			ep = strchr((char *)*quote, ')');
			if(ep)
				*ep = 0;
			suffix[0] = ')';
		}
		else {
			String::set(prefix, sizeof(prefix), quote);
			String::set(suffix, sizeof(suffix), quote);
		}	
	}

	if(is(rflag)) {
		count = args();
		while(count--) {
			output(middle, args[count]);
			middle = true;
		}
	}
	else while(count < args()) {
		output(middle, args[count++]);
		middle = true;
	}

	if(!lines)
		shell::printf("\n");

	return 0;
}

