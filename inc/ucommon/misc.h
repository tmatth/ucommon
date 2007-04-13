#ifndef _UCOMMON_MISC_H_
#define	_UCOMMON_MISC_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT keyconfig : public mempager
{
public:
	typedef treemap<char *>keynode;

	typedef struct {
		const char *key;
		const char *value;
	} define;

	class __EXPORT instance
	{
	private:
		int state;
	
	public:
		instance();
		~instance();
		
		inline const keyconfig *operator->()
			{return keyconfig::cfg;};
	};

	class __EXPORT callback : public OrderedObject
    {
	protected:
		friend class keyconfig;

		static OrderedIndex list;

        callback();
        virtual ~callback();

        void release(void);

        virtual void commit(keyconfig *cfg);
		virtual void reload(keyconfig *cfg);
    };
    
	keyconfig(char *name, size_t s = 0);
	virtual ~keyconfig();

	bool loadxml(const char *name, keynode *top = NULL);
	keynode *getPath(const char *path);
	keynode *getNode(keynode *base, const char *id, const char *value);	
	keynode *addNode(keynode *base, define *defs);
	keynode *addNode(keynode *base, const char *id, const char *value);
	const char *getValue(keynode *base, const char *id, keynode *copy = NULL);

	inline static bool isLinked(keynode *node)
		{return node->isLeaf();};

	inline static bool isValue(keynode *node)
		{return (node->getPointer() != NULL);};

	inline static bool isUndefined(keynode *node)
		{return !isLinked(node) && !isValue(node);};

	inline static bool isNode(keynode *node)
		{return isLinked(node) && isValue(node);};

	void update(void);
	void commit(void);

	inline static void protect(void)
		{lock.access();};

	inline static void release(void)
		{lock.release();};

protected:
	friend class instance;

	static keyconfig *cfg;
	static SharedLock lock;

	linked_pointer<callback> cb; // for update...
	keynode root;
	stringbuf<1024> buffer;
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
