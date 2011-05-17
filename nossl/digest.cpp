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

#define MAX_DIGEST_SIZE 20

Digest::Digest()
{
    hashtype = (void *)" ";
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;
}

Digest::Digest(const char *type)
{
    hashtype = (void *)" ";
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;

    set(type);
}

Digest::~Digest()
{
    release();
}

bool Digest::is(const char *id)
{
    if(ieq(id, "md5"))
        return true;

    return false;
}

void Digest::set(const char *type)
{
    release();

    if(ieq(type, "md5")) {
        hashtype = "m";
        context = new md5_state_t;
        md5_init((md5_state_t*)context);
    }
}

void Digest::release(void)
{

    if(context) {
        switch(*((char *)hashtype)) {
        case 'm':
            delete (md5_state_t*)context;
            break;
        default:
            break;
        }
        context = NULL;
    }

    bufsize = 0;
    textbuf[0] = 0;

    hashtype = " ";
}

bool Digest::put(const void *address, size_t size)
{
    if(!context)
        return false;

    switch(*((char *)hashtype)) {
    case 'm':
        md5_append((md5_state_t*)context, (const unsigned char *)address, size);
        return true;
    default:
        return false;
    }
}

const char *Digest::c_str(void)
{
    if(!bufsize)
        get();

    return textbuf;
}

void Digest::reset(void)
{
    switch(*((char *)hashtype)) {
    case 'm':
        if(!context)
            context = new md5_state_t;
        md5_init((md5_state_t*)context);
        break;
    default:
        break;
    }
    bufsize = 0;
}

void Digest::recycle(bool bin)
{
    unsigned size = bufsize;

    if(!context)
        return;

    switch(*((char *)hashtype)) {
    case 'm':
        if(!bufsize)
            md5_finish((md5_state_t*)context, buffer);
        size = 16;
        md5_init((md5_state_t*)context);
        if(bin)
            md5_append((md5_state_t*)context, (const unsigned char *)buffer, size);
        else {
            unsigned count = 0;
            while(count < bufsize) {
                snprintf(textbuf + (count * 2), 3,
"%2.2x", buffer[count]);
                ++count;
            }
            md5_append((md5_state_t*)context, (const unsigned
char *)textbuf, size * 2);
        }
        break;
    default:
        break;
    }
    bufsize = 0;
}

const unsigned char *Digest::get(void)
{
    unsigned count = 0;

    if(bufsize)
        return buffer;

    if(!context)
        return NULL;

    switch(*((char *)hashtype)) {
    case 'm':
        md5_finish((md5_state_t*)context, buffer);
        release();
        bufsize = 16;
        break;
    default:
        break;
    }

    while(count < bufsize) {
        snprintf(textbuf + (count * 2), 3, "%2.2x", buffer[count]);
        ++count;
    }
    return buffer;
}

void Digest::uuid(char *str, const char *name, const unsigned char *ns)
{
    unsigned mask = 0x50;
    const char *type = "sha1";
    if(!is("sha1")) {
        mask = 0x30;
        type = "md5";
    }

    Digest md(type);
    if(ns)
        md.put(ns, 16);
    md.puts(name);
    unsigned char *buf = (unsigned char *)md.get();

    buf[6] &= 0x0f;
    buf[6] |= mask;
    buf[8] &= 0x3f;
    buf[8] |= 0x80;

    String::hexdump(buf, str, "4-2-2-2-6");
}

String Digest::uuid(const char *name, const unsigned char *ns)
{
    char buf[38];
    uuid(buf, name, ns);
    return String(buf);
}

