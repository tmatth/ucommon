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

#include "../config.h"
#include <ucommon/export.h>
#include <ucommon/object.h>
#include <ucommon/memory.h>
#include <ucommon/thread.h>
#include <ucommon/string.h>
#include <ucommon/fsys.h>
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>
#include <string.h>
#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

extern "C" {

    static int ncompare(const void *o1, const void *o2)
    {
        assert(o1 != NULL);
        assert(o2 != NULL);
        const stringpager::member * const *n1 = static_cast<const stringpager::member * const*>(o1);
        const stringpager::member * const *n2 = static_cast<const stringpager::member * const*>(o2);
        return String::collate((*n1)->get(), (*n2)->get());
    }
}

memalloc::memalloc(size_t ps)
{
#ifdef  HAVE_SYSCONF
    size_t paging = sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
    size_t paging = PAGESIZE;
#elif defined(PAGE_SIZE)
    size_t paging = PAGE_SIZE;
#else
    size_t paging = 1024;
#endif
    if(!ps)
        ps = paging;
    else if(ps > paging)
        ps = (((ps + paging - 1) / paging)) * paging;

#ifdef  HAVE_POSIX_MEMALIGN
    if(ps >= paging)
        align = sizeof(void *);
    else
        align = 0;

    switch(align)
    {
    case 2:
    case 4:
    case 8:
    case 16:
        break;
    default:
        align = 0;
    }
#endif
    pagesize = ps;
    count = 0;
    limit = 0;
    page = NULL;
}

memalloc::~memalloc()
{
    memalloc::purge();
}

unsigned memalloc::utilization(void)
{
    unsigned long used = 0, alloc = 0;
    page_t *mp = page;

    while(mp) {
        alloc += pagesize;
        used += mp->used;
        mp = mp->next;
    }

    if(!used)
        return 0;

    alloc /= 100;
    used /= alloc;
    return used;
}

void memalloc::purge(void)
{
    page_t *next;
    while(page) {
        next = page->next;
        free(page);
        page = next;
    }
    count = 0;
}

memalloc::page_t *memalloc::pager(void)
{
    page_t *npage = NULL;
#ifdef  HAVE_POSIX_MEMALIGN
    void *addr;
#endif

    crit(!limit || count < limit, "mempager limit reached");

#ifdef  HAVE_POSIX_MEMALIGN
    if(align && !posix_memalign(&addr, align, pagesize)) {
        npage = (page_t *)addr;
        goto use;
    }
#endif
    npage = (page_t *)malloc(pagesize);

#ifdef  HAVE_POSIX_MEMALIGN
use:
#endif
    crit(npage != NULL, "mempager alloc failed");

    ++count;
    npage->used = sizeof(page_t);
    npage->next = page;
    page = npage;
    if((size_t)(npage) % sizeof(void *))
        npage->used += sizeof(void *) - ((size_t)(npage) % sizeof(void
*));
    return npage;
}

void *memalloc::_alloc(size_t size)
{
    assert(size > 0);

    caddr_t mem;
    page_t *p = page;

    crit(size <= (pagesize - sizeof(page_t)), "mempager alloc failed");

    while(size % sizeof(void *))
        ++size;

    while(p) {
        if(size <= pagesize - p->used)
            break;
        p = p->next;
    }
    if(!p)
        p = pager();

    mem = ((caddr_t)(p)) + p->used;
    p->used += size;
    return mem;
}

mempager::mempager(size_t ps) :
memalloc(ps)
{
    pthread_mutex_init(&mutex, NULL);
}

mempager::~mempager()
{
    memalloc::purge();
    pthread_mutex_destroy(&mutex);
}

void mempager::_lock(void)
{
    pthread_mutex_lock(&mutex);
}

void mempager::_unlock(void)
{
    pthread_mutex_unlock(&mutex);
}

unsigned mempager::utilization(void)
{
    unsigned long used;

    pthread_mutex_lock(&mutex);
    used = memalloc::utilization();
    pthread_mutex_unlock(&mutex);
    return used;
}

void mempager::purge(void)
{
    pthread_mutex_lock(&mutex);
    memalloc::purge();
    pthread_mutex_unlock(&mutex);
}

void mempager::dealloc(void *mem)
{
}

void *mempager::_alloc(size_t size)
{
    assert(size > 0);

    void *mem;
    pthread_mutex_lock(&mutex);
    mem = memalloc::_alloc(size);
    pthread_mutex_unlock(&mutex);
    return mem;
}

stringpager::member::member(LinkedObject **root, const char *data) :
LinkedObject(root)
{
    text = data;
}

stringpager::member::member(const char *data) :
LinkedObject()
{
    text = data;
}

stringpager::stringpager(size_t size) :
memalloc(size)
{
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
}

stringpager::stringpager(char **list, size_t size) :
memalloc(size)
{
    members = 0;
    root = NULL;
    last = NULL;
    add(list);
}

unsigned stringpager::token(const char *text, const char *list, const char *quote, const char *end)
{
    unsigned count = 0;
    const char *result;
    char *last = NULL;

    if(!text || !*text)
        return 0;

    strdup_t tmp = strdup(text);
    while(NULL != (result = String::token(tmp, &last, list, quote, end))) {
        ++count;
        add(result);
    }
    return count;
}

String stringpager::join(const char *prefix, const char *middle, const char *suffix)
{
    string_t tmp;

    if(!members)
        return tmp;

    if(prefix && *prefix)
        tmp += prefix;

    linked_pointer<member> mp = root;
    while(is(mp)) {
        tmp += mp->text;
        if(mp->next && middle && *middle)
            tmp += middle;
        else if(mp->next == NULL && suffix && *suffix)
            tmp += suffix;
        mp.next();
    }

    return tmp;
}

unsigned stringpager::split(const char *text, const char *string, unsigned flags)
{
    strdup_t tmp = String::dup(string);
    char *match = tmp;
    char *prior = tmp;
    size_t tcl = strlen(text);
    unsigned count = 0;
    bool found = false;

    // until end of string or end of matches...
    while(prior && *prior && match) {
#if defined(_MSWINDOWS_)
        if((flags & 0x01) == String::INSENSITIVE)
            match = strstr(prior, text);
#elif  defined(HAVE_STRICMP)
        if((flags & 0x01) == String::INSENSITIVE)
            match = stristr(prior, text);
#else
        if((flags & 0x01) == String::INSENSITIVE)
            match = strcasestr(prior, text);
#endif
        else
            match = strstr(prior, text);

        if(match)
            found = true;

        // we must have at least one match to add trailing data...
        if(match == NULL && prior != NULL && found) {
            ++count;
            add(prior);
        }
        // if we have a current match see if anything to add in front of it
        else if(match) {
            *match = 0;
            if(match > prior) {
                ++count;
                add(prior);
            }

            prior = match + tcl;
        }
    }
    return count;
}

void stringpager::set(unsigned index, const char *text)
{
    linked_pointer<member> list = root;

    if(index >= members)

    while(index--)
        list.next();

    size_t size = strlen(text) + 1;
    char *str = (char *)memalloc::_alloc(size);
    strcpy(str, text);
    list->text = str;
}

const char *stringpager::get(unsigned index)
{
    linked_pointer<member> list = root;

    if(index >= members)
        return NULL;

    while(index--)
        list.next();

    return list->get();
}

void stringpager::clear(void)
{
    memalloc::purge();
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
}

const char *stringpager::pull(void)
{
    if(!members)
        return NULL;


    member *mem = (member *)root;
    const char *result = mem->text;
    --members;
    if(!members) {
        root = NULL;
        last = NULL;
    }
    else
        root = mem->next;
    index = NULL;
    return result;
}

void stringpager::push(const char *text)
{
    if(!text)
        text = "";

    size_t size = strlen(text) + 1;
    caddr_t mem = (caddr_t)memalloc::_alloc(sizeof(member));
    char *str = (char *)memalloc::_alloc(size);

    strcpy(str, text);
    member *node;

    node = new(mem) member(&root, str);
    if(!last)
        last = node;
    ++members;
    index = NULL;
}

const char *stringpager::pop(void)
{
    const char *out = NULL;

    if(!root)
        return NULL;

    index = NULL;

    if(root == last) {
        out = last->text;
        root = last = NULL;
        members = 0;
        return out;
    }

    linked_pointer<member> np = root;
    while(is(np)) {
        if(np->next == last) {
            out = last->text;
            last = *np;
            np->next = NULL;
            --members;
            break;
        }
        np.next();
    }
    return out;
}

void stringpager::add(const char *text)
{
    if(!text)
        text = "";

    size_t size = strlen(text) + 1;
    caddr_t mem = (caddr_t)memalloc::_alloc(sizeof(member));
    char *str = (char *)memalloc::_alloc(size);

    strcpy(str, text);
    member *node;

    index = NULL;
    if(members++) {
        node = new(mem) member(str);
        last->set(node);
    }
    else
        node = new(mem) member(&root, str);
    last = node;
}

void stringpager::set(char **list)
{
    clear();
    add(list);
}

void stringpager::push(char **list)
{
    const char *cp;
    unsigned index = 0;

    if(!list)
        return;

    while(NULL != (cp = list[index++]))
        push(cp);
}

void stringpager::add(char **list)
{
    const char *cp;
    unsigned index = 0;

    if(!list)
        return;

    while(NULL != (cp = list[index++]))
        add(cp);
}

void stringpager::sort(void)
{
    if(!members)
        return;

    member **list = new member*[members];
    unsigned pos = 0;
    linked_pointer<member> mp = root;

    while(is(mp)) {
        list[pos++] = *mp;
        mp.next();
    }

    qsort(static_cast<void *>(list), members, sizeof(member *), &ncompare);
    root = NULL;
    while(pos)
        list[--pos]->enlist(&root);

    delete list;
    index = NULL;
}

char **stringpager::list(void)
{
    if(index)
        return index;

    unsigned pos = 0;
    index = (char **)memalloc::_alloc(sizeof(char *) * (members + 1));
    linked_pointer<member> mp = root;
    while(is(mp)) {
        index[pos++] = (char *)mp->text;
        mp.next();
    }
    index[pos] = NULL;
    return index;
}

DirPager::DirPager() :
stringpager()
{
    dir = NULL;
}

DirPager::DirPager(const char *path) :
stringpager()
{
    dir = NULL;
    load(path);
}

bool DirPager::filter(const char *fname)
{
    if(*fname != '.')
        return true;
    return false;
}

void DirPager::operator=(const char *path)
{
    dir = NULL;
    clear();
    load(path);
}

bool DirPager::load(const char *path)
{
    fsys_t ds;
    char buffer[128];

    if(!fsys::isdir(path))
        return false;

    dir = dup(path);
    ds.open(path, fsys::ACCESS_DIRECTORY);
    if(!ds)
        return false;

    while(ds.read(buffer, sizeof(buffer)) > 0) {
        if(filter(buffer))
            add(buffer);
    }

    ds.close();
    sort();
    return true;
}

autorelease::autorelease()
{
    pool = NULL;
}

autorelease::~autorelease()
{
    release();
}

void autorelease::release()
{
    LinkedObject *obj;

    while(pool) {
        obj = pool;
        pool = obj->getNext();
        obj->release();
    }
}

void autorelease::operator+=(LinkedObject *obj)
{
    assert(obj != NULL);

    obj->enlist(&pool);
}

PagerObject::PagerObject() :
LinkedObject(NULL), CountedObject()
{
}

void PagerObject::dealloc(void)
{
    pager->put(this);
}

void PagerObject::release(void)
{
    CountedObject::release();
}

PagerPool::PagerPool()
{
    freelist = NULL;
    pthread_mutex_init(&mutex, NULL);
}

PagerPool::~PagerPool()
{
    pthread_mutex_destroy(&mutex);
}

void PagerPool::put(PagerObject *ptr)
{
    assert(ptr != NULL);

    pthread_mutex_lock(&mutex);
    ptr->enlist(&freelist);
    pthread_mutex_unlock(&mutex);
}

PagerObject *PagerPool::get(size_t size)
{
    assert(size > 0);

    PagerObject *ptr;
    pthread_mutex_lock(&mutex);
    ptr = static_cast<PagerObject *>(freelist);
    if(ptr)
        freelist = ptr->next;

    pthread_mutex_unlock(&mutex);

    if(!ptr)
        ptr = new((caddr_t)(_alloc(size))) PagerObject;
    memset(ptr, 0, size);
    ptr->pager = this;
    return ptr;
}

keyassoc::keydata::keydata(keyassoc *assoc, char *kid, unsigned max, unsigned bufsize) :
NamedObject(assoc->root, kid, max)
{
    assert(assoc != NULL);
    assert(kid != NULL && *kid != 0);
    assert(max > 1);

    String::set(text, bufsize, kid);
    data = NULL;
    id = text;
}

keyassoc::keyassoc(unsigned pathmax, size_t strmax, size_t ps) :
mempager(ps)
{
    assert(pathmax > 1);
    assert(strmax > 1);
    assert(ps > 1);

    paths = pathmax;
    keysize = strmax;
    count = 0;

    root = (NamedObject **)_alloc(sizeof(NamedObject *) * pathmax);
    memset(root, 0, sizeof(NamedObject *) * pathmax);
    if(keysize) {
        list = (LinkedObject **)_alloc(sizeof(LinkedObject *) * (keysize / 8));
        memset(list, 0, sizeof(LinkedObject *) * (keysize / 8));
    }
    else
        list = NULL;
}

keyassoc::~keyassoc()
{
    purge();
}

void keyassoc::purge(void)
{
    mempager::purge();
    list = NULL;
    root = NULL;
}

void *keyassoc::locate(const char *id)
{
    assert(id != NULL && *id != 0);

    keydata *kd;

    _lock();
    kd = static_cast<keydata *>(NamedObject::map(root, id, paths));
    _unlock();
    if(!kd)
        return NULL;

    return kd->data;
}

void *keyassoc::remove(const char *id)
{
    assert(id != NULL && *id != 0);

    keydata *kd;
    LinkedObject *obj;
    void *data;
    unsigned path = NamedObject::keyindex(id, paths);
    unsigned size = strlen(id);

    if(!keysize || size >= keysize || !list)
        return NULL;

    _lock();
    kd = static_cast<keydata *>(NamedObject::map(root, id, paths));
    if(!kd) {
        _unlock();
        return NULL;
    }
    data = kd->data;
    obj = static_cast<LinkedObject*>(kd);
    obj->delist((LinkedObject**)(&root[path]));
    obj->enlist(&list[size / 8]);
    --count;
    _unlock();
    return data;
}

bool keyassoc::create(char *id, void *data)
{
    assert(id != NULL && *id != 0);
    assert(data != NULL);

    keydata *kd;
    LinkedObject *obj;
    unsigned size = strlen(id);

    if(keysize && size >= keysize)
        return false;

    _lock();
    kd = static_cast<keydata *>(NamedObject::map(root, id, paths));
    if(kd) {
        _unlock();
        return false;
    }
    caddr_t ptr = NULL;
    size /= 8;
    if(list && list[size]) {
        obj = list[size];
        list[size] = obj->getNext();
        ptr = (caddr_t)obj;
    }
    if(ptr == NULL)
        ptr = (caddr_t)memalloc::_alloc(sizeof(keydata) + size * 8);
    kd = new(ptr) keydata(this, id, paths, 8 + size * 8);
    kd->data = data;
    ++count;
    _unlock();
    return true;
}

bool keyassoc::assign(char *id, void *data)
{
    assert(id != NULL && *id != 0);
    assert(data != NULL);

    keydata *kd;
    LinkedObject *obj;
    unsigned size = strlen(id);

    if(keysize && size >= keysize)
        return false;

    _lock();
    kd = static_cast<keydata *>(NamedObject::map(root, id, paths));
    if(!kd) {
        caddr_t ptr = NULL;
        size /= 8;
        if(list && list[size]) {
            obj = list[size];
            list[size] = obj->getNext();
            ptr = (caddr_t)obj;
        }
        if(ptr == NULL)
            ptr = (caddr_t)memalloc::_alloc(sizeof(keydata) + size * 8);
        kd = new(ptr) keydata(this, id, paths, 8 + size * 8);
        ++count;
    }
    kd->data = data;
    _unlock();
    return true;
}

chartext::chartext()
{
    pos = NULL;
    max = 0;
}

chartext::chartext(char *buf)
{
    pos = buf;
    max = 0;
}

chartext::chartext(char *buf, size_t size)
{
    pos = buf;
    max = size;
}

chartext::~chartext()
{
}

int chartext::_getch(void)
{
    if(!pos || !*pos || max)
        return EOF;

    return *(pos++);
}

int chartext::_putch(int code)
{
    if(!pos || !max)
        return EOF;

    *(pos++) = code;
    *pos = 0;
    --max;
    return code;
}

bufpager::bufpager(size_t ps) :
memalloc(ps)
{
    first = last = current = freelist = NULL;
    ccount = 0;
    cpos = 0;
}

void bufpager::set(const char *text)
{
    reset();
    add(text);
}

void bufpager::put(const char *text, size_t iosize)
{
    _lock();

    while(text && (iosize--)) {
        if(!last || last->used == last->size) {
            cpage_t *next;

            if(freelist) {
                next = freelist;
                freelist = next->next;
            }
            else {
                next = (cpage_t *)memalloc::_alloc(sizeof(cpage_t));
                if(!next) {
                    _unlock();
                    return;
                }

                page_t *p = page;
                unsigned size = 0;

                while(p) {
                    size = pagesize - p->used;
                    if(size)
                        break;
                    p = p->next;
                }
                if(!p)
                    p = pager();

                if(!p) {
                    _unlock();
                    return;
                }

                next->text = ((char *)(p)) + p->used;
                next->used = 0;
                next->size = size;
                p->used = pagesize;
            }

            if(last)
                last->next = next;

            if(!first)
                first = next;
            last = next;
        }

        ++ccount;
        last->text[last->used++] = *(text++);
    }
    _unlock();
}

char *bufpager::copy(size_t *iosize)
{
    *iosize = 0;
    _lock();
    if(!current || (current->next == NULL && cpos >= current->used)) {
        _unlock();
        return NULL;
    }

    if(cpos >= current->used) {
        current = current->next;
        cpos = 0l;
    }

    char *result = current->text + cpos;
    *iosize = current->used - cpos;
    cpos = 0l;
    current = current->next;
    return result;
}

char *bufpager::request(size_t *iosize)
{
    *iosize = 0;
    _lock();
    if(!last || last->used >= last->size) {
        cpage_t *next;
        if(freelist) {
            next = freelist;
            freelist = next->next;
        }
        else {
            next = (cpage_t *)memalloc::_alloc(sizeof(cpage_t));
            if(!next) {
                _unlock();
                return NULL;
            }

            page_t *p = page;
            unsigned size = 0;

            while(p) {
                size = pagesize - p->used;
                if(size)
                    break;
                p = p->next;
            }
            if(!p)
                p = pager();

            if(!p) {
                _unlock();
                return NULL;
            }
            next->text = ((char *)(p)) + p->used;
            next->used = 0;
            next->size = size;
            p->used = pagesize;
        }

        if(last)
            last->next = next;

        if(!first)
            first = next;
        last = next;
    }
    *iosize = last->size - last->used;
    return last->text + last->used;
}

void bufpager::commit(size_t iosize)
{
    last->used += iosize;
    _unlock();
}

void bufpager::release(void)
{
    _unlock();
}

size_t bufpager::get(char *text, size_t iosize)
{
    _lock();
    if(!ccount) {
        _unlock();
        return 0;
    }

    unsigned long index = 0;

    while(index < iosize && current) {
        if(cpos >= current->used) {
            if(!current->next)
                break;
            current = current->next;
            cpos = 0l;
        }
        text[index++] = current->text[cpos++];
    }
    if(index < iosize)
        text[index] = 0;
    _unlock();
    return index;
}

void bufpager::add(const char *text)
{
    _lock();

    while(text && *text) {
        if(!last || last->used == last->size) {
            cpage_t *next;

            if(freelist) {
                next = freelist;
                freelist = next->next;
            }
            else {
                next = (cpage_t *)memalloc::_alloc(sizeof(cpage_t));
                if(!next) {
                    _unlock();
                    return;
                }

                page_t *p = page;
                unsigned size = 0;

                while(p) {
                    size = pagesize - p->used;
                    if(size)
                        break;
                    p = p->next;
                }
                if(!p)
                    p = pager();

                if(!p) {
                    _unlock();
                    return;
                }

                next->text = ((char *)(p)) + p->used;
                next->used = 0;
                next->size = size;
                p->used = pagesize;
            }

            if(last)
                last->next = next;

            if(!first)
                first = next;
            last = next;
        }

        ++ccount;
        last->text[last->used++] = *(text++);
    }
    _unlock();
}

char *bufpager::dup(void)
{
    _lock();
    if(!ccount) {
        _unlock();
        return NULL;
    }

    char *text = (char *)malloc(ccount + 1l);
    if(!text) {
        _unlock();
        return NULL;
    }

    unsigned long index = 0, pos = 0;
    cpage_t *page = first;

    while(index < ccount && page) {
        if(pos >= page->used) {
            if(!page->next)
                break;
            page = page->next;
            pos = 0l;
        }
        text[index++] = page->text[pos++];
    }
    text[index] = 0;
    _unlock();
    return text;
}

int bufpager::_getch(void)
{
    _lock();

    if(!current)
        current = first;

    if(!current) {
        _unlock();
        return EOF;
    }

    if(cpos >= current->used) {
        if(!current->next) {
            _unlock();
            return EOF;
        }
        current = current->next;
        cpos = 0;
    }

    if(cpos >= current->used) {
        _unlock();
        return EOF;
    }

    char ch = current->text[cpos++];
    _unlock();
    return ch;
}

int bufpager::_putch(int code)
{
    _lock();
    if(!last || last->used == last->size) {
        cpage_t *next;

        if(freelist) {
            next = freelist;
            freelist = next->next;
        }
        else {
            next = (cpage_t *)memalloc::_alloc(sizeof(cpage_t));
            if(!next) {
                _unlock();
                return EOF;
            }

            page_t *p = page;
            unsigned size = 0;

            while(p) {
                size = pagesize - p->used;
                if(size)
                    break;
                p = p->next;
            }
            if(!p)
                p = pager();

            if(!p) {
                _unlock();
                return EOF;
            }

            next->text = ((char *)(p)) + p->used;
            next->used = 0;
            next->size = size;
            p->used = pagesize;
        }

        if(last)
            last->next = next;

        if(!first)
            first = next;
        last = next;
    }

    ++ccount;
    last->text[last->used++] = code;
    _unlock();
    return code;
}

void *bufpager::_alloc(size_t size)
{
    _lock();
    void *ptr = memalloc::_alloc(size);
    _unlock();
    return ptr;
}

void bufpager::rewind(void)
{
    _lock();
    cpos = 0;
    current = first;
    _unlock();
}

void bufpager::reset(void)
{
    _lock();
    cpos = 0;
    ccount = 0;
    current = first;
    while(current) {
        current->used = 0;
        current = current->next;
    }
    freelist = first;
    first = last = current = NULL;
    _unlock();
}

charmem::charmem(char *mem, size_t size)
{
    dynamic = false;
    buffer = NULL;
    set(mem, size);
}

charmem::charmem(size_t memsize)
{
    buffer = NULL;
    set(size);
}

charmem::charmem()
{
    buffer = NULL;
    dynamic = false;
    inp = out = size = 0;
}

charmem::~charmem()
{
    release();
}

void charmem::set(size_t total)
{
    release();
    buffer = (char *)malloc(total);
    size = total;
    inp = out = 0;
    buffer[0] = 0;
}

void charmem::set(char *mem, size_t total)
{
    release();

    if(!mem) {
        buffer = NULL;
        inp = out = size = 0;
        return;
    }

    buffer = mem;
    size = total;
    inp = 0;
    out = strlen(mem);
}

void charmem::release(void)
{
    if(buffer && dynamic)
        free(buffer);

    buffer = NULL;
    dynamic = false;
}

int charmem::_getch(void)
{
    if(!buffer || inp == size || buffer[inp] == 0)
        return EOF;

    return buffer[inp++];
}

int charmem::_putch(int code)
{
    if(!buffer || out > size - 1)
        return EOF;

    buffer[out++] = code;
    buffer[out] = 0;
    return code;
}


