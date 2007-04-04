#ifndef _UCOMMON_COUNTER_H_
#define	_UCOMMON_COUNTER_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT counter
{
private:
	unsigned value, cycle;

public:
	counter();

	counter(unsigned limit);

	unsigned get(void);

	inline unsigned range(void)
		{return cycle;};

	inline unsigned operator*()
		{return get();};

	inline operator unsigned()
		{return get();};

	void operator=(unsigned v);
};

class __EXPORT SeqCounter : public counter
{
private:
	void *item;
	size_t offset;
	
protected:
	SeqCounter(void *start, size_t size, unsigned count);

	void *get(void);

	void *get(unsigned idx);

public:
	inline void operator=(unsigned v)
		{counter::operator=(v);};
};
	
class __EXPORT toggle
{
private:
	bool value;

public:
	inline toggle()
		{value = false;};

	bool get(void);

	inline bool operator*()
		{return get();};

	inline void operator=(bool v)
		{value = v;};

	inline operator bool()
		{return get();};

};

template <class T>
class sequence : public SeqCounter
{
protected:
	inline T *get(unsigned idx)
		{return static_cast<T *>(SeqCounter::get(idx));};

public:
	inline sequence(T *list, unsigned max) : 
		SeqCounter(list, sizeof(T), max) {};

	inline T* get(void)
		{return static_cast<T *>(SeqCounter::get());};

	inline T& operator*()
		{return *get();};

	inline operator T&()
		{return *get();};

	inline T& operator[](unsigned idx)
		{return *get(idx);};
};

END_NAMESPACE

#endif
