#ifndef	_UCOMMON_OBJECT_H_
#define	_UCOMMON_OBJECT_H_

#ifndef	_UCOMMON_CONFIG_H_
#include <ucommon/config.h>
#endif

#include <stdlib.h>

NAMESPACE_UCOMMON

class __EXPORT Object
{
public:
	virtual void retain(void);
	virtual void release(void);
	virtual ~Object();

	Object *copy(void);
};

class __EXPORT CountedObject : public Object
{
private:
	volatile unsigned count;

protected:
	CountedObject();
	CountedObject(const Object &ref);
	virtual void dealloc(void);

public:
	inline bool isCopied(void)
		{return count > 1;};

	void retain(void);
	void release(void);
};

class __EXPORT Temporary
{
protected:
	friend class auto_delete;
	virtual ~Temporary();
};

class __EXPORT ExitObject
{
protected:
	ExitObject();
	virtual ~ExitObject();

	void delist(void);

public:
	ExitObject *next;
	
	static void forkPrepare(void);
	static void forkRelease(void);
	static void forkRetain(void);

	virtual void release(void) = 0;
	virtual void prepare(void);
	virtual void parent(void);
	virtual void child(void);
};	

class __EXPORT AutoObject
{
protected:
	class __EXPORT base_exit
	{
	public:
		base_exit();
		~base_exit();
		void set(AutoObject *obj);
		AutoObject *get(void);
	};

	static base_exit ex;

	AutoObject();
	virtual ~AutoObject();

	void delist(void);

public:
	AutoObject *next;

	virtual void release(void);
};

class __EXPORT auto_delete
{
protected:
	Temporary *object;

public:
	~auto_delete();
};

class __EXPORT auto_release
{
protected:
	Object *object;
	
	auto_release();

public:
	auto_release(Object *o);
	auto_release(const auto_release &from);
	~auto_release();

	void release(void);

	bool operator!() const;

	operator bool() const;

	bool operator==(Object *o) const;

	bool operator!=(Object *o) const;

	auto_release &operator=(Object *x);
};	

class __EXPORT sparse_array
{
private:
	Object **vector;
	unsigned max;

protected:
	virtual Object *create(void) = 0;

	void purge(void);

	Object *get(unsigned idx);

	sparse_array(unsigned limit);

public:
	virtual ~sparse_array();

	unsigned count(void);
};
	
template <class T, unsigned S>
class sarray : public sparse_array
{
public:
	inline sarray() : sparse_array(S) {};

	inline T *get(unsigned idx)
		{static_cast<T*>(sparse_array::get(idx));};

	inline T *operator[](unsigned idx)
		{return get(idx);};

private:
	Object *create(void)
		{return new T;};
};

template <class T>
class temporary : public auto_delete
{
public:
	inline temporary() 
		{object = new T;};

	inline T& operator*() const
		{return *(static_cast<T*>(object));};

	inline T* operator->() const
		{return static_cast<T*>(object);};
};

template <class T, class O = CountedObject>
class value : public O
{
protected:
	T _value;

public:
	inline value() : O() {};

	inline T& operator*()
		{return _value;};

	inline T &operator=(const T &v)
		{_value = v; return _value;};

	inline operator T() 
		{return _value;};

	inline T* get(void) const
		{return &_value;};
};

template <class T, class P = auto_release>
class pointer : public P
{
public:
	inline pointer() : P() {};

	inline pointer(T* o) : P(o) {};

	inline T& operator*() const
		{return *(static_cast<T*>(P::object));};

	inline T* operator->() const
		{return static_cast<T*>(P::object);};

	inline T* get(void) const
		{return static_cast<T*>(P::object);};

	inline T* operator++()
		{P::operator++(); return get();};

    inline void operator--()
        {P::operator--(); return get();};
};

inline void retain(Object *obj)
	{obj->retain();};

inline void release(Object *obj)
	{obj->release();};

inline Object *copy(Object *obj)
	{return obj->copy();};

END_NAMESPACE

#endif
