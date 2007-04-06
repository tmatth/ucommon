#ifndef _UCOMMON_MISC_H_
#define	_UCOMMON_MISC_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

NAMESPACE_UCOMMON

typedef treemap<char *> xmlnode;

class __EXPORT XMLTree : public mempager
{
public:
    
	XMLTree(size_t s, char *name);
	virtual ~XMLTree();

	bool load(const char *name, xmlnode *top = NULL);

protected:
	xmlnode root;
	unsigned loaded;
	size_t size;

    bool change(xmlnode *node, const char *text);
    void remove(xmlnode *node);
    xmlnode *add(xmlnode *parent, const char *id, const char *text = NULL);
	xmlnode *search(xmlnode *base, const char *leaf, const char *value);

	inline xmlnode *find(const char *id)
		{return root.find(id);};

	inline xmlnode *path(const char *p)
		{return root.path(p);};

	virtual bool validate(xmlnode *node);
};

class __EXPORT xmlconfig : public XMLTree
{
public:
	typedef xmlnode node;

	class __EXPORT callback : public OrderedObject
    {
	friend class xmlconfig;

    protected:
		static OrderedIndex list;

        callback();
        virtual ~callback();

        void release(void);

        virtual void commit(xmlconfig *cfg);
		virtual void reload(xmlconfig *cfg);
    };

	void commit(void);
	void update(void); // calls new uncommited callbacks...

	static xmlconfig *get(void);
	static void protect(void);
	static void release(void);

protected:
	static xmlconfig *config;
	void lock(void);

private:
	static SharedLock rwlock;
	static linked_pointer<callback> cb; // for update...
};

END_NAMESPACE

extern "C" {
	const size_t uuid_size = 38;

	__EXPORT void cpr_md5hash(char *out, const char *source, size_t size = 0);
	__EXPORT int cpr_uuid(char *out);
	__EXPORT size_t cpr_urldecode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_urlencode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_urlencodesize(char *str);
	__EXPORT size_t cpr_xmldecode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_xmlencode(char *out, size_t limit, const char *src);
	__EXPORT size_t cpr_b64decode(caddr_t out, const char *src, size_t count);
	__EXPORT size_t cpr_b64encode(char *out, caddr_t src, size_t count);
	__EXPORT size_t cpr_b64len(const char *str);
	__EXPORT size_t cpr_snprintf(char *buf, size_t size, const char *fmt, ...);
	__EXPORT void cpr_printlog(const char *path, const char *fmt, ...);
};

#ifdef	DEBUG
#define cpr_printdbg(fmt, ...)	printf(fmt, ...)
#else
#define	cpr_printdbg(fmt, ...)
#endif

#endif
