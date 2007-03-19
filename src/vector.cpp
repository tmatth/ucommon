#include <private.h>
#include <ucommon/vector.h>
#include <string.h>
#include <stdarg.h>

using namespace UCOMMON_NAMESPACE;

const vectorsize_t Vector::npos = (vectorsize_t)(-1);

Vector::array::array(vectorsize_t size)
{
	max = size;
	len = 0;
	list[0] = 0;
}

void Vector::array::dealloc(void)
{
	purge();
	CountedObject::dealloc();
}

void Vector::array::purge(void)
{
	vectorsize_t pos = 0;
	while(list[pos])
		list[pos++]->release();
	len = 0;
	list[0] = 0;
}

void Vector::array::dec(vectorsize_t offset)
{
	if(!len)
		return;

	if(offset >= len) {
		purge();
		return;
	}

	while(offset--) {
		list[--len]->release();
		list[len] = 0;
	}
}

void Vector::array::inc(vectorsize_t offset)
{
	if(!offset)
		++offset;

	if(offset >= len) {
		purge();
		return;
	}

	if(!len)
		return;

	for(vectorsize_t pos = 0; pos < offset; ++pos)
		list[pos]->release();

	memmove(list, &list[offset], (len - offset) * sizeof(void *));
	len -= offset;
	list[len] = 0;
}

void Vector::array::set(Object **items)
{
	purge();
	add(items);
}

void Vector::array::add(Object **items)
{
	vectorsize_t size = cpr_vectorsize((void **)(items));

	if(!size)
		return;

	if(len + size > max)
		size = max - len;

	if(size < 1)
		return;

	for(vectorsize_t pos = 0; pos < size; ++pos) {
		list[len++] = items[pos];
		items[pos]->retain();
	}
	list[len] = 0;
}

void Vector::array::add(Object *obj)
{
	if(!obj)
		return;

	if(len == max)
		return;

	obj->retain();
	list[len++] = obj;	
	list[len] = 0;
}

vectorsize_t Vector::size(void) const
{
    if(!data)
        return 0;

    return data->max;
}

vectorsize_t Vector::len(void) const
{
	if(!data)
		return 0;

	return data->len;
}

Vector::Vector()
{
	data = NULL;
}

Vector::Vector(Object **items, vectorsize_t limit)
{
	if(!items) {
		data = NULL;
		return;
	}

	if(!limit)
		limit = cpr_vectorsize((void **)items);

	data = create(limit);
	data->retain();
	data->set(items);
}		

Vector::Vector(vectorsize_t size) 
{
	data = create(size);
	data->retain();
};

Vector::~Vector()
{
	release();
}

Vector::array *Vector::copy(void) const
{
	return data;
}

Object **Vector::list(void) const
{
	if(!data)
		return NULL;

	return data->list;
}

Object *Vector::get(int offset) const
{
	if(!data || !data->len)
		return NULL;

	if(offset >= (int)(data->len))
		return NULL;

	if(((vectorsize_t)(-offset)) >= data->len)
		return NULL;

	if(offset >= 0)
		return data->list[offset];

	return data->list[data->len + offset];
}

vectorsize_t Vector::get(void **target, vectorsize_t limit) const
{
	vectorsize_t pos;
	if(!data) {
		target[0] = 0;
		return 0;
	}

	for(pos = 0; pos < data->len && pos < limit - 1; ++pos)
		target[pos] = data->list[pos];
	target[pos] = 0;
	return pos;
}

Vector::array *Vector::create(vectorsize_t size) const
{
	return new((size_t)size) array(size);
}

void Vector::release(void)
{
	if(data)
		data->release();
	data = NULL;
}

Object *Vector::begin(void) const
{
	if(!data)
		return NULL;

	return data->list[0];
}

Object *Vector::end(void) const
{
	if(!data || !data->len)
		return NULL;

	return data->list[data->len - 1];
}

vectorsize_t Vector::find(Object *obj, vectorsize_t pos) const
{
	if(!data)
		return npos;

	while(pos < data->len) {
		if(data->list[pos] == obj)
			return pos;
		++pos;
	}
	return npos;
}

void Vector::split(vectorsize_t pos)
{
	if(!data || pos >= data->len)
		return;

	while(data->len > pos) {
		--data->len;
		data->list[data->len]->release();
	}
	data->list[data->len] = NULL;
}

void Vector::rsplit(vectorsize_t pos)
{
	vectorsize_t head = 0;

	if(!data || pos >= data->len || !pos)
		return;

	while(head < pos)
		data->list[head++]->release();

	head = 0;
	while(data->list[pos])
		data->list[head++] = data->list[pos++];

	data->len = head;
	data->list[head] = NULL;
}

void Vector::set(Object **list)
{
	if(!data && list) {
		data = create(cpr_vectorsize((void **)list));
		data->retain();
	}
	if(data && list)
		data->set(list);
}

void Vector::set(vectorsize_t pos, Object *obj)
{
	if(!data || pos > data->len)
		return;

	if(pos == data->len && data->len < data->max) {
		data->list[data->len++] = obj;
		data->list[data->len] = NULL;
		obj->retain();
		return;
	}
	data->list[pos]->release();
	data->list[pos] = obj;
	obj->retain();
}

void Vector::add(Object **list)
{
	if(data && list)
		data->add(list);
}

void Vector::add(Object *obj)
{
	if(data && obj)
		data->add(obj);
}

void Vector::clear(void)
{
	if(data)
		data->purge();
}

bool Vector::resize(vectorsize_t size)
{
	if(!size) {
		release();
		data = NULL;
		return true;
	}

	if(data->isCopied() || data->max < size) {
		data->release();
		data = create(size);
		data->retain();
	}
	return true;
}

void Vector::cow(vectorsize_t size)
{
	if(data) {
		size += data->len;
	}

	if(!size)
		return;

	if(!data || !data->max || data->isCopied() || size > data->max) {
		array *a = create(size);
		a->len = data->len;
		memcpy(a->list, data->list, data->len * sizeof(Object *));
		a->list[a->len] = 0;
		a->retain();
		data->release();
		data = a;
	}
}

void Vector::operator^=(Vector &v)
{
	release();
	set(v.list());
}	

Vector &Vector::operator^(Vector &v)
{
	vectorsize_t vs = v.len();

	if(!vs)
		return *this;

	if(data && data->len + vs > data->max)
		cow();

	add(v.list());
	return *this;
}

void Vector::operator++()
{
	if(!data)
		return;

	data->inc(1);
}

void Vector::operator--()
{
	if(!data)
		return;

	data->dec(1);
}

void Vector::operator+=(vectorsize_t inc)
{
	if(!data)
		return;

	data->inc(inc);
}

void Vector::operator-=(vectorsize_t dec)
{
    if(!data)
        return;

    data->inc(dec);
}

MemVector::MemVector(void *mem, vectorsize_t size)
{
	data = new((caddr_t)mem) array(size);
}

MemVector::~MemVector()
{
	data = NULL;
}

void MemVector::release(void)
{
	data = NULL;
}

bool MemVector::resize(vectorsize_t size)
{
	return false;
}

void MemVector::cow(vectorsize_t adj)
{
}

Vector::array *MemVector::copy(void) const
{
	array *tmp = create(data->max);
	tmp->set(data->list);
	return tmp;
}
	
extern "C" vectorsize_t cpr_vectorsize(void **list)
{
	vectorsize_t pos = 0;
	while(list[pos])
		++pos;
	return pos;
}
