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

#include <config.h>
#include <ucommon/linked.h>
#include <ucommon/memory.h>
#include <ucommon/keydata.h>
#include <ucommon/string.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

keydata::keyvalue::keyvalue(keyfile *allocator, keydata *section, const char *kv, const char *dv) :
OrderedObject(&section->index)
{
    assert(allocator != NULL);
    assert(section != NULL);
    assert(kv != NULL);

    id = allocator->dup(kv);

    if(dv)
        value = allocator->dup(dv);
    else
        value = "";
}

keydata::keydata(keyfile *file, const char *id) :
OrderedObject(&file->index), index()
{
    assert(file != NULL);
    assert(id != NULL);

    name = file->dup(id);
    root = file;
}

keydata::keydata(keyfile *file) :
OrderedObject(), index()
{
    root = file;
    name = "-";
}

const char *keydata::get(const char *key) const
{
    assert(key != NULL);

    iterator keys = begin();

    while(is(keys)) {
        if(ieq(key, keys->id))
            return keys->value;
        keys.next();
    }
    return NULL;
}

void keydata::clear(const char *key)
{
    assert(key != NULL);

    iterator keys = begin();

    while(is(keys)) {
        if(ieq(key, keys->id)) {
            keys->delist(&index);
            return;
        }
        keys.next();
    }
}

void keydata::set(const char *key, const char *value)
{
    assert(key != NULL);
    assert(value != NULL);

    caddr_t mem = (caddr_t)root->alloc(sizeof(keydata::keyvalue));
    keydata::iterator keys = begin();

    while(is(keys)) {
        if(ieq(key, keys->id)) {
            keys->delist(&index);
            break;
        }
        keys.next();
    }
    new(mem) keydata::keyvalue(root, this, key, value);
}


keyfile::keyfile(size_t pagesize) :
memalloc(pagesize), index()
{
    errcode = 0;
    defaults = NULL;
}

keyfile::keyfile(const char *path, size_t pagesize) :
memalloc(pagesize), index()
{
    errcode = 0;
    defaults = NULL;
    load(path);
}

void keyfile::release(void)
{
    defaults = NULL;
    index.reset();
    memalloc::purge();
}

keydata *keyfile::get(const char *key) const
{
    assert(key != NULL);

    iterator keys = begin();

    while(is(keys)) {
        if(ieq(key, keys->name))
            return *keys;
        keys.next();
    }
    return NULL;
}

keydata *keyfile::create(const char *id)
{
    assert(id != NULL);

    caddr_t mem = (caddr_t)alloc(sizeof(keydata));
    keydata *old = get(id);

    if(old)
        old->delist(&index);

    return new(mem) keydata(this, id);
}

void keyfile::load(const char *path)
{
    assert(path != NULL && path[0] != 0);

    char linebuf[1024];
    char *lp = linebuf;
    char *ep;
    unsigned size = sizeof(linebuf);
    FILE *fp = fopen(path, "r");
    keydata *section = NULL;
    const char *key;
    char *value;

    errcode = 0;

    if(!fp) {
        errcode = EBADF;
        return;
    }

    if(!defaults) {
        caddr_t mem = (caddr_t)alloc(sizeof(keydata));
        defaults = new(mem) keydata(this);
    }

    for(;;) {
        *lp = 0;
        if(NULL == fgets(lp, size, fp)) {
            errcode = ferror(fp);
            lp[0] = 0;
        }
        else
            String::chop(lp, "\r\n\t ");
        ep = lp + strlen(lp);
        if(ep != lp) {
            --ep;
            if(*ep == '\\') {
                lp = ep;
                size = (linebuf + sizeof(linebuf) - ep);
                continue;
            }
        }
        if(!linebuf[0] && feof(fp))
            break;

        lp = linebuf;
        while(isspace(*lp))
            ++lp;

        if(!*lp)
            goto next;

        if(*lp == '[') {
            ep = strchr(lp, ']');
            if(!ep)
                goto next;
            *ep = 0;
            section = create(String::strip(++lp, " \t"));
            goto next;
        }
        else if(!isalnum(*lp) || !strchr(lp, '='))
            goto next;

        ep = strchr(lp, '=');
        *ep = 0;
        key = String::strip(lp, " \t");
        value = String::strip(++ep, " \t\r\n");
        value = String::unquote(value, "\"\"\'\'{}()");
        if(section)
            section->set(key, value);
        else
            defaults->set(key, value);
next:
        lp = linebuf;
        size = sizeof(linebuf);
    }
    fclose(fp);
}

