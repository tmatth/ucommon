#ifndef _UCOMMON_MISC_H_
#define	_UCOMMON_MISC_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT xmlnode : public OrderedObject
{
protected:
	friend class XMLTree;

	xmlnode();
	xmlnode(xmlnode *parent, const char *id);
	OrderedIndex child;
	const char *id;
	const char *text;
	xmlnode *parent;

public:
	inline const char *getId(void)
		{return id;};

	inline const char *getText(void)
		{return text;};

	const char *getValue(const char *id);
};

class __EXPORT XMLTree : public SharedObject, public mempager
{
public:
	XMLTree(size_t s);
	virtual ~XMLTree();

	bool load(const char *name);

	inline operator bool()
		{return (root.id != NULL);};

	inline bool operator!()
		{return (root.id == NULL);};

protected:
	xmlnode root;
	bool updated;
	unsigned loaded;
	size_t size;

    bool change(xmlnode *node, const char *text);
    void remove(xmlnode *node);
    xmlnode *add(xmlnode *parent, const char *id, const char *text = NULL);

	virtual bool validate(xmlnode *node);
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
