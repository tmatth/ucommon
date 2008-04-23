// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

#include <config.h>
#include <ucommon/string.h>
#include <ucommon/thread.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>

using namespace UCOMMON_NAMESPACE;

const strsize_t string::npos = (strsize_t)(-1);
const size_t memstring::header = sizeof(cstring);

string::cstring::cstring(strsize_t size)
{
	max = size;
	len = 0;
	fill = 0;
	text[0] = 0;
}

string::cstring::cstring(strsize_t size, char f)
{
	max = size;
	len = 0;
	fill = f;

	if(fill) {
		memset(text, fill, max);
		len = max;
	}
}

void string::cstring::fix(void)
{
	while(fill && len < max)
		text[len++] = fill;
	text[len] = 0;
}

void string::cstring::unfix(void)
{
	while(len && fill) {
		if(text[len - 1] == fill)
			--len;
	}
	text[len] = 0;
}

void string::cstring::clear(strsize_t offset, strsize_t size)
{
	if(!fill || offset >= max)
		return;

	while(size && offset < max) {
		text[offset++] = fill;
		--size;
	}
}

void string::cstring::dec(strsize_t offset)
{
	if(!len)
		return;

	if(offset >= len) {
		text[0] = 0;
		len = 0;
		fix();
		return;
	}

	if(!fill) {
		text[--len] = 0;
		return;
	}

	while(len) {
		if(text[--len] == fill)
			break;
	}
	text[len] = 0;
	fix();
}

void string::cstring::inc(strsize_t offset)
{
	if(!offset)
		++offset;

	if(offset >= len) {
		text[0] = 0;
		len = 0;
		fix();
		return;
	}

	memmove(text, text + offset, len - offset);
	len -= offset;
	fix();
}

void string::cstring::add(const char *str)
{
	strsize_t size = strlen(str);

	if(!size)
		return;

	while(fill && len && text[len - 1] == fill)
		--len;
	
	if(len + size > max)
		size = max - len;

	if(size < 1)
		return;

	memcpy(text + len, str, size);
	len += size;
	fix();
}

void string::cstring::add(char ch)
{
	if(!ch)
		return;

	while(fill && len && text[len - 1] == fill)
		--len;

	if(len == max)
		return;

	text[len++] = ch;	
	fix();
}

void string::cstring::set(strsize_t offset, const char *str, strsize_t size)
{
	assert(str != NULL);

	if(offset >= max || offset > len)
		return;

	if(offset + size > max)
		size = max - offset;
 
	while(*str && size) {
		text[offset++] = *(str++);
		--size;
	}

	while(size && fill) {
		text[offset++] = fill;
		--size;
	}

	if(offset > len) {
		len = offset;
		text[len] = 0;
	}
}

char string::fill(void)
{
	if(!str)
		return 0;

	return str->fill;
}

strsize_t string::size(void) const
{
    if(!str)
        return 0;

    return str->max;
}

strsize_t string::len(void)
{
	if(!str)
		return 0;

	return str->len;
}

void string::cstring::set(const char *str)
{
	assert(str != NULL);

	strsize_t size = strlen(str);
	if(size > max)
		size = max;
	
	if(str < text || str > text + len)
		memcpy(text, str, size);
	else if(str != text)
		memmove(text, str, size);
	len = size;
	fix();
}		

string::string()
{
	str = NULL;
}

string::string(const char *s, const char *end)
{
	strsize_t size = 0;

	if(!s)
		s = "";
	else if(!end)
		size = strlen(s);
	else if(end > s)
		size = (strsize_t)(end - s);
	str = create(size);
	str->retain();
	str->set(s);
}		

string::string(const char *s)
{
	strsize_t size = count(s);
	if(!s)
		s = "";
	str = create(size);
	str->retain();
	str->set(s);
}

string::string(const char *s, strsize_t size)
{
	if(!s)
		s = "";
	if(!size)
		size = strlen(s);
	str = create(size);
	str->retain();
	str->set(s);
}

string::string(strsize_t size) 
{
	str = create(size);
	str->retain();
};

string::string(strsize_t size, char fill)
{
    str = create(size, fill);
    str->retain();
};

string::string(strsize_t size, const char *format, ...)
{
	assert(format != NULL);

	va_list args;
	va_start(args, format);

	str = create(size);
	str->retain();
	vsnprintf(str->text, size + 1, format, args);
	va_end(args);
}

string::string(const string &dup)
{
	str = dup.c_copy();
	if(str)
		str->retain();
}

string::~string()
{
	string::release();
}

string::cstring *string::c_copy(void) const
{
	return str;
}

string string::get(strsize_t offset, strsize_t len) const
{
	if(!str || offset >= str->len)
		return string("");

	if(!len)
		len = str->len - offset;
	return string(len, str->text + offset);
}

string::cstring *string::create(strsize_t size, char fill) const
{
	if(fill)
		return new((size_t)size) cstring(size, fill);
	else
		return new((size_t)size) cstring(size);
}

void string::retain(void)
{
	if(str)
		str->retain();
}

void string::release(void)
{
	if(str)
		str->release();
	str = NULL;
}

char *string::c_mem(void) const
{
	if(!str)
		return NULL;

	return str->text;
}

const char *string::c_str(void) const
{
	if(!str)
		return "";

	return str->text;
}

int string::compare(const char *s) const
{
	const char *mystr = "";

	if(str)
		mystr = str->text;

	if(!s)
		s = "";

	return strcmp(mystr, s);
}

const char *string::last(const char *clist) const
{
	if(!str)
		return NULL;

	return last(str->text, clist);
}

const char *string::first(const char *clist) const
{
	if(!str)
		return NULL;

	return first(str->text, clist);
}

const char *string::begin(void) const
{
	if(!str)
		return NULL;

	return str->text;
}

const char *string::end(void) const
{
	if(!str)
		return NULL;

	return str->text + str->len;
}

const char *string::chr(char ch) const
{
	assert(ch != 0);

	if(!str)
		return NULL;

	return ::strchr(str->text, ch);
}

const char *string::rchr(char ch) const
{
	assert(ch != 0);

    if(!str)
        return NULL;

    return ::strrchr(str->text, ch);
}

const char *string::skip(const char *clist, strsize_t offset) const
{
	if(!str || !clist || !*clist || !str->len || offset > str->len)
		return NULL;

	while(offset < str->len) {
		if(!strchr(clist, str->text[offset]))
			return str->text + offset;
		++offset;
	}
	return NULL;
}

const char *string::rskip(const char *clist, strsize_t offset) const
{
	if(!str || !clist || !*clist)
		return NULL;

	if(!str->len)
		return NULL;

	if(offset > str->len)
		offset = str->len;

	while(offset--) {
		if(!strchr(clist, str->text[offset]))
			return str->text + offset;
	}
	return NULL;
}

const char *string::rfind(const char *clist, strsize_t offset) const
{
	if(!str || !clist || !*clist)
		return NULL;

	if(!str->len)
		return str->text;

	if(offset > str->len)
		offset = str->len;

	while(offset--) {
		if(strchr(clist, str->text[offset]))
			return str->text + offset;
	}
	return NULL;
}

void string::chop(const char *clist)
{
	strsize_t offset;

	if(!str)
		return;

	if(!str->len)
		return;

	offset = str->len;
	while(offset) {
		if(!strchr(clist, str->text[offset - 1]))
			break;
		--offset;
	}

	if(!offset) {
		clear();
		return;
	}

	if(offset == str->len)
		return;

	str->len = offset;
	str->fix();
}

void string::strip(const char *clist)
{
	trim(clist);
	chop(clist);
}

void string::trim(const char *clist)
{
	unsigned offset = 0;

	if(!str)
		return;

	while(offset < str->len) {
		if(!strchr(clist, str->text[offset]))
			break;
		++offset;
	}

	if(!offset)
		return;

	if(offset == str->len) {
		clear();
		return;
	}

	memmove(str->text, str->text + offset, str->len - offset);
	str->len -= offset;
	str->fix();
}


const char *string::find(const char *clist, strsize_t offset) const
{
	if(!str || !clist || !*clist || !str->len || offset > str->len)
		return NULL;

	while(offset < str->len) {
		if(strchr(clist, str->text[offset]))
			return str->text + offset;
		++offset;
	}
	return NULL;
}

bool string::unquote(const char *clist)
{
	assert(clist != NULL);

	char *s;

	if(!str)
		return false;

	str->unfix();
	s = unquote(str->text, clist);
	if(!s) {
		str->fix();
		return false;
	}

	set(s);
	return true;
}

void string::upper(void)
{
	if(str)
		upper(str->text);
}

void string::lower(void)
{
	if(str)
		lower(str->text);
}

strsize_t string::offset(const char *s) const
{
	if(!str || !s)
		return npos;

	if(s < str->text || s > str->text + str->max)
		return npos;

	if(s - str->text > str->len)
		return str->len;

	return (strsize_t)(s - str->text);
}

strsize_t string::count(void) const
{
	if(!str)
		return 0;
	return str->len;
}

strsize_t string::ccount(const char *clist) const
{
	if(!str)
		return 0;

	return ccount(str->text, clist);
}

strsize_t string::printf(const char *format, ...)
{
	assert(format != NULL);

	va_list args;
	va_start(args, format);
	if(str) {
		vsnprintf(str->text, str->max + 1, format, args);
		str->len = strlen(str->text);
		str->fix();
	}
	va_end(args);
	return len();
}

strsize_t string::vprintf(const char *format, va_list args)
{
	assert(format != NULL);

	if(str) {
		vsnprintf(str->text, str->max + 1, format, args);
		str->len = strlen(str->text);
		str->fix();
	}
	return len();
}

int string::vscanf(const char *format, va_list args)
{
	assert(format != NULL);

	if(str)
		return vsscanf(str->text, format, args);
	return -1;
}

int string::scanf(const char *format, ...)
{
	assert(format != NULL);

	va_list args;
	int rtn = -1;

	va_start(args, format);
	if(str)
		rtn = vsscanf(str->text, format, args);
	va_end(args);
	return rtn;
}

void string::rsplit(const char *s)
{
	if(!str || !s || s <= str->text || s > str->text + str->len)
		return;

	str->set(s);
}

void string::rsplit(strsize_t pos)
{
	if(!str || pos > str->len || !pos)
		return;

	str->set(str->text + pos);
}

void string::split(const char *s)
{
	unsigned pos;

	if(!s || !*s || !str)
		return;

	if(s < str->text || s >= str->text + str->len)
		return;

	pos = s - str->text;
	str->text[pos] = 0;
	str->fix();
}

void string::split(strsize_t pos)
{
	if(!str || pos >= str->len)
		return;

	str->text[pos] = 0;
	str->fix();
}

void string::set(strsize_t offset, const char *s, strsize_t size)
{
	if(!s || !*s || !str)
		return;

	if(!size)
		size = strlen(s);

	str->set(offset, s, size);
}

void string::set(const char *s, char overflow, strsize_t offset, strsize_t size)
{
	size_t len = count(s);

	if(!s || !*s || !str)
		return;
	
	if(offset >= str->max)
		return;

	if(!size || size > str->max - offset)
		size = str->max - offset;

	if(len <= size) {
		set(offset, s, size);
		return;
	}

	set(offset, s, size);

	if(len > size && overflow)
		str->text[offset + size - 1] = overflow;
}

void string::rset(const char *s, char overflow, strsize_t offset, strsize_t size)
{
	size_t len = count(s);
	strsize_t dif;

	if(!s || !*s || !str)
		return;
	
	if(offset >= str->max)
		return;

	if(!size || size > str->max - offset)
		size = str->max - offset;

	dif = len;
	while(dif < size && str->fill) {
		str->text[offset++] = str->fill;
		++dif;
	}

	if(len > size)
		s += len - size;
	set(offset, s, size);
	if(overflow && len > size)
		str->text[offset] = overflow;
}

void string::set(const char *s)
{
	strsize_t len;

	if(!s)
		s = "";

	if(!str) {
		len = strlen(s);
		str = create(len);
		str->retain();
	}

	str->set(s);
}

void string::cut(strsize_t offset, strsize_t size)
{
	if(!str || offset >= str->len)
		return;

	if(!size)
		size = str->len;

	if(offset + size >= str->len) {
		str->len = offset;
		str->fix();
		return;
	}

	memmove(str->text + offset, str->text + offset + size, str->len - offset - size);
	str->len -= size;
	str->fix();
}

bool string::resize(strsize_t size)
{
	char fill;

	if(!size) {
		release();
		str = NULL;
		return true;
	}

	if(str->isCopied() || str->max < size) {
		fill = str->fill;
		str->release();
		str = create(size, fill);
		str->retain();
	}
	return true;
}

void string::clear(strsize_t offset, strsize_t size)
{
	if(!str)
		return;

	if(!size)
		size = str->max;

	str->clear(offset, size);
}

void string::clear(void)
{
	if(str)
		str->set("");
}

void string::cow(strsize_t size)
{
	if(str) {
		if(str->fill)
			size = str->max;
		else
			size += str->len;
	}

	if(!size)
		return;

	if(!str || !str->max || str->isCopied() || size > str->max) {
		cstring *s = create(size);
		s->len = str->len;
		string::set(s->text, s->max + 1, str->text);
		s->retain();
		str->release();
		str = s;
	}
}

void string::add(const char *s)
{
	if(!s || !*s)
		return;

	if(!str) {
		set(s);
		return;
	}

	str->add(s);
}

char string::at(int offset) const
{
	if(!str)
		return 0;

	if(offset >= (int)str->len)
		return 0;

	if(offset > -1)
		return str->text[offset];

	if((strsize_t)(-offset) >= str->len)
		return str->text[0];

	return str->text[(int)(str->len) + offset];
}

string string::operator()(int offset, strsize_t len) const
{
	const char *cp = operator()(offset);
	if(!cp)
		cp = "";

	if(!len)
		len = strlen(cp);

	return string(cp, len);
}

const char *string::operator()(int offset) const
{
	if(!str)
		return NULL;

	if(offset >= (int)str->len)
		return NULL;

	if(offset > -1)
		return str->text + offset;

	if((strsize_t)(-offset) >= str->len)
		return str->text;

	return str->text + str->len + offset;
}

const char string::operator[](int offset) const
{
	if(!str)
		return 0;

	if(offset >= (int)str->len)
		return 0;

	if(offset > -1)
		return str->text[offset];

	if((strsize_t)(-offset) >= str->len)
		return *str->text;

	return str->text[str->len + offset];
}

string &string::operator++()
{
	if(str)
		str->inc(1);
	return *this;
}

string &string::operator--()
{
    if(str)
        str->dec(1);
    return *this;
}

string &string::operator-=(strsize_t offset)
{
    if(str)
        str->dec(offset);
    return *this;
}

string &string::operator+=(strsize_t offset)
{
    if(str)
        str->inc(offset);
    return *this;
}

string &string::operator^=(const char *s)
{
	release();
	set(s);
	return *this;
}

string &string::operator=(const char *s)
{
	set(s);
	return *this;
}

string &string::operator^=(const string &s)
{
	release();
	set(s.c_str());
	return *this;
}	

string &string::operator=(const string &s)
{
	if(str == s.str)
		return *this;

	if(s.str)
		s.str->retain();
	
	if(str)
		str->release();

	str = s.str;
	return *this;
}

bool string::full(void) const
{
	if(!str)
		return false;

	if(str->len == str->max && 
	   str->text[str->len - 1] != str->fill)
		return true;
	
	return false;
}

bool string::operator!() const
{
	bool rtn = false;
	if(!str)
		return true;

	str->unfix();
	if(!str->len)
		rtn = true;

	str->fix();
	return rtn;
}

string::operator bool() const
{
	bool rtn = false;

	if(!str)
		return false;

	str->unfix();
	if(str->len)
		rtn = true;
	str->fix();
	return rtn;
}

bool string::operator==(const char *s) const
{
	return (compare(s) == 0);
}

bool string::operator!=(const char *s) const
{
    return (compare(s) != 0);
}

bool string::operator<(const char *s) const
{
	return (compare(s) < 0);
}

bool string::operator<=(const char *s) const
{
    return (compare(s) <= 0);
}

bool string::operator>(const char *s) const
{
    return (compare(s) > 0);
}

bool string::operator>=(const char *s) const
{
    return (compare(s) >= 0);
}

string &string::operator&(const char *s)
{
	add(s);
	return *this;
}

string &string::operator+(const char *s)
{
	if(!s || !*s)
		return *this;

	cow(strlen(s));
	add(s);
	return *this;
}

memstring::memstring(void *mem, strsize_t size, char fill)
{
	assert(mem != NULL);
	assert(size > 0);

	str = new((caddr_t)mem) cstring(size, fill);
	str->set("");
}

memstring::~memstring()
{
	str = NULL;
}

memstring *memstring::create(strsize_t size, char fill)
{
	assert(size > 0);

	caddr_t mem = (caddr_t)cpr_memalloc(size + sizeof(memstring) + sizeof(cstring));
	return new(mem) memstring(mem + sizeof(memstring), size, fill);
}

memstring *memstring::create(mempager *pager, strsize_t size, char fill)
{
	assert(size > 0);

	caddr_t mem = (caddr_t)pager->alloc(size + sizeof(memstring) + sizeof(cstring));
	return new(mem) memstring(mem + sizeof(memstring), size, fill);
}

void memstring::release(void)
{
	str = NULL;
}

bool memstring::resize(strsize_t size)
{
	return false;
}

void memstring::cow(strsize_t adj)
{
}

const char *string::token(char *text, char **token, const char *clist, const char *quote, const char *eol)
{
	char *result;

	if(!eol)
		eol = "";

	if(!token || !clist)
		return NULL;

	if(!*token)
		*token = text;
	
	if(!**token) {
		*token = text;
		return NULL;
	}

	while(**token && strchr(clist, **token))
		++token;

	result = *token;

	if(*result && *eol && NULL != (eol = strchr(eol, *result))) {
		if(eol[0] != eol[1] || *result == eol[1]) {
			*token = text;
			return NULL;
		}
	}

	if(!*result) {
		*token = text;
		return NULL;
	}

	while(quote && *quote && *result != *quote) {
		quote += 2;
	}

	if(quote && *quote) {
		++result;
		++quote;
		*token = strchr(result, *quote);
		if(!*token)
			*token = result + strlen(result);
		else {
			**token = 0;
			++(*token);
		}
		return result;
	}

	while(**token && !strchr(clist, **token))
		++(*token);

	if(**token) {
		**token = 0;
		++(*token);
	}

	return result;
}

string::cstring *memstring::c_copy(void) const
{
	cstring *tmp = string::create(str->max, str->fill);
	tmp->set(str->text);
	return tmp;
}

void string::fix(string &s)
{
	if(s.str) {
		s.str->len = strlen(s.str->text);
		s.str->fix();
	}
}

int string::scanf(string &s, const char *fmt, ...)
{
	assert(fmt != NULL);

	int rtn;
	va_list args;

	va_start(args, fmt);
	rtn = s.vscanf(fmt, args);
	va_end(args);
	return rtn;
}
	
strsize_t string::printf(string &s, const char *fmt, ...)
{
	assert(fmt != NULL);

	va_list args;
	va_start(args, fmt);
	s.vprintf(fmt, args);
	va_end(args);
	return len(s);
}

void string::swap(string &s1, string &s2)
{
	string::cstring *s = s1.str;
	s1.str = s2.str;
	s2.str = s;
}

int string::read(Socket &so, string &s)
{
	int rtn;

	if(!mem(s))
		return 0;

	clear(s);

	rtn = so.get(mem(s), size(s));
	if(rtn > -1) {
		s.str->len = rtn;
		s.str->text[rtn] = 0;
	}
	return rtn;
}

int string::write(Socket &so, string &s)
{
	if(!mem(s))
		return 0;

	return so.put(mem(s), len(s));
}

int string::read(FILE *fp, string &s)
{
	int rtn;

	if(!mem(s))
		return 0;

	clear(s);

	rtn = fread(mem(s), 1, size(s), fp);
	if(rtn > -1) {
		s.str->len = rtn;
		s.str->text[rtn] = 0;
	}
	return rtn;
}

int string::write(FILE *fp, string &s)
{
	assert(fp != NULL);

	if(!mem(s))
		return 0;

	return fwrite(mem(s), 1, len(s), fp);
}

bool string::putline(Socket &so, string &s)
{
	if(s[-1] != '\n')
		s.add("\r\n");
		
	if(so.puts(mem(s)) >= 0)
		return true;

	return false;
}

bool string::putline(FILE *fp, string &s)
{
	assert(fp != NULL);

	if(s[-1] != '\n')
		s.add("\n");
		
	if(fputs(mem(s), fp) >= 0)
		return true;

	return false;
}

bool string::getline(Socket &so, string &s)
{
	if(!mem(s))
		return true;

	if(so.gets(mem(s), size(s)) < 0) 
		return false;

	fix(s);

	if(s[-1] == '\n')
		--s;

	if(s[-1] == '\r')
		--s;

	return true;
}

bool string::getline(FILE *fp, string &s)
{
	assert(fp != NULL);

	if(!mem(s))
		return true;

	if(!::fgets(mem(s), size(s), fp) || feof(fp)) {
		clear(s);
		return false;
	}
	fix(s);
	if(s[-1] == '\n')
		--s;

	if(s[-1] == '\r')
		--s;

	return true;
}
	
char *string::dup(const char *cp)
{
	char *mem;

	if(!cp)
		return NULL;

	size_t len = strlen(cp) + 1;
	mem = (char *)malloc(len);
	crit(mem != NULL, "string dup allocation error");
	string::set(mem, len, cp);
	return mem;
}	

size_t string::count(const char *cp)
{
	if(!cp)
		return 0;

	return strlen(cp);
}

const char *string::find(const char *str, const char *key, const char *delim)
{
	unsigned l1 = strlen(str);
	unsigned l2 = strlen(key);

	if(!delim[0])
		delim = NULL;

	while(l1 >= l2) {
		if(!strncmp(key, str, l2)) {
			if(l1 == l2 || !delim || strchr(delim, str[l2]))
				return str;
		}
		if(!delim) {
			++str;
			--l1;
			continue;
		}
		while(l1 >= l2 && !strchr(delim, *str)) {
			++str;
			--l1;
		}
		while(l1 >= l2 && strchr(delim, *str)) {
			++str;
			--l1;
		} 
	}
	return NULL;
}

const char *string::ifind(const char *str, const char *key, const char *delim)
{
	unsigned l1 = strlen(str);
	unsigned l2 = strlen(key);

	if(!delim[0])
		delim = NULL;

	while(l1 >= l2) {
		if(!strnicmp(key, str, l2)) {
			if(l1 == l2 || !delim || strchr(delim, str[l2]))
				return str;
		}
		if(!delim) {
			++str;
			--l1;
			continue;
		}
		while(l1 >= l2 && !strchr(delim, *str)) {
			++str;
			--l1;
		}
		while(l1 >= l2 && strchr(delim, *str)) {
			++str;
			--l1;
		} 
	}
	return NULL;
}

char *string::set(char *str, size_t size, const char *s, size_t len)
{
	if(!str)
		return NULL;

	if(size < 2)
		return str;

	if(!s)
		s = "";

	size_t l = strlen(s);
	if(l >= size)
		l = size - 1;

	if(l > len)
		l = len;

	if(!l) {
		*str = 0;
		return str;
	}

	memmove(str, s, l);
	str[l] = 0;
	return str;
}

char *string::rset(char *str, size_t size, const char *s)
{
	size_t len = count(s);
	if(len > size) 
		s += len - size;
	return set(str, size, s);
}

char *string::set(char *str, size_t size, const char *s)
{
	if(!str)
		return NULL;

	if(size < 2)
		return str;

	if(!s)
		s = "";

	size_t l = strlen(s);

	if(l >= size)
		l = size - 1;

	if(!l) {
		*str = 0;
		return str;
	}

	memmove(str, s, l);
	str[l] = 0;
	return str;
}

char *string::add(char *str, size_t size, const char *s, size_t len)
{
    if(!str)
        return NULL;

    if(!s)
        return str;

    size_t l = strlen(s);
    size_t o = strlen(str);

	if(o >= (size - 1))
		return str;
	set(str + o, size - o, s, l);
	return str;
}

char *string::add(char *str, size_t size, const char *s)
{
	if(!str)
		return NULL;

	if(!s)
		return str;

	size_t o = strlen(str);

	if(o >= (size - 1))
		return str;

	set(str + o, size - o, s);
	return str;
}

char *string::trim(char *str, const char *clist)
{
	if(!str)
		return NULL;

	if(!clist)
		return str;

	while(*str && strchr(clist, *str))
		++str;

	return str;
}

char *string::chop(char *str, const char *clist)
{
	if(!str)
		return NULL;

	if(!clist)
		return str;

	size_t offset = strlen(str);
	while(offset && strchr(clist, str[offset - 1]))
		str[--offset] = 0;
	return str;
}

char *string::strip(char *str, const char *clist)
{
	str = trim(str, clist);
	chop(str, clist);
	return str;
}

void string::upper(char *str)
{
	while(str && *str) {
		*str = toupper(*str);
		++str;
	}
}

void string::lower(char *str)
{
    while(str && *str) {
        *str = tolower(*str);
        ++str;
    }
}

unsigned string::ccount(const char *str, const char *clist)
{
	unsigned count = 0;
	while(str && *str) {
		if(strchr(clist, *(str++)))
			++count;
	}
	return count;
}

char *string::skip(char *str, const char *clist)
{
	if(!str || !clist)
		return NULL;

	while(*str && strchr(clist, *str))
		++str;

	if(*str)
		return str;

	return NULL;
}

char *string::rskip(char *str, const char *clist)
{
	size_t len = count(str);

	if(!len || !clist)
		return NULL;

	while(len > 0) {
		if(!strchr(clist, str[--len]))
			return str;
	}
	return NULL;
}

char *string::find(char *str, const char *clist)
{
	if(!str)
		return NULL;

	if(!clist)
		return str;

	while(str && *str) {
		if(strchr(clist, *str))
			return str;
	}
	return NULL;
}

char *string::rfind(char *str, const char *clist)
{
	if(!str)
		return NULL;

	if(!clist)
		return str + strlen(str);

	char *s = str + strlen(str);

	while(s > str) {
		if(strchr(clist, *(--s)))
			return s;
	}
	return NULL;
}

char *string::last(char *str, const char *clist)
{
	char *cp, *lp = NULL;

	if(!str)
		return NULL;

	if(!clist)
		return str + strlen(str) - 1;

	while(clist && *clist) {
		cp = strrchr(str, *(clist++));
		if(cp && cp > lp)
			lp = cp;
	}

	return lp;
}

char *string::first(char *str, const char *clist)
{
    char *cp, *fp;

    if(!str)
        return NULL;

	if(!clist)
		return str;

	fp = str + strlen(str);
    while(clist && *clist) {
        cp = strchr(str, *(clist++));
        if(cp && cp < fp)
            fp = cp;
    }

	if(!*fp)
		fp = NULL;
    return fp;
}

int string::compare(const char *s1, const char *s2)
{
	if(!s1)
		s1 = "";
	if(!s2)
		s2 = "";

	return strcmp(s1, s2);
}

int string::compare(const char *s1, const char *s2, size_t s)
{
    if(!s1)
        s1 = "";
    if(!s2)
        s2 = "";

    return strncmp(s1, s2, s);
}

int string::icompare(const char *s1, const char *s2)
{
	if(!s1)
		s1 = "";

	if(!s2)
		s2 = "";

#ifdef	HAVE_STRICMP
	return stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif
}

int string::icompare(const char *s1, const char *s2, size_t s)
{
    if(!s1)
        s1 = "";

    if(!s2)
        s2 = "";

#ifdef  HAVE_STRICMP
    return strnicmp(s1, s2, s);
#else
    return strncasecmp(s1, s2, s);
#endif
}

char *string::unquote(char *str, const char *clist)
{
	assert(clist != NULL);

	size_t len = count(str);
	if(!len || !str)
		return NULL;

	while(clist[0]) {
		if(*str == clist[0] && str[len - 1] == clist[1]) {
			str[len - 1] = 0;
			return ++str;
		}
		clist += 2;
	}
	return str;
}

char *string::fill(char *str, size_t size, char fill)
{
	if(!str)
		return NULL;

	memset(str, fill, size - 1);
	str[size - 1] = 0;
	return str;
}



