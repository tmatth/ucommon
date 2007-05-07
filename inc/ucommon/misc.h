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

const size_t uuid_size = 38;

class __EXPORT config : public mempager
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
		
		inline const config *operator->()
			{return config::cfg;};
	};

	class __EXPORT callback : public OrderedObject
    {
	protected:
		friend class config;

		static OrderedIndex list;

        callback();
        virtual ~callback();

        void release(void);

        virtual void commit(config *cfg);
		virtual void reload(config *cfg);
    };
    
	config(char *name, size_t s = 0);
	virtual ~config();

	bool loadxml(const char *name, keynode *top = NULL);
	bool loadconf(const char *name, keynode *top = NULL, char *gid = NULL, keynode *entry = NULL);
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

	static config *cfg;
	static SharedLock lock;

	linked_pointer<callback> cb; // for update...
	keynode root;
	stringbuf<1024> buffer;

public:
	static void md5hash(char *out, const char *source, size_t size = 0);
	static int uuid(char *out);
	static size_t urldecode(char *out, size_t limit, const char *src);
	static size_t urlencode(char *out, size_t limit, const char *src);
	static size_t urlencodesize(char *str);
	static size_t xmldecode(char *out, size_t limit, const char *src);
	static size_t xmlencode(char *out, size_t limit, const char *src);
	static size_t b64decode(caddr_t out, const char *src, size_t count);
	static size_t b64encode(char *out, caddr_t src, size_t count);
	static size_t b64len(const char *str);
};

END_NAMESPACE

#endif
