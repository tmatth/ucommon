#include <config.h>
#include <ucommon/object.h>
#include <stdlib.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

AutoObject::base_exit AutoObject::ex;

CountedObject::CountedObject()
{
	count = 0;
}

CountedObject::CountedObject(const Object &source)
{
	count = 0;
}

Object::~Object()
{
}

void Object::release(void)
{
	delete this;
}

void Object::retain(void)
{
}

Object *Object::copy(void)
{
	retain();
	return this;
}

Temporary::~Temporary()
{
}

void CountedObject::dealloc(void)
{
	delete this;
}

void CountedObject::retain(void)
{
	++count;
}

void CountedObject::release(void)
{
	if(count > 1) {
		--count;
		return;
	}
	dealloc();
}

auto_delete::~auto_delete()
{
	if(object)
		delete object;

	object = 0;
}

auto_pointer::auto_pointer(Object *o)
{
	o->retain();
	object = o;
}

auto_pointer::auto_pointer()
{
	object = 0;
}

void auto_pointer::release(void)
{
	if(object)
		object->release();
	object = 0;
}

auto_pointer::~auto_pointer()
{
	release();
}

auto_pointer::auto_pointer(const auto_pointer &from)
{
	object = from.object;
	if(object)
		object->retain();
}

bool auto_pointer::operator!() const
{
	return (object == 0);
}

auto_pointer::operator bool() const
{
    return (object != 0);
}

bool auto_pointer::operator==(Object *o) const
{
	return object == o;
}

bool auto_pointer::operator!=(Object *o) const
{
	return object != o;
}

auto_pointer &auto_pointer::operator=(Object *o)
{
	if(object == o)
		return *this;

	if(o)
		o->retain();
	if(object)
		object->release();
	object = o;
	return *this;
}

static pthread_key_t exit_key;

extern "C" {
	static void exit_handler(void *obj)
	{
		AutoObject *node = (AutoObject *)obj;
		AutoObject *next;
		while(node) {
			next = node->next;
			node->release();
			node = next;
		}
	}
}

AutoObject::base_exit::base_exit()
{
	pthread_key_create(&exit_key, &exit_handler);
}

AutoObject::base_exit::~base_exit()
{
	AutoObject *obj = get();
	set(NULL);
	if(obj)
		exit_handler(obj);
}

AutoObject *AutoObject::base_exit::get(void)
{
	return (AutoObject *)pthread_getspecific(exit_key);
}

void AutoObject::base_exit::set(AutoObject *ao)
{
	pthread_setspecific(exit_key, ao);
}

AutoObject::AutoObject()
{
	next = ex.get();
	ex.set(this);
}

AutoObject::~AutoObject()
{
	delist();
}

// We assume auto for heap object by default, hence release deletes...

void AutoObject::release(void)
{
	delete this;
}

// this protects in case objects are deleted or auto-released from a
// stack frame out of reverse order, otherwise it peals off the objects
// in reverse order that they were created...

void AutoObject::delist(void)
{
	AutoObject *node = ex.get();
	while(node) {
		if(node == this) {
			ex.set(next);
			return;
		}
		node = node->next;
	}
}

sparse_array::sparse_array(unsigned m)
{
	max = m;
	vector = new Object*[m];
	memset(vector, 0, sizeof(Object *) * m);
}

sparse_array::~sparse_array()
{
	purge();
}

void sparse_array::purge(void)
{
	if(!vector)
		return;

	for(unsigned pos = 0; pos < max; ++ pos) {
		if(vector[pos])
			vector[pos]->release();
	}
	delete[] vector;
	vector = NULL; 
}

unsigned sparse_array::count(void)
{
	unsigned c = 0;
	for(unsigned pos = 0; pos < max; ++pos) {
		if(vector[pos])
			++c;
	}
	return c;
}
 
Object *sparse_array::get(unsigned pos)
{
	Object *obj;

	if(pos >= max)
		return NULL;

	if(!vector[pos]) {
		obj = create();
		if(!obj)
			return NULL;
		obj->retain();
		vector[pos] = obj;
	}
	return vector[pos];
}


