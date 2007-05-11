#include <config.h>
#include <ucommon/string.h>
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
	if(!str)
		return NULL;

	return ::strchr(str->text, ch);
}

const char *string::rchr(char ch) const
{
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

void string::printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if(str) {
		vsnprintf(str->text, str->max + 1, format, args);
		str->len = strlen(str->text);
		str->fix();
	}
	va_end(args);
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
		strcpy(s->text, str->text);
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

string &string::operator<<(::DIR *dp)
{
	dirent *d;

	if(!str || !dp)
		return *this;

	d = readdir(dp);
	if(d)
		set(d->d_name);
	else
		set("");

	return *this;
}	

string &string::operator<<(::FILE *fp)
{
	if(!str || !fp)
		return *this;

	::fgets(str->text, str->max, fp);
	str->len = strlen(str->text);
	if(!str->fill)
		return *this;

	if(str->len && str->text[str->len - 1] == '\n')
		--str->len;

	if(str->len && str->text[str->len - 1] == '\r')
		--str->len;
	str->fix();
	return *this;
}

string &string::operator>>(::FILE *fp)
{
	if(!str || !fp)
		return *this;

	fputs(str->text, fp);
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
	str = new((caddr_t)mem) cstring(size, fill);
	str->set("");
}

memstring::~memstring()
{
	str = NULL;
}

memstring *memstring::create(strsize_t size, char fill)
{
	caddr_t mem = (caddr_t)cpr_memalloc(size + sizeof(memstring) + sizeof(cstring));
	return new(mem) memstring(mem + sizeof(memstring), size, fill);
}

memstring *memstring::create(mempager *pager, strsize_t size, char fill)
{
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

string::cstring *memstring::c_copy(void) const
{
	cstring *tmp = string::create(str->max, str->fill);
	tmp->set(str->text);
	return tmp;
}
	
tokenstring::tokenstring(const char *cl) :
string()
{
	clist = cl;
	token = NULL;
}

tokenstring::tokenstring(caddr_t mem, const char *cl) :
string()
{
	clist = cl;
	token = trim(mem, cl);
}

const char *tokenstring::get(void)
{
	const char *mem;

	if(!token && !str)
		return NULL;

	if(!token) {
		cow();
		chop(clist);
		token = str->text;
	}
	else if(!*token)
		return NULL;

	while(*token && strchr(clist, *token))
		++token;

	mem = token;	
	while(*token && !strchr(clist, *token))
		++token;

	if(*token)
		*(token++) = 0;

	return mem;
}

const char *tokenstring::get(const char *q)
{
	char *mem;

	if(!token && !str)
		return NULL;

	if(!token) {
		cow();
		chop(clist);
		token = str->text;
	}
	else if(!*token)
		return NULL;

	while(*token && strchr(clist, *token))
		++token;

	mem = token;	
	
	if(strchr(q, *mem)) {
		++token;
		switch(*mem) {
		case '{':
			*mem = '}';
			break;
		case '(':
			*mem = ')';
			break;
		case '[':
			*mem = ']';
			break;
		case '<':
			*mem = '>';
			break;
		}
		while(*token && *token != *mem)
			++token;
		++mem;
	}
	else while(*token && !strchr(clist, *token))
		++token;

	if(*token)
		*(token++) = 0;

	return mem;
}


const char *tokenstring::next(void)
{
	const char *p = token;

	if(!p && str) {
		cow();
		chop(clist);
		p = str->text;
	}
	else if(!p)
		return NULL;

	while(*p && strchr(clist, *p))
		++p;

	return p;
}

tokenstring &tokenstring::operator=(caddr_t s)
{
	token = NULL;
	if(str)
		set(s);
	else
		token = trim(s, clist);
	return *this;
}

tokenstring &tokenstring::operator=(const string &s)
{
	set(s.c_str());
	token = NULL;
	return *this;
}

void string::swap(string &s1, string &s2)
{
	string::cstring *s = s1.str;
	s1.str = s2.str;
	s2.str = s;
}

char *string::dup(const char *cp)
{
	char *mem;

	if(!cp)
		return NULL;

	mem = (char *)malloc(strlen(cp) + 1);
	crit(mem != NULL);
	strcpy(mem, cp);
	return mem;
}	

size_t string::count(const char *cp)
{
	if(!cp)
		return 0;

	return strlen(cp);
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
	while(offset && strchr(clist, str[offset - 1])) {
		*(--str) = 0;
		--offset;
	}
	return str;
}

char *string::strip(char *str, const char *clist)
{
	str = trim(str, clist);
	return chop(str, clist);
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

timeout_t string::totimeout(const char *cp, char **ep, bool sec)
{
	char *end, *nend;
	timeout_t base = strtol(cp, &end, 10);
	timeout_t rem = 0;
	unsigned dec = 0;

	if(!cp)
		return 0;

	if(ep)
		*ep = NULL;

	if(!end)
		return base;

	while(end && *end == ':') {
		cp = ++end;
		base *= 60;
		base += strtol(cp, &end, 10);
		sec = true;
	}

	if(end && *end == '.') {
		sec = true;
		rem = strtol(end, &nend, 10);
		while(rem > 1000l)
			rem /= 10;
		while(++dec < 4) {
			rem *= 10;
		}
		end = nend;
	}
	
	switch(*end)
	{
	case 'm':
	case 'M':
		++end;
		if(*end == 's' || *end == 'S') {
			++end;
			break;
		}
		base *= 60000l;
		if(dec)
			base += (rem * 60l);
		break;
	case 's':
	case 'S':
		base *= 1000l;
		if(dec)
			base += rem;
		break;
	case 'h':
		base *= 3600000l;
		if(dec)
			base += (rem * 3600l);
		break;
	default:
		if(sec)
			base *= 1000l;
		if(sec && dec)
			base += rem;
	}
		
	if(ep)
		*ep = end;
	
	return base;
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

int string::case_compare(const char *s1, const char *s2)
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

int string::case_compare(const char *s1, const char *s2, size_t s)
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

int32_t string::toint(const char *cp, char **ep)
{
	if(!cp) {
		if(ep)
			*ep = NULL;
		return 0;
	}
	long value = strtol(cp, ep, 10);
	return (int32_t)value;
}	

char *string::unquote(char *str, const char *clist)
{
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
	return NULL;
}

char *string::fill(char *str, size_t size, const char fill)
{
	if(!str)
		return NULL;

	memset(str, fill, size - 1);
	str[size - 1] = 0;
	return str;
}

bool string::tobool(const char *cp, char **ep)
{
	bool rtn = false;

	if(!cp) {
		if(ep)
			*ep = NULL;
		return false;
	}

	if(*cp == '1') {
		rtn = true;
		++cp;
		goto exit;
	}
	else if(*cp == '0') {
		++cp;
		goto exit;
	}

	if(!strnicmp(cp, "true", 4)) {
		cp += 4;
		rtn = true;
		goto exit;
	}
	else if(!strnicmp(cp, "yes", 3)) {
		cp += 3;
		rtn = true;
		goto exit;
	}
	else if(!strnicmp(cp, "false", 5)) {
		cp += 5;
		goto exit;
	}
	else if(!strnicmp(cp, "no", 2)) {
		cp += 2;
		goto exit;
	}

	if(strchr("yYtT", *cp)) {
		++cp;
		rtn = true;
		goto exit;
	}

	if(strchr("nNfF", *cp)) {
		++cp;
		goto exit;
	}

exit:
	if(ep)
		*ep = (char *)cp;

	return rtn;
}

bool string::isinteger(const char *cp)
{
	if(!cp)
		return false;

	while(*cp && isdigit(*cp))
		++cp;

	if(*cp)
		return false;

	return true;
}

bool string::isnumeric(const char *cp)
{
	if(!cp)
		return false;

	while(*cp && strchr("0123456789.+-e", *cp))
		++cp;

	if(*cp)
		return false;

	return true;
}

size_t string::urlencodesize(char *src)
{
	size_t size = 0;

	while(src && *src) {
		if(isalnum(*src) ||  strchr("/.-:;, ", *src))
			++size;
		else
			size += 3;
	}
	return size;
}

size_t string::urlencode(char *dest, size_t limit, const char *src)
{
	static const char *hex = "0123456789abcdef";
	char *ret = dest;
	unsigned char ch;

	assert(dest != NULL && src != NULL && limit > 3);
	
	while(limit-- > 3 && *src) {
		if(*src == ' ') {
			*(dest++) = '+';
			++src;
		}
		else if(isalnum(*src) ||  strchr("/.-:;,", *src))
			*(dest++) = *(src++);
		else {		
			limit -= 2;
			ch = (unsigned char)(*(src++));
			*(dest++) = '%';
			*(dest++) = hex[(ch >> 4)&0xF];
			*(dest++) = hex[ch % 16];
		}
	}		
	*dest = 0;
	return dest - ret;
}

size_t string::urldecode(char *dest, size_t limit, const char *src)
{
	char *ret = dest;
	char hex[3];

	assert(dest != NULL && src != NULL && limit > 1);

	while(limit-- > 1 && *src && !strchr("?&", *src)) {
		if(*src == '%') {
			memset(hex, 0, 3);
			hex[0] = *(++src);
			if(*src)
				hex[1] = *(++src);
			if(*src)
				++src;			
			*(dest++) = (char)strtol(hex, NULL, 16);	
		}
		else if(*src == '+') {
			*(dest++) = ' ';
			++src;
		}
		else
			*(dest++) = *(src++);
	}
	*dest = 0;
	return dest - ret;
}

size_t string::xmlencode(char *out, size_t limit, const char *src)
{
	char *ret = out;

	assert(src != NULL && out != NULL && limit > 0);

	while(src && *src && limit > 6) {
		switch(*src) {
		case '<':
			strcpy(out, "&lt;");
			limit -= 4;
			out += 4;
			break;
		case '>':
			strcpy(out, "&gt;");
			limit -= 4;
			out += 4;
			break;
		case '&':
			strcpy(out, "&amp;");
			limit -= 5;
			out += 5;
			break;
		case '\"':
			strcpy(out, "&quot;");
			limit -= 6;
			out += 6;
			break;
		case '\'':
			strcpy(out, "&apos;");
			limit -= 6;
			out += 6;
			break;
		default:
			--limit;
			*(out++) = *src;
		}
		++src;
	}
	*out = 0;
	return out - ret;
}

size_t string::xmldecode(char *out, size_t limit, const char *src)
{
	char *ret = out;

	assert(src != NULL && out != NULL && limit > 0);

	if(*src == '\'' || *src == '\"')
		++src;
	while(src && limit-- > 1 && !strchr("<\'\">", *src)) {
		if(!strncmp(src, "&amp;", 5)) {
			*(out++) = '&';
			src += 5;
		}
		else if(!strncmp(src, "&lt;", 4)) {
			src += 4;
			*(out++) = '<';
		}
		else if(!strncmp(src, "&gt;", 4)) {
			src += 4;
			*(out++) = '>';
		}
		else if(!strncmp(src, "&quot;", 6)) {
			src += 6;
			*(out++) = '\"';
		}
		else if(!strncmp(src, "&apos;", 6)) {
			src += 6;
			*(out++) = '\'';
		}
		else
			*(out++) = *(src++);
	}
	*out = 0;
	return out - ret;
}

static const unsigned char alphabet[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t string::b64decode(caddr_t out, const char *src, size_t count)
{
	static char *decoder = NULL;
	int i, bits, c;

	unsigned char *dest = (unsigned char *)out;

	if(!decoder) {
		decoder = (char *)malloc(256);
		for (i = 0; i < 256; ++i)
			decoder[i] = 64;
		for (i = 0; i < 64 ; ++i)
			decoder[alphabet[i]] = i;
	}

	bits = 1;

	while(*src) {
		c = (unsigned char)(*(src++));
		if (c == '=') {
			if (bits & 0x40000) {
				if (count < 2) break;
				*(dest++) = (bits >> 10);
				*(dest++) = (bits >> 2) & 0xff;
				break;
			}
			if (bits & 0x1000 && count)
				*(dest++) = (bits >> 4);
			break;
		}
		// skip invalid chars
		if (decoder[c] == 64)
			continue;
		bits = (bits << 6) + decoder[c];
		if (bits & 0x1000000) {
			if (count < 3) break;
			*(dest++) = (bits >> 16);
			*(dest++) = (bits >> 8) & 0xff;
			*(dest++) = (bits & 0xff);
		    	bits = 1;
			count -= 3;
		}
	}
	return (size_t)((caddr_t)dest - out);
}

size_t string::b64encode(char *out, caddr_t src, size_t count)
{
	unsigned bits;
	char *dest = out;

	assert(out != NULL && src != NULL && count > 0);

	while(count >= 3) {
		bits = (((unsigned)src[0])<<16) | (((unsigned)src[1])<<8)
			| ((unsigned)src[2]);
		src += 3;
		count -= 3;
		*(out++) = alphabet[bits >> 18];
	    *(out++) = alphabet[(bits >> 12) & 0x3f];
	    *(out++) = alphabet[(bits >> 6) & 0x3f];
	    *(out++) = alphabet[bits & 0x3f];
	}
	if (count) {
		bits = ((unsigned)src[0])<<16;
		*(out++) = alphabet[bits >> 18];
		if (count == 1) {
			*(out++) = alphabet[(bits >> 12) & 0x3f];
	    		*(out++) = '=';
		}
		else {
			bits |= ((unsigned)src[1])<<8;
			*(out++) = alphabet[(bits >> 12) & 0x3f];
	    		*(out++) = alphabet[(bits >> 6) & 0x3f];
		}
	    	*(out++) = '=';
	}
	*out = 0;
	return out - dest;
}

size_t string::b64len(const char *str)
{
	unsigned count = strlen(str);
	const char *ep = str + count - 1;

	if(!count)
		return 0;

	count /= 4;
	count *= 3;
	if(*ep == '=') {
		--ep;
		--count;
		if(*ep == '=')
			--count;
	}
	return count;
}

string string::uuid(void)
{
	string str(38);
	char uuid[39];
	int fd = open("/dev/urandom", O_RDONLY);
	char buf[16];
	if(fd < 0)
		return str;

	if(read(fd, buf, 16) < 16)
		return -1;

	str.printf(uuid, sizeof(uuid), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		buf[0], buf[1], buf[2], buf[3],
        buf[4], buf[5], buf[6], buf[7],
        buf[8], buf[9], buf[10], buf[11],
        buf[12], buf[13], buf[14], buf[15]);

	close(fd);
	str = uuid;
	return str;
}

struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
};

static void MD5Transform(uint32_t buf[4], uint32_t const in[16]);

#if(__BYTE_ORDER == __LITTLE_ENDIAN)
#define byteReverse(buf, len)	/* Nothing */
#else
static void byteReverse(unsigned char *buf, unsigned longs)
{
	uint32_t t;
	do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	    ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
	} while (--longs);
}
#endif

static void MD5Init(struct MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

static void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len)
{
	uint32_t t;

	/* Update bitcount */

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++;		/* Carry from low to high */
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* Handle any leading odd-sized chunks */

	if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += t;
	len -= t;
	}
	/* Process data in 64-byte chunks */

	while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += 64;
	len -= 64;
	}

	/* Handle any remaining bytes of data. */

	memcpy(ctx->in, buf, len);
}

static void MD5Final(unsigned char digest[16], struct MD5Context *ctx)
{
	unsigned count;
	unsigned char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/* Set the first char of padding to 0x80.  This is safe since there is
	   always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in, 0, 56);
	} else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* Append length in bits and transform */
	((uint32_t *) ctx->in)[14] = ctx->bits[0];
	((uint32_t *) ctx->in)[15] = ctx->bits[1];

	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);
	memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f /*(x, y, z)*/ + data,  w = w<<s | w>>(32-s),  w += x )

static void MD5Transform(uint32_t buf[4], uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1(b,c,d), a, b, c, d, in[0] + 0xd76aa478L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[1] + 0xe8c7b756L, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[2] + 0x242070dbL, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[3] + 0xc1bdceeeL, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[4] + 0xf57c0fafL, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[5] + 0x4787c62aL, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[6] + 0xa8304613L, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[7] + 0xfd469501L, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[8] + 0x698098d8L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[9] + 0x8b44f7afL, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[10] + 0xffff5bb1L, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[11] + 0x895cd7beL, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[12] + 0x6b901122L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[13] + 0xfd987193L, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[14] + 0xa679438eL, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[15] + 0x49b40821L, 22);

	MD5STEP(F2(b,c,d), a, b, c, d, in[1] + 0xf61e2562L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[6] + 0xc040b340L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[11] + 0x265e5a51L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[0] + 0xe9b6c7aaL, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[5] + 0xd62f105dL, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[10] + 0x02441453L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[15] + 0xd8a1e681L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[4] + 0xe7d3fbc8L, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[9] + 0x21e1cde6L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[14] + 0xc33707d6L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[3] + 0xf4d50d87L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[8] + 0x455a14edL, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[13] + 0xa9e3e905L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[2] + 0xfcefa3f8L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[7] + 0x676f02d9L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[12] + 0x8d2a4c8aL, 20);

	MD5STEP(F3(b,c,d), a, b, c, d, in[5] + 0xfffa3942L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[8] + 0x8771f681L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[11] + 0x6d9d6122L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[14] + 0xfde5380cL, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[1] + 0xa4beea44L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[4] + 0x4bdecfa9L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[7] + 0xf6bb4b60L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[10] + 0xbebfbc70L, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[13] + 0x289b7ec6L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[0] + 0xeaa127faL, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[3] + 0xd4ef3085L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[6] + 0x04881d05L, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[9] + 0xd9d4d039L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[12] + 0xe6db99e5L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[15] + 0x1fa27cf8L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[2] + 0xc4ac5665L, 23);

	MD5STEP(F4(b,c,d), a, b, c, d, in[0] + 0xf4292244L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[7] + 0x432aff97L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[14] + 0xab9423a7L, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[5] + 0xfc93a039L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[12] + 0x655b59c3L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[3] + 0x8f0ccc92L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[10] + 0xffeff47dL, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[1] + 0x85845dd1L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[8] + 0x6fa87e4fL, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[15] + 0xfe2ce6e0L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[6] + 0xa3014314L, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[13] + 0x4e0811a1L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[4] + 0xf7537e82L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[11] + 0xbd3af235L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[2] + 0x2ad7d2bbL, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[9] + 0xeb86d391L, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

string string::md5(uint8_t *source, size_t len)
{
	string str(32);
	struct MD5Context md5;
	unsigned char digest[16];
	char out[33];
	unsigned p = 0;

	if(!len)
		len = strlen((char *)source);

	MD5Init(&md5);
	MD5Update(&md5, (unsigned char *)source, len);
	MD5Final(digest, &md5);

	while(p < 16) {
		snprintf(&out[p * 2], 3, "%2.2x", digest[p]);
		++p;
	}
	str = out;
	return str;
}
