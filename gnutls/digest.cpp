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
    hashid = 0;
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;
}

Digest::Digest(const char *type)
{
    hashtype = NULL;
    context = NULL;
    bufsize = 0;
    textbuf[0] = 0;

    set(type);
}

Digest::~Digest()
{
    release();
}

void Digest::release(void)
{
    if(context) {
        gcry_md_close((MD_CTX)context);
        context = NULL;
    }

    bufsize = 0;
    textbuf[0] = 0;
}

void Digest::set(const char *type)
{
    secure::init();

    release();

    if(case_eq(type, "sha"))
        type = "sha1";

    hashid = gcry_md_map_name(type);
    if(hashid == GCRY_MD_NONE)
        return;

    gcry_md_open((MD_CTX *)&context, hashid, 0);
}

bool Digest::is(const char *type)
{
    if(case_eq(type, "sha"))
        type = "sha1";

    return gcry_md_map_name(type) != GCRY_MD_NONE;
}

bool Digest::put(const void *address, size_t size)
{
    if(!context)
        return false;

    gcry_md_write((MD_CTX)context, address, size);
    return true;
}

const char *Digest::c_str(void)
{
    if(!bufsize)
        get();

    return textbuf;
}

void Digest::reset(void)
{
    if(!context) {
        if(hashid == GCRY_MD_NONE)
            return;

        gcry_md_open((MD_CTX *)&context, hashid, 0);
    }
    else
        gcry_md_reset((MD_CTX)context);

    bufsize = 0;
}

void Digest::recycle(bool bin)
{
    unsigned size = bufsize;

    if(!context)
        return;

    if(!bufsize) {
        gcry_md_final((MD_CTX)context);
        size = gcry_md_get_algo_dlen(hashid);
        memcpy(buffer, gcry_md_read((MD_CTX)context, hashid), size);
    }

    gcry_md_reset((MD_CTX)context);

    if(bin)
        gcry_md_write((MD_CTX)context, buffer, size);
    else {
        unsigned count = 0;

        while(count < size) {
            snprintf(textbuf + (count * 2), 3, "%2.2x",
buffer[count]);
            ++count;

        }
        gcry_md_write((MD_CTX)context, textbuf, size * 2);
    }
    bufsize = 0;
}

const unsigned char *Digest::get(void)
{
    unsigned count = 0;
    unsigned size = 0;

    if(bufsize)
        return buffer;

    if(!context)
        return NULL;

    gcry_md_final((MD_CTX)context);
    size = gcry_md_get_algo_dlen(hashid);
    memcpy(buffer, gcry_md_read((MD_CTX)context, hashid), size);

    release();

    bufsize = size;

    while(count < bufsize) {
        snprintf(textbuf + (count * 2), 3, "%2.2x",
buffer[count]);
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

