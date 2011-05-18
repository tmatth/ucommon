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
    if(case_eq(id, "md5"))
        return true;

    if(case_eq(id, "sha1") || case_eq(id, "sha"))
        return true;

    return false;
}

void Digest::set(const char *type)
{
    release();

    if(case_eq(type, "md5")) {
        hashtype = "m";
        context = new MD5_CTX;
        MD5Init((MD5_CTX*)context);
    }
    else if(case_eq(type, "sha") || case_eq(type, "sha1")) {
        hashtype = "s";
        context = new SHA1_CTX;
        SHA1Init((SHA1_CTX*)context);
    }
}

void Digest::release(void)
{

    if(context) {
        switch(*((char *)hashtype)) {
        case 'm':
            delete (MD5_CTX*)context;
            break;
        case 's':
            delete (SHA1_CTX *)context;
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
        MD5Update((MD5_CTX*)context, (const unsigned char *)address, size);
        return true;
    case 's':
        SHA1Update((SHA1_CTX*)context, (const unsigned char *)address, size);
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
            context = new MD5_CTX;
        MD5Init((MD5_CTX*)context);
        break;
    case 's':
        if(!context)
            context = new SHA1_CTX;
        SHA1Init((SHA1_CTX*)context);
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
            MD5Final(buffer, (MD5_CTX*)context);
        size = 16;
        MD5Init((MD5_CTX*)context);
        if(bin)
            MD5Update((MD5_CTX*)context, (const unsigned char *)buffer, size);
        else {
            unsigned count = 0;
            while(count < bufsize) {
                snprintf(textbuf + (count * 2), 3,
"%2.2x", buffer[count]);
                ++count;
            }
            MD5Update((MD5_CTX*)context, (const unsigned
char *)textbuf, size * 2);
        }
        break;
    case 's':
        if(!bufsize)
            SHA1Final(buffer, (SHA1_CTX*)context);
        size = 20;
        SHA1Init((SHA1_CTX*)context);
        if(bin)
            SHA1Update((SHA1_CTX*)context, (const unsigned char *)buffer, size);
        else {
            unsigned count = 0;
            while(count < bufsize) {
                snprintf(textbuf + (count * 2), 3,
"%2.2x", buffer[count]);
                ++count;
            }
            SHA1Update((SHA1_CTX*)context, (const unsigned
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
        MD5Final(buffer, (MD5_CTX*)context);
        release();
        bufsize = 16;
        break;
    case 's':
        SHA1Final(buffer, (SHA1_CTX*)context);
        release();
        bufsize = 20;
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

