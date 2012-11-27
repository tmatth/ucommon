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

#include "local.h"

HMAC::Key::Key(const char *digest)
{
    secure::init();

    set(digest);
}

HMAC::Key::Key(const char *digest, const char *key)
{
    secure::init();
    set(digest);
    assign(key);
}

void HMAC::Key::assign(const char *text, size_t size)
{
    keysize = 0;
}

HMAC::Key::Key()
{
    secure::init();
    clear();
}

HMAC::Key::~Key()
{
    clear();
}

void HMAC::Key::set(const char *digest)
{
    hmacid = 0;
}

void HMAC::Key::clear(void)
{
    hmacid = 0;
    keysize = 0;

    zerofill(keybuf, sizeof(keybuf));
}

HMAC::HMAC()
{
    hmactype = (void *)" ";
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;
}

HMAC::HMAC(key_t key)
{
    hmactype = (void *)" ";
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;

    set(key);
}

HMAC::~HMAC()
{
    release();
}

bool HMAC::has(const char *id)
{
    return false;
}

void HMAC::set(key_t key)
{
    release();
}

void HMAC::release(void)
{
    bufsize = 0;
    textbuf[0] = 0;

    hmactype = " ";
}

bool HMAC::put(const void *address, size_t size)
{
    return false;
}

const char *HMAC::c_str(void)
{
    if(!bufsize)
        get();

    return textbuf;
}

const unsigned char *HMAC::get(void)
{
    if(bufsize)
        return buffer;

    return NULL;
}

