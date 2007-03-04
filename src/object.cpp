#include <private.h>
#include <stdlib.h>
#include <string.h>
#include <inc/object.h>

#if UCOMMON_THREADING > 0
#include <pthread.h>
#endif

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

auto_release::auto_release(Object *o)
{
	o->retain();
	object = o;
}

auto_release::auto_release()
{
	object = 0;
}

void auto_release::release(void)
{
	if(object)
		object->release();
	object = 0;
}

auto_release::~auto_release()
{
	release();
}

auto_release::auto_release(const auto_release &from)
{
	object = from.object;
	if(object)
		object->retain();
}

bool auto_release::operator!() const
{
	return object == 0;
}

bool auto_release::operator==(Object *o) const
{
	return object == o;
}

bool auto_release::operator!=(Object *o) const
{
	return object != o;
}

auto_release &auto_release::operator=(Object *o)
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

#if UCOMMON_THREADING > 0

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

void AutoObject::base_exit::set(AutoObject *ex)
{
	pthread_setspecific(exit_key, ex);
}

#else
AutoObject::base_exit::base_exit()
{
}

AutoObject::base_exit::~base_exit()
{
}

AutoObject *AutoObject::base_exit::get(void)
{
	return NULL;
}

void AutoObject::base_exit::set(AutoObject *ex)
{
}

#endif

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

static ExitObject *exit_list = NULL;

extern "C" {

	static void exit_release(void)
	{
		while(exit_list) {
			exit_list->release();
			exit_list = exit_list->next;
		}
	}

#if UCOMMON_THREADING > 0
	static void fork_release(void)
	{
    	while(exit_list) {
        	exit_list->child();
        	exit_list = exit_list->next;
    	}
	}

    static void fork_retain(void)
    {
		ExitObject *obj = exit_list;
        while(obj) {
            obj->parent();
            obj = obj->next;
        }
    }

    static void fork_prepare(void)
    {
        ExitObject *obj = exit_list;
        while(obj) {
            obj->prepare();
            obj = obj->next;
        }
    }

#endif

} // c

void ExitObject::forkPrepare(void)
{
#ifndef	HAVE_PTHREAD_ATFORK
	fork_prepare();	
#endif
}

void ExitObject::forkRelease(void)
{
#ifndef	HAVE_PTHREAD_ATFORK
	fork_release();
#endif
}

void ExitObject::forkRetain(void)
{
#ifndef	HAVE_PTHREAD_ATFORK
	fork_retain();
#endif
}

void ExitObject::child(void)
{
	release();
}

void ExitObject::parent(void)
{
}

void ExitObject::prepare(void)
{
}

void ExitObject::delist(void)
{
    while(exit_list) {
        if(exit_list == this) {
            exit_list = next;
            return;
        }
        exit_list = exit_list->next;
    }
}

ExitObject::ExitObject()
{
	static bool installed = false;

	if(!installed) {
#if	UCOMMON_THREADING > 0 && defined(HAVE_PTHREAD_ATFORK)
		pthread_atfork(&fork_prepare, &fork_retain, &fork_release);
#endif
		atexit(&exit_release);
		installed = true;
	}

	next = exit_list;
	exit_list = this;
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

