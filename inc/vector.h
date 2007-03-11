#ifndef	_UCOMMON_VECTOR_H_
#define	_UCOMMON_VECTOR_H_

#ifndef	_UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

typedef	unsigned short vectorsize_t;

NAMESPACE_UCOMMON

class __EXPORT Vector
{
protected:
	class __EXPORT array : public CountedObject
	{
	public:
#pragma pack(1)
		vectorsize_t max, len;
		Object *list[1];
#pragma pack()

		array(vectorsize_t size);
		void dealloc(void);
		void set(Object **items);
		void add(Object **list);
		void add(Object *obj);
		void purge(void);
		void inc(vectorsize_t adj);
		void dec(vectorsize_t adj);
	};

	array *data;

	array *create(vectorsize_t size) const;

	virtual void release(void);
	virtual array *copy(void) const;
	virtual void cow(vectorsize_t adj = 0);
	Object **list(void) const;

public:
	static const vectorsize_t npos;

	Vector();
	Vector(vectorsize_t size);
	Vector(Object **items, vectorsize_t size = 0);
	virtual ~Vector();

	vectorsize_t len(void) const;
	vectorsize_t max(void) const;

	Object *get(int index) const;
	vectorsize_t get(void **mem, vectorsize_t max) const;
	Object *begin(void) const;
	Object *end(void) const;
	vectorsize_t find(Object *obj, vectorsize_t pos) const;

	void split(vectorsize_t pos);
	void rsplit(vectorsize_t pos);
	void set(vectorsize_t pos, Object *obj);
	void set(Object **list);
	void add(Object **list);
	void add(Object *obj);
	void clear(void);
	virtual bool resize(vectorsize_t size);

	inline void set(Vector &v)
		{set(v.list());};

	inline void add(Vector &v)
		{add(v.list());};

	inline Object *operator[](int index)
		{return get(index);};

	inline void operator()(vectorsize_t pos, Object *obj)
		{set(pos, obj);};

	inline Object *operator()(vectorsize_t pos)
		{return get(pos);};

	inline void operator()(Object *obj)
		{return add(obj);};

	inline void operator=(Vector &v)
		{set(v.list());};

	inline void operator+=(Vector &v)
		{add(v.list());};

	inline Vector& operator+(Vector &v)
		{add(v.list()); return *this;};

	Vector &operator^(Vector &v);
	void operator^=(Vector &v);
	void operator++();
	void operator--();
	void operator+=(vectorsize_t vs);
	void operator-=(vectorsize_t vs);
};

class __EXPORT MemVector : public Vector
{
private:
	bool resize(vectorsize_t size);
	void cow(vectorsize_t adj = 0);
	void release(void);

protected:
	array *copy(void) const;

public:
	inline void operator=(Vector &v)
		{set(v);};

	MemVector(void *mem, vectorsize_t size);
	~MemVector();
};

template<class T>
class vector : public Vector
{
public:
	inline vector() : Vector() {};
	inline vector(vectorsize_t size) : Vector(size) {};

	inline operator Vector()
		{return static_cast<Vector>(*this);};

	inline T *get(int pos)
		{return static_cast<T *>(Vector::get(pos));};

    inline T *operator()(vectorsize_t pos)
        {return static_cast<T *>(Vector::get(pos));};

	inline T *begin(void)
		{return static_cast<T *>(Vector::begin());};

	inline T *end(void)
		{return static_cast<T *>(Vector::end());};

	inline Vector &operator+(Vector &v)
		{Vector::add(v); return static_cast<Vector &>(*this);};
};

template<class T, vectorsize_t S>
class vectorbuf : public MemVector
{
private:
	char buffer[sizeof(array) + (S * sizeof(void *))];
	
public:
	inline vectorbuf() : MemVector(buffer, S) {};

	inline operator Vector()
		{return static_cast<Vector>(*this);};

	inline T *get(int pos)
		{return static_cast<T *>(Vector::get(pos));};

    inline T *operator()(vectorsize_t pos)
        {return static_cast<T *>(Vector::get(pos));};

	inline T *begin(void)
		{return static_cast<T *>(Vector::begin());};

	inline T *end(void)
		{return static_cast<T *>(Vector::end());};

	inline Vector &operator+(Vector &v)
		{Vector::add(v); return static_cast<Vector &>(*this);};
};

END_NAMESPACE

extern "C" {
	vectorsize_t cpr_vectorsize(void **list);
};

#endif
