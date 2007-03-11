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
	void printf(const char *format, ...);	
	char *c_mem(void) const;
	const char *c_str(void) const;
	virtual bool resize(strsize_t size);
	void set(const char *s);
	void set(strsize_t offset, const char *str, strsize_t size = 0);
	void add(const char *s);
	void add(char ch);
	void trim(const char *clist);
	void chop(const char *clist);
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
	strsize_t max(void);
	char fill(void);

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

template<strsize_t S>
class stringbuf : public memstring
{
private:
	char buffer[sizeof(cstring) + S];
	
public:
	inline stringbuf() : memstring(buffer, S) {};

	inline void operator=(const char *s)
		{set(s);};

	inline void operator=(string &s)
		{set(s.c_str());};	
};

END_NAMESPACE

extern "C" {
	__EXPORT const char *cpr_strstr(const char *body, const char *item);
	__EXPORT const char *cpr_stristr(const char *body, const char *item);
	__EXPORT int cpr_strcmp(const char *s1, const char *s2);
	__EXPORT int cpr_strncmp(const char *s1, const char *s2, size_t len);
    __EXPORT int cpr_stricmp(const char *s1, const char *s2);
    __EXPORT int cpr_strnicmp(const char *s1, const char *s2, size_t len);
	__EXPORT char *cpr_strchr(const char *str, char c);
	__EXPORT char *cpr_strrchr(const char *str, char c);
	__EXPORT size_t cpr_strlen(const char *cp);
	__EXPORT char *cpr_strdup(const char *cp);
	__EXPORT char *cpr_strset(char *str, size_t size, const char *src);
	__EXPORT char *cpr_strnset(char *str, size_t size, const char *src, size_t len);
	__EXPORT char *cpr_stradd(char *str, size_t size, const char *src);
	__EXPORT char *cpr_strnadd(char *str, size_t size, const char *src, size_t len);
	__EXPORT char *cpr_strtrim(char *str, const char *clist); 
	__EXPORT char *cpr_strrtrim(char *str, const char *clist); 
	__EXPORT char *cpr_strstrip(char *str, const char *clist);
	__EXPORT char *cpr_strupper(char *str);
	__EXPORT char *cpr_strlower(char *str);
	__EXPORT unsigned cpr_strccount(const char *str, const char *clist);
	__EXPORT char *cpr_strfill(char *str, size_t size, const char fill);
	__EXPORT char *cpr_strfield(char *str, const char *s, const char fill, size_t offset = 0, size_t len = 0);
	__EXPORT char *cpr_strclear(char *str, size_t offset, size_t len, const char fill);
	__EXPORT char *cpr_strskip(char *str, const char *clist);
	__EXPORT char *cpr_strrskip(char *str, const char *clist);
	__EXPORT char *cpr_strfind(char *str, const char *clist);
	__EXPORT char *cpr_strrfind(char *str, const char *clist);
	__EXPORT char *cpr_strlast(char *str, const char *clist);
	__EXPORT char *cpr_strfirst(char *str, const char *clist);
	__EXPORT timeout_t cpr_strtotimeout(const char *cp, char **ep = NULL, bool sec = false);
	__EXPORT bool cpr_strtobool(const char *cp, char **ep = NULL);
	__EXPORT int32_t cpr_strtoint(const char *cp, char **ep = NULL);
};

#endif
