#ifndef	_UCOMMON_STRING_H_
#define	_UCOMMON_STRING_H_

#ifndef	_UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#include <stdio.h>
#include <string.h>
#include <dirent.h>

typedef	unsigned short strsize_t;

NAMESPACE_UCOMMON

class __EXPORT string
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
		void inc(strsize_t adj);
		void dec(strsize_t adj);
	};

	cstring *str;

	cstring *create(strsize_t size, char fill = 0) const;

	virtual int compare(const char *s) const;
	virtual void release(void);
	virtual cstring *c_copy(void) const;
	virtual void cow(strsize_t adj = 0);

public:
	static const strsize_t npos;

	string();
	string(strsize_t size);
	string(strsize_t size, char fill);
	string(strsize_t size, const char *fmt, ...);
	string(const char *str, strsize_t size = 0);
	string(const char *str, const char *end);
	string(const string &copy);
	virtual ~string();

	string get(strsize_t offset, strsize_t size = 0) const;
	void printf(const char *format, ...);	
	char *c_mem(void) const;
	const char *c_str(void) const;
	virtual bool resize(strsize_t size);
	void set(const char *s);
	void set(strsize_t offset, const char *str, strsize_t size = 0);
	void add(const char *s);
	void add(char ch);
	void trim(const char *clist);
	void rtrim(const char *clist);
	void strip(const char *clist);
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
	const char *find(const char *clist, strsize_t offset = 0) const;
	const char *rfind(const char *clist, strsize_t offset = npos) const;
	const char *chr(char ch) const;
	const char *rchr(char ch) const;

	inline operator const char *() const
		{return c_str();};

	inline const char *operator*() const
		{return c_str();};

	const char *operator[](int offset) const;
	operator strsize_t() const;
	bool operator!() const;
	string &operator<<(::DIR *dp);
	string &operator<<(::FILE *fp);
	string &operator>>(::FILE *fp);
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

	friend __EXPORT void swap(string &s1, string &s2);
};

class __EXPORT memstring : public string
{
private:
	static const size_t header;
	bool resize(strsize_t size);

protected:
	cstring *c_copy(void) const;
	void cow(strsize_t adj = 0);

public:
	memstring(void *mem, strsize_t size, char fill = 0);
	~memstring();
};

class __EXPORT istring : public string
{
protected:
	virtual int compare(const char *s) const;

public:
	static const strsize_t npos;

	istring();
	istring(strsize_t size);
	istring(strsize_t size, char fill);
	istring(strsize_t size, const char *fmt, ...);
	istring(const char *str, strsize_t size = 0);
	istring(const char *str, const char *end);
	istring(const string &copy);
};

class __EXPORT tokenstring : protected string
{
private:
	const char *clist;
	char *token;

public:
	tokenstring(const char *cl);
	tokenstring(caddr_t mem, const char *cl);
	
	const char *get(const char *quoting);
	const char *get(void);
	const char *next(void);

	tokenstring &operator=(caddr_t s);

	tokenstring &operator=(const string &s);

	inline const char *operator*()
		{return get();};
};

__EXPORT void swap(string &s1, string &s2);

END_NAMESPACE

extern "C" {

	__EXPORT char *strset(char *str, size_t size, const char *src);
	__EXPORT char *stradd(char *str, size_t size, const char *src);
	__EXPORT char *strtrim(char *str, const char *clist); 
	__EXPORT char *strrtrim(char *str, const char *clist); 
	__EXPORT char *strstrip(char *str, const char *clist);
	__EXPORT char *strupper(char *str);
	__EXPORT char *strlower(char *str);
	__EXPORT unsigned strccount(const char *str, const char *clist);
	__EXPORT char *strfill(char *str, size_t size, const char fill);
	__EXPORT char *strfield(char *str, const char *s, const char fill, size_t offset = 0, size_t len = 0);
	__EXPORT char *strclear(char *str, size_t offset, size_t len, const char fill);
	__EXPORT char *strfind(char *str, const char *clist);
	__EXPORT char *strrfind(char *str, const char *clist);
	__EXPORT char *strlast(char *str, const char *clist);
	__EXPORT char *strfirst(char *str, const char *clist);
	__EXPORT timeout_t strtotimeout(const char *cp, char **ep = NULL, bool sec = false);
};

#ifndef	_MSWINDOWS_
#define	stricmp strcasecmp
#define	strnicmp strncasecmp
#define	stristr strcasestr
#endif
	
#endif
