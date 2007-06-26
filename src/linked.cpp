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

#include <config.h>
#include <ucommon/linked.h>
#include <ucommon/string.h>

using namespace UCOMMON_NAMESPACE;

LinkedObject *LinkedObject::nil = (LinkedObject *)NULL;
LinkedObject *LinkedObject::inv = (LinkedObject *)-1;

LinkedObject::LinkedObject(LinkedObject **root)
{
	if(root)
		enlist(root);
}

LinkedObject::~LinkedObject()
{
}

void LinkedObject::purge(LinkedObject *root)
{
	LinkedObject *next;

	while(root) {
		next = root->next;
		root->release();
		root = next;
	}
}

bool LinkedObject::isMember(LinkedObject *list) const
{
	while(list) {
		if(list == this)
			return true;
		list = list->next;
	}
	return false;
}

void LinkedObject::enlist(LinkedObject **root)
{
	next = *root;
	*root = this;
}

void LinkedObject::delist(LinkedObject **root)
{
	LinkedObject *prev = NULL, *node = *root;

	while(node && node != this) {
		prev = node;
		node = node->next;
	}

	if(!node)
		return;

	if(!prev)
		*root = next;
	else
		prev->next = next;
}

void ReusableObject::release(void)
{
	next = (LinkedObject *)nil;
}

NamedObject::NamedObject() :
OrderedObject()
{
	id = NULL;
}

NamedObject::NamedObject(OrderedIndex *root, char *nid) :
OrderedObject()
{
	NamedObject *node = static_cast<NamedObject*>(root->head), *prev = NULL;

	while(node) {
		if(node->compare(nid)) {
			if(prev) 
				prev->next = node->getNext();
			else
				root->head = node->getNext();
			node->release();
			break;
		}
		prev = node;
		node = node->getNext();
	}	
	next = NULL;							
	id = nid;
	if(!root->head)
		root->head = this;
	if(!root->tail)
		root->tail = this;
	else
		root->tail->next = this;
}

NamedObject::NamedObject(NamedObject **root, char *nid, unsigned max) :
OrderedObject()
{
	NamedObject *node, *prev = NULL;

	if(max < 2)
		max = 1;
	else
		max = keyindex(nid, max);

	node = root[max];
	while(node) {
		if(node && node->compare(nid)) {
			if(prev) {
				prev->next = this;
				next = node->next;
			}
			else
				root[max] = node->getNext();
			node->release();
			break;
		}
		prev = node;
		node = node->getNext();
	}		

	if(!prev) {
		next = root[max];
		root[max] = this;
	}
	id = nid;
}

NamedObject::~NamedObject()
{
	if(id) {
		free(id);
		id = NULL;
	}
}

NamedList::NamedList(NamedObject **root, char *id, unsigned max) :
NamedObject(root, id, max)
{
	keyroot = root;
	keysize = max;
}

NamedList::~NamedList()
{
	delist();
}

void NamedList::delist(void)
{
	if(!keyroot)
		return;
	
	LinkedObject::delist((LinkedObject**)(&keyroot[keyindex(id, keysize)]));	
	keyroot = NULL;
}

void LinkedObject::retain(void)
{
	next = this;
}

void LinkedObject::release(void)
{
	if(next != this) {
		next = this;
		delete this;
	}
}

unsigned LinkedObject::count(LinkedObject *root)
{
	unsigned c = 0;
	while(root) {
		++c;
		root = root->next;
	}
	return c;
}

unsigned NamedObject::keyindex(const char *id, unsigned max)
{
	unsigned val = 0;

	while(*id)
		val = (val << 1) ^ (*(id++) & 0x1f);

	return val % max;
}

bool NamedObject::compare(const char *cid) const
{
	if(!strcmp(id, cid))
		return true;

	return false;
}

extern "C" {

	static int ncompare(const void *o1, const void *o2)
	{
		const NamedObject * const *n1 = static_cast<const NamedObject * const*>(o1);
		const NamedObject * const*n2 = static_cast<const NamedObject * const*>(o2);
		return (*n1)->compare((*n2)->getId());
	}
};

NamedObject **NamedObject::sort(NamedObject **list, size_t count)
{
	if(!count) {
		while(list[count])
			++count;
	}
	qsort(static_cast<void *>(list), count, sizeof(NamedObject *), &ncompare);
	return list;
}

NamedObject **NamedObject::index(NamedObject **idx, unsigned max)
{
	NamedObject **op = new NamedObject *[count(idx, max) + 1];
	unsigned pos = 0;
	NamedObject *node = skip(idx, NULL, max);

	while(node) {
		op[pos++] = node;
		node = skip(idx, node, max);
	}
	op[pos] = NULL;
	return op;
}

NamedObject *NamedObject::skip(NamedObject **idx, NamedObject *rec, unsigned max)
{
	unsigned key = 0;
	if(rec && !rec->next) 
		key = keyindex(rec->id, max) + 1;
		
	if(!rec || !rec->next) {
		while(key < max && !idx[key])
			++key;
		if(key >= max)
			return NULL;
		return idx[key];
	}

	return rec->getNext();
}

void NamedObject::purge(NamedObject **idx, unsigned max)
{
	LinkedObject *root;

	if(max < 2)
		max = 1;

	while(max--) {
		root = idx[max];
		LinkedObject::purge(root);
	}
}

unsigned NamedObject::count(NamedObject **idx, unsigned max)
{
	unsigned count = 0;
	LinkedObject *node;

	if(max < 2)
		max = 1;

	while(max--) {
		node = idx[max];
		while(node) {
			++count;
			node = node->next;
		}
	}
	return count;
}

NamedObject *NamedObject::map(NamedObject **idx, const char *id, unsigned max)
{
	if(max < 2)
		return find(*idx, id);

	return find(idx[keyindex(id, max)], id);
}

NamedObject *NamedObject::find(NamedObject *root, const char *id)
{
	while(root)
	{
		if(root->compare(id))
			break;
		root = root->getNext();
	}
	return root;
}

NamedTree::NamedTree(char *nid) :
NamedObject(), child()
{
	id = nid;
	parent = NULL;
}

NamedTree::NamedTree(NamedTree *p, char *nid) :
NamedObject(), child()
{
	enlist(&p->child);
	id = nid;
	parent = p;
}

NamedTree::~NamedTree()
{
	purge();
}

NamedTree *NamedTree::getChild(const char *tid) const
{
	linked_pointer<NamedTree> node = child.begin();
	
	while(node) {
		if(!strcmp(node->id, tid))
			return *node;
		node.next();
	}
	return NULL;
}

NamedTree *NamedTree::path(const char *tid) const
{
	const char *np;
	char buf[65];
	char *ep;
	NamedTree *node = const_cast<NamedTree*>(this);

	if(!tid || !*tid)
		return const_cast<NamedTree*>(this);

	while(*tid == '.') {
		if(!node->parent)
			return NULL;
		node = node->parent;

		++tid;
	}
		
	while(tid && *tid && node) {
		string::set(buf, sizeof(buf), tid);
		ep = strchr(buf, '.');
		if(ep)
			*ep = 0;
		np = strchr(tid, '.');
		if(np)
			tid = ++np;
		else
			tid = NULL;
		node = node->getChild(buf);
	}
	return node;
}

NamedTree *NamedTree::getLeaf(const char *tid) const
{
    linked_pointer<NamedTree> node = child.begin();

    while(node) {
        if(node->isLeaf() && !strcmp(node->id, tid))
            return *node;
        node.next();
    }
    return NULL;
}

NamedTree *NamedTree::leaf(const char *tid) const
{
    linked_pointer<NamedTree> node = child.begin();
    NamedTree *obj;

	while(node) {
		if(node->isLeaf() && !strcmp(node->id, tid))
			return *node;
		obj = NULL;
		if(!node->isLeaf())
			obj = node->leaf(tid);
		if(obj)
			return obj;
		node.next();
	}
	return NULL;
}

NamedTree *NamedTree::find(const char *tid) const
{
	linked_pointer<NamedTree> node = child.begin();
	NamedTree *obj;

	while(node) {
		if(!node->isLeaf()) {
			if(!strcmp(node->id, tid))
				return *node;
			obj = node->find(tid);
			if(obj)
				return obj;
		}
		node.next();
	}
	return NULL;
}

void NamedTree::setId(char *nid)
{
	if(id)
		free(id);

	id = nid;
}

void NamedTree::remove(void)
{
	if(parent)
		delist(&parent->child);

	id = NULL;
}

void NamedTree::purge(void)
{
	linked_pointer<NamedTree> node = child.begin();
	NamedTree *obj;

	if(parent)
		delist(&parent->child);

	while(node) {
		obj = *node;
		obj->parent = NULL;	// save processing
		node.next();
		delete obj;
	}

	if(id) {
		free(id);
		id = NULL;
	}
}

LinkedObject::LinkedObject()
{
	next = 0;
}

OrderedObject::OrderedObject() : LinkedObject()
{
}

OrderedObject::OrderedObject(OrderedIndex *root) :
LinkedObject()
{
	if(root)
		enlist(root);
}

void OrderedObject::delist(OrderedIndex *root)
{
    OrderedObject *prev = NULL, *node;

	node = root->head;

    while(node && node != this) {
        prev = node;
		node = node->getNext();
    }

    if(!node) 
        return;

    if(!prev)
		root->head = getNext();
    else
        prev->next = next;

	if(this == root->tail)
		root->tail = prev;
}	

void OrderedObject::enlist(OrderedIndex *root)
{
	if(root->head == NULL)
		root->head = this;
	
	if(root->tail)
		root->tail->next = this;

	root->tail = this;
}

LinkedList::LinkedList()
{
	root = 0;
	prev = 0;
	next = 0;
}

LinkedList::LinkedList(OrderedIndex *r)
{
	root = NULL;
	next = prev = 0;
	if(r)
		enlist(r);
}

void LinkedList::enlist(OrderedIndex *r)
{
	if(root)
		delist();
	root = r;
	prev = 0;
	next = 0;

	if(!root->head) {
		root->head = root->tail = static_cast<OrderedObject *>(this);
		return;
	}

	prev = static_cast<LinkedList *>(root->tail);
	prev->next = this;
	root->tail = static_cast<OrderedObject *>(this);
}

void LinkedList::delist(void)
{
	if(!root)
		return;

	if(prev)
		prev->next = next;
	else if(root->head == static_cast<OrderedObject *>(this))
		root->head = static_cast<OrderedObject *>(next);

	if(next)
		(static_cast<LinkedList *>(next))->prev = prev;
	else if(root->tail == static_cast<OrderedObject *>(this))
		root->tail = static_cast<OrderedObject *>(prev);

	root = 0;
	next = prev = 0;
}

LinkedList::~LinkedList()
{
	delist();
}

OrderedIndex::OrderedIndex()
{
	head = tail = 0;
}

OrderedIndex::~OrderedIndex()
{
	head = tail = 0;
}

void OrderedIndex::purge(void)
{
	if(head) {
		LinkedObject::purge((LinkedObject *)head);
		head = tail = 0;
	}
}

void OrderedIndex::lock_index(void)
{
}

void OrderedIndex::unlock_index(void)
{
}

LinkedObject **OrderedIndex::index(void) const
{
	LinkedObject **op = new LinkedObject *[count() + 1];
	LinkedObject *node;
	unsigned idx = 0;

	node = head;
	while(node) {
		op[idx++] = node;
		node = node->next;
	}
	op[idx] = NULL;
	return op;
}

LinkedObject *OrderedIndex::find(unsigned index) const
{
	unsigned count = 0;
	LinkedObject *node;

	node = head;

	while(node && ++count < index)
		node = node->next;

	return node;
}

unsigned OrderedIndex::count(void) const
{
	unsigned count = 0;
	LinkedObject *node;

	node = head;

	while(node) {
		node = node->next;
		++count;
	}
	return count;
}

objmap::objmap(NamedList *o)
{
	object = o;
}

void objmap::operator++()
{
	if(object)
		object = static_cast<NamedList *>(NamedObject::skip(object->keyroot, 
			static_cast<NamedObject*>(object), object->keysize));
}

