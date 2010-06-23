// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
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

#ifndef	DEBUG
#define	DEBUG
#endif

#include <config.h>
#include <ucommon/secure.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

int main(int argc, char **argv)
{
    secure::init();

    secure::context_t ctx = secure::client();

    printf("ctx %p %d\n", (void *)ctx, ctx->err());

    ssl_t ssl("https", ctx);
    ssl.open("www.google.com");
    printf("open %d\n", ssl.isopen());

	ssl.writes("GET /\r\n\r\n");
    ssl.flush();

    char buf[256];

    while(!ssl.eof()) {
        ssl.getline(buf, sizeof(buf));
        printf("%s\n", buf);
    }

    ssl.close();
}


