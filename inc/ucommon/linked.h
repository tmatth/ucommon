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

/**
 * Linked objects, lists, templates, and containers.
 * Common support for objects that might be organized as single and double 
 * linked lists, rings and queues, and tree oriented data structures.  These
 * generic classes may be used to help form anything from callback 
 * registration systems and indexed memory hashes to xml parsed tree nodes.  
 * @file ucommon/linked.h
 */

#ifndef _UCOMMON_LINKED_H_
#define	_UCOMMON_LINKED_H_

#ifndef	_UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

NAMESPACE_UCOMMON

class OrderedObject;

/**
 * Common base class for all objects that can be formed into a linked list.  
 * This base class is used directly for objects that can be formed into a 
 * single linked list.  It is also used directly as a type for a pointer to the
 * start of list of objects that are linked together as a list.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT LinkedObject : public Object
{
protected:
	friend class OrderedIndex;
	friend class LinkedRing;
	friend class NamedObject;

	LinkedObject *next;

	/**
	 * Construct base class attached to a chain of objects.
	 * @param root pointer to chain of objects we are part of.
	 */
	LinkedObject(LinkedObject **root);

	/**
	 * Construct base class unattached to anyone.  This might be
	 * used to construct intermediary base classes that may form
	 * lists through indexing objects.
	 */
	LinkedObject();

public:
	static const LinkedObject *nil; /**< Marker for end of linked list. */
	static const LinkedObject *inv;	/**< Marker for invalid list pointer */

	virtual ~LinkedObject();

	/**
	 * Release list, mark as no longer linked.  Inherited from base Object.
	 */
	virtual void release(void);

	/**
	 * Retain by marking as self referenced list. Inherited from base Object.
	 */
	virtual void retain(void);

	/**
	 * Add our object to an existing linked list through a pointer.  This
	 * forms a container sorted in lifo order since we become the head
	 * of the list, and the previous head becomes our next.
	 * @param root pointer to list we are adding ourselves to.
	 */
	void enlist(LinkedObject **root);

	/**
	 * Locate and remove ourselves from a list of objects.  This searches
	 * the list to locate our object and if found relinks the list around
	 * us.
	 * @param root pointer to list we are removing ourselves from.
	 */
	void delist(LinkedObject **root);

	/**
	 * Search to see if we are a member of a specific list.
	 * @return true if we are member of the list.
	 */
	bool isMember(LinkedObject *list) const;

	/**
	 * Release all objects from a list.
	 * @param root pointer to list we are purging.
	 */
	static void purge(LinkedObject *root);

	/**
	 * Count the number of linked objects in a list.
	 * @param root pointer to list we are counting.
	 */
	static unsigned count(LinkedObject *root);

	/**
	 * Get next effective object when iterating.
	 * @return next linked object in list.
	 */
	inline LinkedObject *getNext(void) const
		{return next;};
};

/**
 * Reusable objects for forming private heaps.  Reusable objects are
 * linked objects that may be allocated in a private heap, and are
 * returned to a free list when they are no longer needed so they can
 * be reused without having to be re-allocated.  The free list is the
 * root of a linked object chain.  This is used as a base class for those
 * objects that will be managed through reusable heaps.
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
class __EXPORT ReusableObject : public LinkedObject
{
	friend class ReusableAllocator;

protected:
	virtual void release(void);

public:
	/**
	 * Get next effective reusable object when iterating.
	 * @return next reusable object in list.
	 */
	inline ReusableObject *getNext(void)
		{return static_cast<ReusableObject*>(LinkedObject::getNext());};
};

/**
 * An index container for maintaining an ordered list of objects.
 * This index holds a pointer to the head and tail of an ordered list of
 * linked objects.  Fundimental methods for supporting iterators are
 * also provided.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT OrderedIndex
{
protected:
	friend class OrderedObject;
	friend class LinkedList;
	friend class NamedObject;

	OrderedObject *head, *tail;

public:
	/**
	 * Create and initialize an empty index.
	 */
	OrderedIndex();

	/**
	 * Destroy index.
	 */
	virtual ~OrderedIndex();

	/**
	 * Find a specific member in the ordered list.
	 * @param offset to member to find.
	 */
	LinkedObject *find(unsigned offset) const;

	/**
	 * Count of objects this list manages.
	 * @return number of objects in the list.
	 */
	unsigned count(void) const;

	/**
	 * Purge the linked list and then set the index to empty.
	 */	
	void purge(void);

	/**
	 * Used to synchronize lists managed by multiple threads.  A derived
	 * locking method would be invoked.
	 */
	virtual void lock_index(void);

	/**
	 * Used to synchronize lists managed by multiple threads.  A derived
	 * unlocking method would be invoked.
	 */
	virtual void unlock_index(void);

	/**
	 * Return a pointer to the head of the list.  This allows the head
	 * pointer to be used like a simple root list pointer for pure
	 * LinkedObject based objects.
	 * @return LinkedIndex style object.
	 */
	LinkedObject **index(void) const;

	/**
	 * Get (pull) object off the list.  The start of the list is advanced to 
	 * the next object.
	 * @return LinkedObject based object that was head of the list.
	 */
	LinkedObject *get(void);

	/**
	 * Return first object in list for iterators.
	 * @return first object in list.
	 */
	inline LinkedObject *begin(void) const
		{return (LinkedObject*)(head);};

	/**
	 * Return last object in list for iterators.
	 * @return last object in list.
	 */
	inline LinkedObject *end(void) const
		{return (LinkedObject*)(tail);};

	/**
	 * Return head object pointer.
	 * @return head pointer.
	 */
	inline LinkedObject *operator*() const
		{return (LinkedObject*)(head);};
};

/**
 * A linked object base class for ordered objects.  This is used for
 * objects that must be ordered and listed through the OrderedIndex
 * class.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT OrderedObject : public LinkedObject
{
protected:
	friend class LinkedList;
	friend class OrderedIndex;

	/**
	 * Construct an ordered object aot end of a an index.
	 * @param index we are listed on.
	 */
	OrderedObject(OrderedIndex *index);

	/**
	 * Construct an ordered object unattached.
	 */
	OrderedObject();

public:
	/**
	 * List our ordered object at end of a linked list on an index.
	 * @param index we are listing on.
	 */
	void enlist(OrderedIndex *index);

	/**
	 * Remove our ordered object from an existing index.
	 * @param index we are listed on.
	 */
	void delist(OrderedIndex *index);

	/**
	 * Get next ordered member when iterating.
	 * @return next ordered object.
	 */
	inline OrderedObject *getNext(void) const
		{return static_cast<OrderedObject *>(LinkedObject::getNext());};
};

/**
 * A linked object base class with members found by name.  This class is
 * used to help form named option lists and other forms of name indexed
 * associative data structures.  The id is assumed to be passed from a
 * dupped or dynamically allocated string.  If a constant string is used
 * then you must not call delete for this object.
 *
 * Named objects are either listed on an ordered list or keyed through an
 * associate hash map table.  When using a hash table, the name id string is
 * used to determine the slot number to use in a list of n sized linked
 * object lists.  Hence, a hash index refers to a specific sized array of 
 * object indexes.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT NamedObject : public OrderedObject
{
protected:
	char *id;

	/**
	 * Create an empty unnamed cell object.
	 */
	NamedObject();

	/**
	 * Create a named object and add to hash indexed list.
	 * @param hash map table to list node on.
	 * @param name of the object we are listing.
	 * @param size of hash map table used.
	 */
    NamedObject(NamedObject **hash, char *name, unsigned size = 1);

	/**
	 * Created a named object on an ordered list.  This is commonly used
	 * to form attribute lists.
	 * @param index to list object on.
	 * @param name of the object we are listing.
	 */
	NamedObject(OrderedIndex *index, char *name);

	/**
	 * Destroy named object.  We do not always destroy named objects, since
	 * we may use them in reusable pools or we may initialize a list that we
	 * keep permenantly.  If we do invoke delete for something based on
	 * NamedObject, then be aware the object id is assumed to be formed from 
	 * a dup'd string which will also be freed.
	 */
	~NamedObject();

public:
	/**
	 * Purge a hash indexed table of named objects.
	 * @param hash map table to purge.
	 * @param size of hash map table used.
	 */
	static void purge(NamedObject **hash, unsigned size);

	/**
	 * Convert a hash index into a linear object pointer array.  The
	 * object pointer array is created from the heap and must be deleted
	 * when no longer used.
	 * @param hash map table of objects to index.
	 * @param size of hash map table used.
	 * @return array of named object pointers.
	 */
	static NamedObject **index(NamedObject **idx, unsigned max);

	/**
	 * Count the total named objects in a hash table.
	 * @param hash map table of objects to index.
	 * @param size of hash map table used.
	 */
	static unsigned count(NamedObject **hash, unsigned size);

	/**
	 * Find a named object from a simple list.  This may also use the
	 * begin() member of an ordered index of named objects.
	 * @param root node of named object list.
	 * @param name of object to find.
	 * @return object pointer or NULL if not found.
	 */
	static NamedObject *find(NamedObject *root, const char *name);

	/**
	 * Find a named object through a hash map table.
	 * @param hash map table of objects to search.
	 * @param name of object to find.
	 * @param size of hash map table.
	 */
	static NamedObject *map(NamedObject **idx, const char *id, unsigned max);

	/**
	 * Iterate through a hash map table.
	 * @param hash map table to iterate.
	 * @param current named object we iterated or NULL to find start of list.
	 * @param size of map table.
	 * @return next named object in hash map or NULL if no more objects.
	 */
	static NamedObject *skip(NamedObject **hash, NamedObject *current, unsigned size);

	/**
	 * Internal function to convert a name to a hash index number.
	 * @param name to convert into index.
	 * @param size of map table.
	 */
	static unsigned keyindex(const char *name, unsigned size);

	/**
	 * Sort an array of named objects in alphabetical order.  This would
	 * typically be used to sort a list created and returned by index().
	 * @param list of named objects to sort.
	 * @param count of objects in the list or 0 to find by NULL pointer.
	 * @return list in sorted order.
	 */
	static NamedObject **sort(NamedObject **list, size_t count = 0);

	/**
	 * Get next effective object when iterating.
	 * @return next linked object in list.
	 */
	inline NamedObject *getNext(void) const
		{return static_cast<NamedObject*>(LinkedObject::getNext());};

	/**
	 * Get the named id string of this object.
	 * @return name id.
	 */
	inline char *getId(void) const
		{return id;};

	/**
	 * Compare the name of our object to see if equal.  This is a virtual
	 * so that it can be overriden when using named lists or hash lookups
	 * that must be case insensitive.
	 * @param name to compare our name to.
	 * @return true if effectivily equal.
	 */
	virtual bool compare(const char *name) const;

	/**
	 * Comparison operator between our name and a string.
	 * @param name to compare with.
	 * @return true if equal.
	 */
	inline bool operator==(const char *name) const
		{return compare(name);};

	/**
	 * Comparison operator between our name and a string.
	 * @param name to compare with.
	 * @return true if not equal.
	 */
	inline bool operator!=(const char *name) const
		{return !compare(name);};
};

/**
 * The named tree class is used to form a tree oriented list of associated
 * objects.  Typical uses for such data structures might be to form a
 * parsed XML document, or for forming complex configuration management
 * systems or for forming system resource management trees.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT NamedTree : public NamedObject
{
protected:
	NamedTree *parent;
	OrderedIndex child;

	/**
	 * Create a stand-alone or root tree node, with an optional name.
	 * @param name for this node.
	 */
	NamedTree(char *name = NULL);

	/**
	 * Create a tree node as a child of an existing node.
	 * @param parent node we are listed under.
	 * @param name of our node.
	 */
	NamedTree(NamedTree *parent, char *name);

	/**
	 * Delete node in a tree.  If we delete a node, we must delist it from
	 * it's parent.  We must also delink any child nodes.  This is done by
	 * calling the purge() member.
	 */
	virtual ~NamedTree();

	/**
	 * Performs object destruction.  Note, if we delete a named tree object
	 * the name of our member object is assumed to be a dynamically allocated
	 * string that will also be free'd.
	 */
	void purge(void);

public:
	/**
	 * Find a child node of our object with the specified name.  This will 
	 * also recursivily search all child nodes that have children until
	 * the named node can be found.  This seeks a child node that has
	 * children.
	 * @param name to search for.
	 * @return tree object found or NULL.
	 */ 
	NamedTree *find(const char *name) const;

	/**
	 * Find a subnode by a dot seperated list of node names.  If one or
	 * more lead dots are used, then the search will go through parent
	 * node levels of our node.  The dot seperated list could be thought
	 * of as a kind of pathname where dot is used like slash.  This implies
	 * that individual nodes can never use names which contain dot's if
	 * the path function will be used.
	 * @param path name string being sought.
	 * @return tree node object found at the path or NULL.
	 */
	NamedTree *path(const char *path) const;

	/**
	 * Find a child leaf node of our object with the specified name.  This
	 * will recursivily search all our child nodes until it can find a leaf
	 * node containing the specified id but that holds no further children.
	 * @param name of leaf node to search for.
	 * @return tree node object found or NULL.
	 */
	NamedTree *leaf(const char *name) const;

	/**
	 * Find a direct child of our node which matches the specified name.
	 * @param name of child node to find.
	 * @return tree node object of child or NULL.
	 */
	NamedTree *getChild(const char *tag) const;

	/**
	 * Find a direct leaf node on our node.  A leaf node is a node that has
	 * no children of it's own.  This does not perform a recursive search.
	 * @param name of leaf child node to find.
	 * @return tree node object of leaf or NULL.
	 */
	NamedTree *getLeaf(const char *tag) const;

	/**
	 * Get first child node in our ordered list of children.  This might
	 * be used to iterate child nodes.  This may also be used to get
	 * unamed child nodes.
	 * @return first child node or NULL if no children.
	 */
	inline NamedTree *getFirst(void) const
		{return static_cast<NamedTree *>(child.begin());};

	/**
	 * Get parent node we are listed as a child on.
	 * @return parent node or NULL if none.
	 */
	inline NamedTree *getParent(void) const
		{return parent;};

	/**
	 * Get the ordered index of our child nodes.
	 * @return ordered index of our children.
	 */
	inline OrderedIndex *getIndex(void) const
		{return const_cast<OrderedIndex*>(&child);};

	/**
	 * Test if this node has a name.
	 * @return true if name is set.
	 */
	inline operator bool() const
		{return (id != NULL);};

	/**
	 * Test if this node is unnamed.
	 * @return false if name is set.
	 */
	inline bool operator!() const
		{return (id == NULL);};

	/**
	 * Set or replace the name id of this node.  This will free the string
	 * if a name had already been set.
	 * @param name for this node to set.
	 */
	void setId(char *name);

	/**
	 * Remove our node from our parent list.  The name is set to NULL to
	 * keep delete from freeing the name string.
	 */
	void remove(void);

	/**
	 * Test if node has children.
	 * @return true if node contains child nodes.
	 */
	inline bool isLeaf(void) const
		{return (child.begin() == NULL);};
};

/**
 * A named hash map member object.  This is used to form a named object
 * which belongs to a specific hash map table.  This is a convienence
 * base class.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT NamedList : public NamedObject
{
protected:
	friend class objmap;

	NamedObject **keyroot;
	unsigned keysize;

	/**
	 * Create a named list member node.
	 * @param hash map table list belongs to.
	 * @param name of our member node.
	 * @param size of the map table used.
	 */
	NamedList(NamedObject **hash, char *id, unsigned size);

	/**
	 * Destroy list member.  Removes node from hash map table.
	 */
	virtual ~NamedList();

public:
	/**
	 * Remove node from hash map table.
	 */
	void delist(void);
};		

/**
 * A double linked list object.  This is used as a base class for objects
 * that will be organized through ordered double linked lists which allow
 * convenient insertion and deletion of list members anywhere in the list.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT LinkedList : public OrderedObject
{
protected:
	LinkedList *prev;
	OrderedIndex *root;

	/**
	 * Construct and add our object to an existing double linked list at end.
	 * @param index of linked list we are listed in.
	 */
	LinkedList(OrderedIndex *index);

	/**
	 * Construct an unlinked object.
	 */
	LinkedList();

	/**
	 * Delete linked list object.  If it is a member of a list of objects,
	 * then the list is reformed around us.
	 */
	virtual ~LinkedList();

public:
	/**
	 * Remove our object from the list it is currently part of.
	 */
	void delist(void);

	/**
	 * Attach our object to a new linked list though an ordered index.
	 * If we are already attached to a list we are delisted first.
	 * @param index of linked list we are joining.
	 */
	void enlist(OrderedIndex *index);

	/**
	 * Test if we are at the head of a list.
	 * @return true if we are the first node in a list.
	 */
	inline bool isHead(void) const
		{return root->head == (OrderedObject *)this;};

	/**
	 * Test if we are at the end of a list.
	 * @return true if we are the last node in a list.
	 */
	inline bool isTail(void) const
		{return root->tail == (OrderedObject *)this;};

	/**
	 * Get previous node in the list for reverse iteration.
	 * @return previous node in list.
	 */
	inline LinkedList *getPrev(void) const
		{return prev;};

	/**
	 * Get next node in the list when iterating.
	 * @return next node in list.
	 */
	inline LinkedList *getNext(void) const
		{return static_cast<LinkedList*>(LinkedObject::getNext());};
};
	
class __EXPORT objmap
{
protected:
	NamedList *object;

public:
	objmap(NamedList *obj);

	void operator++();

	objmap &operator=(NamedList *root);

	unsigned count(void) const;

	void begin(void) const;
};

template <class T, class O=NamedObject>
class named_value : public object_value<T, O>
{
public:
	inline named_value(LinkedObject **root, char *i) 
		{LinkedObject::enlist(root); O::id = i;};

	inline void operator=(T v)
		{set(v);};

	inline static named_value find(named_value *base, const char *id)
		{return static_cast<named_value *>(NamedObject::find(base, id));};
};

template <class T, class O=OrderedObject>
class linked_value : public object_value<T, O>
{
public:
	inline linked_value() {};

	inline linked_value(LinkedObject **root)
		{LinkedObject::enlist(root);};

	inline linked_value(OrderedIndex *index) 
		{O::enlist(index);};

	inline linked_value(LinkedObject **root, T v) 
		{LinkedObject::enlist(root); set(v);};

	inline linked_value(OrderedIndex *index, T v)
 		{O::enlist(index); set(v);};

	inline void operator=(T v)
		{set(v);};
};	

template <class T>
class linked_pointer
{
private:
	T *ptr;

public:
	inline linked_pointer(T *p)
		{ptr = p;};

	inline linked_pointer(const linked_pointer &p)
		{ptr = p.ptr;};

	inline linked_pointer(LinkedObject *p)
		{ptr = static_cast<T*>(p);};

	inline linked_pointer(OrderedIndex *a)
		{ptr = static_cast<T*>(a->begin());};

	inline linked_pointer() 
		{ptr = NULL;};

	inline void operator=(T *v)
		{ptr = v;};

	inline void operator=(linked_pointer &p)
		{ptr = p.ptr;};

	inline void operator=(OrderedIndex *a)
		{ptr = static_cast<T*>(a->begin());};

	inline void operator=(LinkedObject *p)
		{ptr = static_cast<T*>(p);};

	inline T* operator->() const
		{return ptr;};

	inline T* operator*() const
		{return ptr;};

	inline operator T*() const
		{return ptr;};

	inline void prev(void)
		{ptr = static_cast<T*>(ptr->getPrev());};

	inline void next(void)
		{ptr = static_cast<T*>(ptr->getNext());};

	inline T *getNext(void) const
		{return static_cast<T*>(ptr->getNext());};

    inline T *getPrev(void) const
        {return static_cast<T*>(ptr->getPrev());};

	inline void operator++()
		{ptr = static_cast<T*>(ptr->getNext());};

    inline void operator--()
        {ptr = static_cast<T*>(ptr->getPrev());};

	inline bool isNext(void) const
		{return (ptr->getNext() != NULL);};

	inline bool isPrev(void) const
		{return (ptr->getPrev() != NULL);};

	inline operator bool() const
		{return (ptr != NULL);};

	inline bool operator!() const
		{return (ptr == NULL);};

    inline LinkedObject **root(void) const
		{T **r = &ptr; return (LinkedObject**)r;};
};

template <class T>
class treemap : public NamedTree
{
protected:
	T value;

public:
	inline treemap(char *id = NULL) : NamedTree(id) {};
	inline treemap(treemap *parent, char *id) : NamedTree(parent, id) {};
	inline treemap(treemap *parent, char *id, T &v) :
		NamedTree(parent, id) {value = v;};

	inline T &get(void) const
		{return value;};

	inline T operator*() const
		{return value;};

	static inline T getPointer(treemap *node)
		{(node == NULL) ? NULL : node->value;};

	inline bool isAttribute(void) const
		{return (!child.begin() && value != NULL);};

	inline T getPointer(void) const
		{return value;};

	inline T getData(void) const
		{return value;};

	inline void setPointer(const T p)
		{value = p;};

	inline void set(const T &v)
		{value = v;};

	inline void operator=(const T &v)
		{value = v;};

	inline treemap *getParent(void) const
		{return static_cast<treemap*>(parent);};

	inline treemap *getChild(const char *id) const
		{return static_cast<treemap*>(NamedTree::getChild(id));};

	inline treemap *getLeaf(const char *id) const
		{return static_cast<treemap*>(NamedTree::getLeaf(id));};

	inline T getValue(const char *id) const
		{return getPointer(getLeaf(id));};

	inline treemap *find(const char *id) const
		{return static_cast<treemap*>(NamedTree::find(id));};

	inline treemap *path(const char *path) const
		{return static_cast<treemap*>(NamedTree::path(path));};

	inline treemap *leaf(const char *id) const
		{return static_cast<treemap*>(NamedTree::leaf(id));};

	inline treemap *getFirst(void) const
		{return static_cast<treemap*>(NamedTree::getFirst());};
};

template <class T, unsigned M = 177>
class keymap
{
private:
	NamedObject *idx[M];

public:
	inline ~keymap()
		{NamedObject::purge(idx, M);};

	inline NamedObject **root(void) const
		{return idx;};

	inline unsigned limit(void) const
		{return M;};

	inline T *get(const char *id) const
		{return static_cast<T*>(NamedObject::map(idx, id, M));};

	inline T *begin(void) const
		{return static_cast<T*>(NamedObject::skip(idx, NULL, M));};

	inline T *next(T *current) const
		{return static_cast<T*>(NamedObject::skip(idx, current, M));};

	inline unsigned count(void) const
		{return NamedObject::count(idx, M);};

	inline T **index(void) const
		{return NamedObject::index(idx, M);};

	inline T **sort(void) const
		{return NamedObject::sort(NamedObject::index(idx, M));};
}; 

template <class T>
class keylist : public OrderedIndex
{
public:
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

/**
 * Convenience typedef for root pointers of single linked lists.
 */
typedef	LinkedObject *LinkedIndex;

END_NAMESPACE

#endif
