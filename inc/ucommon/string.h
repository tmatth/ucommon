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

#ifndef	_UCOMMON_STRING_H_
#define	_UCOMMON_STRING_H_

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef	_UCOMMON_SOCKET_H_
#include <ucommon/socket.h>
#endif

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

typedef	unsigned short strsize_t;

NAMESPACE_UCOMMON

class __EXPORT string : public Object
{
protected:
	class __EXPORT cstring : public CountedObject
	{
	public:
#pragma pack(1)
		strsize_t max, len;
		char fill;
		char text[1];
#pragma pack()

		cstring(strsize_t size);
		cstring(strsize_t size, char fill);
		void clear(strsize_t offset, strsize_t size);
		void set(strsize_t offset, const char *str, strsize_t size);
		void set(const char *str);
		void add(const char *str);
		void add(char ch);
		void fix(void);
		void unfix(void);
		void inc(strsize_t adj);
		void dec(strsize_t adj);
	};

	cstring *str;

	cstring *create(strsize_t size, char fill = 0) const;

	virtual int compare(const char *s) const;
	virtual void retain(void);
	virtual void release(void);
	virtual cstring *c_copy(void) const;
	virtual void cow(strsize_t adj = 0);

public:
	static const strsize_t npos;

	string();
	string(strsize_t size);
	string(strsize_t size, char fill);
	string(strsize_t size, const char *fmt, ...);
	string(const char *str);
	string(const char *str, strsize_t size);
	string(const char *str, const char *end);
	string(const string &copy);
	virtual ~string();

	string get(strsize_t offset, strsize_t size = 0) const;
	int scanf(const char *format, ...) __SCANF(2, 3);
	int vscanf(const char *format, va_list args) __SCANF(2, 0);
	strsize_t printf(const char *format, ...) __PRINTF(2, 3);
	strsize_t vprintf(const char *format, va_list args) __PRINTF(2, 0);
	char *c_mem(void) const;
	const char *c_str(void) const;
	virtual bool resize(strsize_t size);
	void set(const char *s);
	void set(strsize_t offset, const char *str, strsize_t size = 0);
	void set(const char *s, char overflow, strsize_t offset, strsize_t size = 0);
	void rset(const char *s, char overflow, strsize_t offset, strsize_t size = 0);
	void add(const char *s);
	void add(char ch);
	void trim(const char *clist);
	void chop(const char *clist);
	void strip(const char *clist);
	bool unquote(const char *clist);
	void cut(strsize_t offset, strsize_t size = 0);
	void clear(strsize_t offset, strsize_t size = 0);
	void clear(void);
	void upper(void);
	void lower(void);
	strsize_t ccount(const char *clist) const;
	strsize_t count(void) const;
	strsize_t size(void) const;
	strsize_t offset(const char *c) const;
	char at(int ind) const;
	const char *last(const char *clist) const;
	const char *first(const char *clist) const;
	const char *begin(void) const;
	const char *end(void) const;
	const char *skip(const char *clist, strsize_t offset = 0) const;
	const char *rskip(const char *clist, strsize_t offset = npos) const;
	const char *find(const char *clist, strsize_t offset = 0) const;
	const char *rfind(const char *clist, strsize_t offset = npos) const;
	void split(const char *mark);
	void split(strsize_t offset);
	void rsplit(const char *mark);
	void rsplit(strsize_t offset);
	const char *chr(char ch) const;
	const char *rchr(char ch) const;
	strsize_t len(void);
	char fill(void);

	inline operator const char *() const
		{return c_str();};

	inline const char *operator*() const
		{return c_str();};

	bool full(void) const;
	string operator()(int offset, strsize_t len) const;
	const char *operator()(int offset) const;
	const char operator[](int offset) const;
	bool operator!() const;
	operator bool() const;
	string &operator^=(const string &s);
	string &operator^=(const char *str);
	string &operator+(const char *str);
	string &operator&(const char *str);
	string &operator=(const string &s);
	string &operator=(const char *str);
	string &operator++(void);
	string &operator+=(strsize_t inc);
	string &operator--(void);
	string &operator-=(strsize_t dec);
	bool operator==(const char *str) const;
	bool operator!=(const char *str) const;
	bool operator<(const char *str) const;
	bool operator<=(const char *str) const;
	bool operator>(const char *str) const;
	bool operator>=(const char *str) const;

	static int scanf(string &s, const char *fmt, ...) __SCANF(2, 3);
	static strsize_t printf(string &s, const char *fmt, ...) __PRINTF(2, 3);
	static int read(Socket &so, string &s);
	static int write(Socket &so, string &s);
	static int read(FILE *fp, string &s);
	static int write(FILE *fp, string &s); 
	static int read(DIR *dir, string &s);
	static bool getline(Socket &so, string &s);
	static bool putline(Socket &so, string &s);
	static bool getline(FILE *fp, string &s);
	static bool putline(FILE *fp, string &s);
	static void swap(string &s1, string &s2);
	static void fix(string &s);
	static void lower(char *s);
	static void upper(char *s);
	static const char *token(char *s, char **tokens, const char *clist, const char *quote = NULL, const char *eol = NULL);
	static char *skip(char *s, const char *clist);
	static char *rskip(char *s, const char *clist);
	static char *unquote(char *s, const char *clist);
	static char *rset(char *s, size_t sl, const char *d);
	static char *set(char *s, size_t sl, const char *d);
	static char *set(char *s, size_t sl, const char *d, size_t l);
	static char *add(char *s, size_t sl, const char *d); 
	static char *add(char *s, size_t sl, const char *d, size_t l);
	static size_t count(const char *s);
	static int compare(const char *s1, const char *s2);
	static int compare(const char *s1, const char *s2, size_t len);
	static int case_compare(const char *s1, const char *s2);
	static int case_compare(const char *s1, const char *s2, size_t len);
	static char *trim(char *str, const char *clist); 
	static char *chop(char *str, const char *clist); 
	static char *strip(char *str, const char *clist);
	static char *fill(char *str, size_t size, char chr);
	static unsigned ccount(const char *str, const char *clist);
	static char *find(char *str, const char *clist);
	static char *rfind(char *str, const char *clist);
	static char *last(char *str, const char *clist);
	static char *first(char *str, const char *clist);
	static char *dup(const char *s);

	inline static const char *token(string &s, char **tokens, const char *clist, const char *quote = NULL, const char *eol = NULL)
		{return token(s.c_mem(), tokens, clist, quote, eol);};

	inline static int vscanf(string &s, const char *fmt, va_list args)
		{return s.vscanf(fmt, args);} __SCANF(2, 0);

	inline static strsize_t vprintf(string &s, const char *fmt, va_list args)
		{return s.vprintf(fmt, args);} __PRINTF(2, 0);

	inline static strsize_t len(string &s)
		{return s.len();};

	inline static char *mem(string &s)
		{return s.c_mem();};

	inline static strsize_t size(string &s)
		{return s.size();};

	inline static void clear(string &s)
		{s.clear();};

	inline static unsigned ccount(string &s1, const char *clist)
		{return s1.ccount(clist);};

	inline static size_t count(string &s1)
		{return s1.count();};

	inline static void upper(string &s1)
		{s1.upper();};

	inline static void lower(string &s1)
		{s1.lower();};

	inline static bool unquote(string &s1, const char *clist)
		{return s1.unquote(clist);};

	inline static void trim(string &s, const char *clist)
		{return s.trim(clist);};

	inline static void chop(string &s, const char *clist)
		{return s.trim(clist);};

	inline static void strip(string &s, const char *clist)
		{return s.trim(clist);};

	inline static const char *find(string &s, const char *c)
		{return s.find(c);};

	inline static const char *rfind(string &s, const char *c)
		{return s.rfind(c);};

	inline static const char *first(string &s, const char *c)
		{return s.first(c);};

	inline static const char *last(string &s, const char *c)
		{return s.last(c);};

	inline static double tod(string &s, char **out = NULL)
		{return strtod(mem(s), out);};

	inline static long tol(string &s, char **out = NULL)
		{return strtol(mem(s), out, 0);};

	inline static double tod(const char *cp, char **out = NULL)
		{return strtod(cp, out);};

	inline static long tol(const char *cp, char **out = NULL)
		{return strtol(cp, out, 0);};
};

class __EXPORT memstring : public string
{
public:
	static const size_t header;

private:
	bool resize(strsize_t size);
	void cow(strsize_t adj = 0);
	void release(void);

protected:
	cstring *c_copy(void) const;

public:
	inline void operator=(string &s)
		{set(s.c_str());};

	inline void operator=(const char *s)
		{set(s);};

	memstring(void *mem, strsize_t size, char fill = 0);
	~memstring();

	static memstring *create(strsize_t size, char fill = 0);
	static memstring *create(mempager *pager, strsize_t size, char fill = 0);
};

template<size_t S>
class charbuf
{
private:
	char buffer[S];

public:
	inline charbuf() 
		{buffer[0] = 0;};

	inline charbuf(const char *s) 
		{string::set(buffer, S, s);};

	inline void operator=(const char *s)
		{string::set(buffer, S, s);};

	inline void operator+=(const char *s)
		{string::add(buffer, S, s);};

	inline operator bool() const
		{return buffer[0];};

	inline bool operator!() const
		{return buffer[0] == 0;};	

	inline operator char *()
		{return buffer;};

	inline char *operator*()
		{return buffer;};

	inline char operator[](size_t offset) const
		{return buffer[offset];};

	inline char *operator()(size_t offset)
		{return buffer + offset;};

	inline size_t size(void) const
		{return S;};
};

template<strsize_t S>
class stringbuf : public memstring
{
private:
	char buffer[sizeof(cstring) + S];
	
public:
	inline stringbuf() : memstring(buffer, S) {};

	inline stringbuf(const char *s) : memstring(buffer, S) {set(s);};

	inline void operator=(const char *s)
		{set(s);};

	inline void operator=(string &s)
		{set(s.c_str());};	
};

#ifndef _MSWINDOWS_

extern "C" inline int stricmp(const char *s1, const char *s2)
	{return string::case_compare(s1, s2);};

extern "C" inline int strnicmp(const char *s1, const char *s2, size_t n)
	{return string::case_compare(s1, s2, n);};

#endif

typedef	string string_t;

END_NAMESPACE

#endif
