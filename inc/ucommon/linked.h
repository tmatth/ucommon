#ifndef _UCOMMON_LINKED_H_
#define	_UCOMMON_LINKED_H_

#ifndef	_UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

NAMESPACE_UCOMMON

class OrderedObject;

class __EXPORT LinkedObject
{
protected:
	friend class objlink;		
	friend class objlist;
	friend class objring;
	friend class OrderedIndex;
	friend class LinkedRing;
	friend class NamedObject;

	LinkedObject *next;

	LinkedObject();
	LinkedObject(LinkedObject **root);
	virtual ~LinkedObject();

	virtual void release(void);

public:
	void enlist(LinkedObject **root);
	void delist(LinkedObject **root);

	static void purge(LinkedObject *root);

	inline LinkedObject *getNext(void) const
		{return next;};
};

class __EXPORT OrderedIndex
{
protected:
	friend class OrderedObject;
	friend class LinkedList;
	friend class LinkedRing;
	friend class NamedObject;
	friend class objring;
	friend class objlist;

	OrderedObject *head, *tail;

public:
	OrderedIndex();
	~OrderedIndex();

	LinkedObject *find(unsigned index);

	unsigned count(void);

	LinkedObject **index(void);
};

class __EXPORT OrderedObject : public LinkedObject
{
protected:
	friend class LinkedList;
	friend class OrderedIndex;
	friend class objring;
	friend class objlist;

	OrderedObject(OrderedIndex *root);
	OrderedObject();

public:
	void enlist(OrderedIndex *root);
	void delist(OrderedIndex *root);
};

class __EXPORT NamedObject : public OrderedObject
{
protected:
	const char *id;

    NamedObject(NamedObject **root, const char *id, unsigned max = 1);
	NamedObject(OrderedIndex *idx, const char *id);

public:
	static void purge(NamedObject **idx, unsigned max);

	static NamedObject **index(NamedObject **idx, unsigned max);

	static unsigned count(NamedObject **idx, unsigned max);

	static NamedObject *find(NamedObject *root, const char *id);

	static NamedObject *map(NamedObject **idx, const char *id, unsigned max);

	static NamedObject *skip(NamedObject **idx, NamedObject *current, unsigned max);

	static unsigned keyindex(const char *id, unsigned max);

	static NamedObject **sort(NamedObject **list, size_t count = 0);

	inline const char *getId(void) const
		{return id;};

	virtual bool compare(const char *cmp) const;

	inline bool operator==(const char *cmp)
		{return compare(cmp);};

	inline bool operator!=(const char *cmp)
		{return !compare(cmp);};
};

class __EXPORT NamedList : public NamedObject
{
protected:
	friend class objmap;

	NamedObject **keyroot;
	unsigned keysize;

	NamedList(NamedObject **root, const char *id, unsigned max);
	virtual ~NamedList();

public:
	void delist(void);
};		

class __EXPORT LinkedList : public OrderedObject
{
protected:
	friend class LinkedRing;
	friend class objlist;
	friend class objring;

	LinkedList *prev;
	OrderedIndex *root;

	LinkedList(OrderedIndex *root);
	LinkedList();

	virtual ~LinkedList();

public:
	void delist(void);
	void enlist(OrderedIndex *root);

	inline bool isHead(void) const
		{return root->head == (OrderedObject *)this;};

	inline bool isTail(void) const
		{return root->tail == (OrderedObject *)this;};

	inline LinkedList *getPrev(void) const
		{return prev;};
};

class __EXPORT LinkedRing : public LinkedList
{
protected:
	friend class objring;

	LinkedRing();
	LinkedRing(OrderedIndex *root);
};
	
class __EXPORT objlink
{
protected:
	LinkedObject *object;

public:
	objlink(LinkedObject *obj);

	void operator++();

	objlink &operator=(LinkedObject *root);

	unsigned count(void);
};

class __EXPORT objmap
{
protected:
	NamedList *object;

public:
	objmap(NamedList *obj);

	void operator++();

	objmap &operator=(NamedList *root);

	unsigned count(void);
	void begin(void);
};


class __EXPORT objlist
{
protected:
	LinkedList *object;

public:
	objlist(LinkedList *obj = 0);
	~objlist();

	void operator++();
	void operator--();

	void clear(void);
	void begin(void);
	void end(void);
	unsigned count(void);

	objlist &operator=(LinkedList *l);
};

class __EXPORT objring : public objlist
{
public:
	objring(LinkedObject *obj = 0);
	
	void fifo(void);
	void lifo(void);
};

template <class T, unsigned M = 177>
class keymap
{
private:
	NamedObject *idx[M];

public:
	inline ~keymap()
		{NamedObject::purge(idx, M);};

	inline NamedObject **root(void)
		{return idx;};

	inline unsigned limit(void)
		{return M;};

	inline T *get(const char *id)
		{return static_cast<T*>(NamedObject::map(idx, id, M));};

	inline T *begin(void)
		{return static_cast<T*>(NamedObject::skip(idx, NULL, M));};

	inline T *next(T *current)
		{return static_cast<T*>(NamedObject::skip(idx, current, M));};

	inline unsigned count(void)
		{return NamedObject::count(idx, M);};

	inline T **index(void)
		{return NamedObject::index(idx, M);};

	inline T **sort(void)
		{return NamedObject::sort(NamedObject::index(idx, M));};
}; 

template <class T>
class keylist : public OrderedIndex
{
public:
    typedef pointer<T, objlink> pointer;

	inline NamedObject **root(void)
		{return static_cast<NamedObject*>(&head);};

	inline T *begin(void)
		{return static_cast<T*>(head);};

	inline T *end(void)
		{return static_cast<T*>(tail);};

	inline T *create(const char *id)
		{return new T(this, id);};

    inline T *next(LinkedObject *current)
        {return static_cast<T*>(current->getNext());};

    inline T *find(const char *id)
        {return static_cast<T*>(NamedObject::find(begin(), id));};

    inline T *operator[](unsigned index)
        {return static_cast<T*>(OrderedIndex::find(index));};

	inline T **sort(void)
		{return NamedObject::sort(index());};
};

template <class T>
class list : public OrderedIndex
{
public:
	typedef pointer<T, objlist> pointer;

	inline LinkedObject **root(void)
		{return &head;};

	inline T *begin(void)
		{return static_cast<T*>(head);};

    inline T *end(void)
        {return static_cast<T*>(tail);};

	inline T *create(void)
		{return new T(this);};

	inline T *next(LinkedObject *current)
		{return static_cast<T*>(current->getNext());};

	inline T *prev(LinkedList *current)
		{return static_cast<T*>(current->getPrev());};

	inline T *operator[](unsigned index)
		{return static_cast<T*>(OrderedIndex::find(index));};
};

template <class T>
class linked : protected LinkedList, public T
{
public:
	typedef linked<T> member;
	typedef	pointer<member, objlist> pointer;
	typedef	list<member> container;

	inline linked(container *index) : 
		LinkedList(index), T() {};

	inline linked(const T &o, container *index) :
		LinkedList(index), T(o) {};
};

template <class T>
class slinked : protected LinkedObject, public T
{
public:
	typedef slinked<T> member;
    typedef pointer<member, objlist> pointer;
    typedef list<member> container;

    inline slinked(container *index) : 
		LinkedObject(index->root), T() {};

	inline slinked(const T& o, container *index) :
		LinkedObject(index->root), T(o) {};
};

template <class T>
class named : protected NamedObject, public T
{
public:
	typedef named<T> member;
    typedef pointer<member, objlist> pointer;
    typedef keylist<member> container;

    inline named(container *index, const char *id) : 
		NamedObject(index, id), T() {};

	inline named(const T &o, container *index, const char *id) :
		NamedObject(index, id), T(o) {};
};

template <class T, unsigned M=177>
class mapped : protected NamedList, public T
{
public:
	typedef mapped<T, M> member;
	typedef pointer<member, objmap> pointer;
	typedef keymap<member, M> container;

	inline mapped(const T &o, container *c, const char *id) :
		NamedList(c->root(), id, M), T(o) {};

	inline mapped(container *c, const char *id) :
		NamedList(c->root(), id, M), T() {};
};

END_NAMESPACE

#endif
