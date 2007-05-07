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
	void printf(const char *format, ...);	
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

	inline bool isNumeric(void) const
		{return isnumeric(c_str());};

	inline bool isInteger(void) const
		{return isinteger(c_str());};

	bool full(void) const;
	string operator()(int offset, strsize_t len) const;
	const char *operator()(int offset) const;
	const char operator[](int offset) const;
	operator strsize_t() const;
	bool operator!() const;
	operator bool() const;
	string &operator<<(::DIR *dir);
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

	static void swap(string &s1, string &s2);
	
	static void lower(char *s);
	static void upper(char *s);
	static char *skip(char *s, const char *clist);
	static char *rskip(char *s, const char *clist);
	static char *unquote(char *s, const char *clist);
	static char *rset(char *s, size_t s, const char *d);
	static char *set(char *s, size_t s, const char *d);
	static char *set(char *s, size_t s, const char *d, size_t l);
	static char *add(char *s, size_t s, const char *d); 
	static char *add(char *s, size_t s, const char *d, size_t l);
	static size_t count(const char *s);
	static int compare(const char *s1, const char *s2);
	static int compare(const char *s1, const char *s2, size_t len);
	static int casecompare(const char *s1, const char *s2);
	static int casecompare(const char *s1, const char *s2, size_t len);
	static timeout_t totimeout(const char *cp, char **ep = NULL, bool sec = false);
	static bool tobool(const char *cp, char **ep = NULL);
	static int32_t toint(const char *cp, char **ep = NULL);
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
	static bool isnumeric(const char *s);
	static bool isinteger(const char *s);

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

	inline static bool isinteger(string &s)
		{return isinteger(s.c_str());};

	inline static bool isnumeric(string &s)
		{return isnumeric(s.c_str());};
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

#endif
