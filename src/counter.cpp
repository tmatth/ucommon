#include <private.h>
#include <inc/config.h>
#include <inc/object.h>
#include <inc/counter.h>
#include <stdlib.h>
#include <unistd.h>

using namespace UCOMMON_NAMESPACE;

counter::counter()
{
	cycle = value = 0;
}

counter::counter(unsigned max)
{
	cycle = max;
	value = 0;
}

void counter::operator=(unsigned v)
{
	if(!cycle || v < cycle)
		value = v;
}

unsigned counter::get(void)
{
	unsigned v = value++;
	if(cycle && value >= cycle)
		value = 0;
	return v;
}

SeqCounter::SeqCounter(void *base, size_t size, unsigned limit) :
counter(limit)
{
	item = base;
	offset = size;
}

void *SeqCounter::get(void)
{
	unsigned pos = counter::get();
	return (caddr_t)item + (pos * offset);
}

void *SeqCounter::get(unsigned pos)
{
	if(pos >= max())
		return NULL;

	return (caddr_t)item + (pos * offset);
}

bool toggle::get(void)
{
	bool v = value;
	if(value)
		value = false;
	else
		value = true;

	return v;
}

	
