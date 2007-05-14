#ifndef _UCOMMON_SERVICE_H_
#define	_UCOMMON_SERVICE_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef	_MSWINDOWS_
#include <signal.h>
#endif

NAMESPACE_UCOMMON

typedef fd_t spawniov_t[4];
typedef void (*action_t)(int signo);

typedef enum
{
	PRIORITY_LOWEST = 0,
	PRIORITY_LOW,
	PRIORITY_NORMAL,
	PRIORITY_HIGH
}	priority_t;

typedef enum
{
	SERVICE_INFO,
	SERVICE_NOTICE,
	SERVICE_WARNING,
	SERVICE_ERROR,
	SERVICE_FAILURE
} err_t;

typedef enum
{
	SPAWN_WAIT = 0,
	SPAWN_NOWAIT,
	SPAWN_DETACH
} spawn_t;

class __EXPORT MappedMemory
{
private:
	caddr_t map;
	fd_t fd;	

protected:
	size_t size, used;

	virtual void fault(void);

public:
	MappedMemory(const char *fname, size_t len = 0);
	virtual ~MappedMemory();

	inline operator bool() const
		{return (size != 0);};

	inline bool operator!() const
		{return (size == 0);};

	void *sbrk(size_t size);
	void *get(size_t offset);

	inline size_t len(void)
		{return size;};
};

class __EXPORT MappedReuse : protected ReusableAllocator, protected MappedMemory
{
private:
	unsigned objsize;

protected:
	MappedReuse(const char *name, size_t osize, unsigned count);

	bool avail(void);
	ReusableObject *request(void);
	ReusableObject *get(void);
	ReusableObject *get(timeout_t timeout);
};

class __EXPORT service : public mempager
{
protected:
	LinkedObject *root;

public:
	typedef struct
	{
		char *name;
		const char *value;
	}	define;

	typedef named_value<const char *> member;

	service(size_t paging, define *def = NULL);
	~service();

	void dup(const char *id, const char *val);
	void set(char *id, const char *val);
	const char *get(const char *id);

	inline member *find(const char *id)
		{return static_cast<member*>(NamedObject::find(static_cast<NamedObject*>(root), id));};

	inline member *begin(void)
		{return static_cast<member*>(root);};

#ifdef	_MSWINDOWS_
	char **getEnviron(void);
#else
	inline void block(sigset_t *set)
		{pthread_sigmask(SIG_BLOCK, set, NULL);};

	void block(int signo);

	void setEnviron();
#endif
	static void setenv(const char *id, const char *value);
	static void setenv(define *list);
	static int spawn(const char *fn, char **args, spawn_t mode, pid_t *pid, fd_t *iov = NULL, service *ep = NULL);
	static void createiov(fd_t *iov);
	static void closeiov(fd_t *iov);
	static void attachiov(fd_t *iov, fd_t io);
	static void detachiov(fd_t *iov);
	static fd_t pipeInput(fd_t *iov, size_t size = 0);
	static fd_t pipeOutput(fd_t *iov, size_t size = 0);
	static fd_t pipeError(fd_t *iov, size_t size = 0);

	static int scheduler(int policy, priority_t priority = PRIORITY_NORMAL);
	static int priority(priority_t priority);
	static void openlog(const char *id);
	static void errlog(err_t level, const char *fmt, ...);

	static void logfile(const char *id, const char *name, const char *fmt, ...);
	static size_t createctrl(const char *id);
	static bool control(const char *id, const char *fmt, ...);
	static size_t receive(char *buf, size_t max);
	static void releasectrl(const char *id);

#ifndef	_MSWINDOWS_
	static void attach(const char *dev);
	static void detach(void);
	static pid_t pidfile(const char *id);
	static pid_t pidfile(const char *id, pid_t pid);
#endif
	static bool getexit(pid_t pid, int *status);
	static int  wait(pid_t pid);
	static bool running(pid_t pid);
	static bool reload(const char *id);
	static bool shutdown(const char *id);
	static bool terminate(const char *id);
};

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
};

template <class T>
class mapped_array : public MappedMemory
{
public:
	inline mapped_array(const char *fn, unsigned members) : 
		MappedMemory(fn, members * sizeof(T)) {};

	inline void addLock(void)
		{sbrk(sizeof(T));};
	
	inline T *operator()(unsigned idx)
		{return static_cast<T*>(get(idx * sizeof(T)));}

	inline T *operator()(void)
		{return static_cast<T*>(sbrk(sizeof(T)));};
	
	inline T &operator[](unsigned idx)
		{return *(operator()(idx));};

	inline unsigned getSize(void)
		{return (unsigned)(size / sizeof(T));};
};

template <class T>
class mapped_reuse : protected MappedReuse
{
public:
	inline mapped_reuse(const char *fname, unsigned count) :
		MappedReuse(fname, sizeof(T), count) {};

	inline operator bool()
		{return MappedReuse::avail();};

	inline bool operator!()
		{return !MappedReuse::avail();};

	inline operator T*()
		{return mapped_reuse::get();};

	inline T* operator*()
		{return mapped_reuse::get();};

	inline T *get(void)
		{return static_cast<T*>(MappedReuse::get());};

    inline T *get(timeout_t timeout)
        {return static_cast<T*>(MappedReuse::get(timeout));};


	inline T *request(void)
		{return static_cast<T*>(MappedReuse::request());};

	inline void release(T *o)
		{MappedReuse::release(o);};
};
	
template <class T>
class mapped_view : protected MappedMemory
{
public:
	inline mapped_view(const char *fn) : 
		MappedMemory(fn) {};
	
	inline const char *id(unsigned idx)
		{return static_cast<const char *>(get(idx * sizeof(T)));};

	inline T *operator()(unsigned idx)
		{return static_cast<const T*>(get(idx * sizeof(T)));}
	
	inline T &operator[](unsigned idx)
		{return *(operator()(idx));};

	inline unsigned getSize(void)
		{return (unsigned)(size / sizeof(T));};
};

END_NAMESPACE

#endif
