#include <config.h>
#include <ucommon/linked.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

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

bool LinkedObject::isMember(LinkedObject *list)
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

NamedObject::NamedObject(OrderedIndex *root, const char *nid) :
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

NamedObject::NamedObject(NamedObject **root, const char *nid, unsigned max) :
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

NamedList::NamedList(NamedObject **root, const char *id, unsigned max) :
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

void LinkedObject::release(void)
{
	if(next != this) {
		next = this;
		delete this;
	}
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
    OrderedObject *prev = NULL, *node = root->head;

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

LinkedObject **OrderedIndex::index(void)
{
	LinkedObject **op = new LinkedObject *[count() + 1];
	LinkedObject *node = head;
	unsigned idx = 0;

	while(node) {
		op[idx++] = node;
		node = node->next;
	}
	op[idx] = NULL;
	return op;
}

LinkedObject *OrderedIndex::find(unsigned index)
{
	unsigned count = 0;
	LinkedObject *node = head;

	while(node && ++count < index)
		node = node->next;
	
	return node;
}

unsigned OrderedIndex::count(void)
{
	unsigned count = 0;
	LinkedObject *node = head;

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

