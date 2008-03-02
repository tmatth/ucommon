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
 * A common object base class with auto-pointer support.
 * A common object class is used which may be referenced counted and
 * associated with a smart auto-pointer class.  A lot of the things
 * found here were inspired by working with Objective-C.  Many of the
 * classes are designed to offer automatic heap management through
 * smart pointers and temporary objects controlled through the scope of
 * the stack frame of method calls.
 * @file ucommon/object.h
 */

#ifndef	_UCOMMON_OBJECT_H_
#define	_UCOMMON_OBJECT_H_

#ifndef	_UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

#include <stdlib.h>

NAMESPACE_UCOMMON

/**
 * A common base class for all managed objects.  This is used to manage
 * objects that might be linked or reference counted.  The base class defines
 * only core virtuals some common public methods that should be used by
 * all inherited object types.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Object
{
public:
	/**
	 * Method to retain (or increase retention) of an object.
	 */
	virtual void retain(void);

	/**
	 * Method to release (or decrease retention) of an object.
	 */
	virtual void release(void);

	/**
	 * Required virtual destructor.
	 */
	virtual ~Object();

	/**
	 * Retain (increase retention of) object when copying.
	 */
	Object *copy(void);

	/**
	 * Increase retention operator.
	 */
	inline void operator++(void)
		{retain();};

	/**
	 * Decrease retention operator.
	 */
	inline void operator--(void)
		{release();};
};

/**
 * A base class for reference counted objects.  Reference counted objects
 * keep track of how many objects refer to them and fall out of scope when
 * they are no longer being referred to.  This can be used to achieve
 * automatic heap management when used in conjunction with smart pointers.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT CountedObject : public Object
{
private:
	volatile unsigned count;

protected:
	/**
	 * Construct a counted object, mark initially as unreferenced.
	 */
	CountedObject();

	/**
	 * Construct a copy of a counted object.  Our instance is not a
	 * reference to the original object but a duplicate, so we do not
	 * retain the original and we do reset our count to mark as
	 * initially unreferenced.
	 */
	CountedObject(const Object &ref);

	/**
	 * Dealloc object no longer referenced.  The dealloc routine would commonly 
	 * be used for a self delete to return the object back to a heap when 
	 * it is no longer referenced.
	 */
	virtual void dealloc(void);

public:
	/**
	 * Test if the object has copied references.  This means that more than
	 * one object has a reference to our object.
	 * @return true if referenced by more than one object.
	 */
	inline bool isCopied(void)
		{return count > 1;};

	/**
	 * Test if the object has been referenced (retained) by anyone yet.
	 * @return true if retained.
	 */
	inline bool isRetained(void)
		{return count > 0;};

	/**
	 * Return the number of active references (retentions) to our object.
	 * @return number of references to our object.
	 */
	inline unsigned copied(void)
		{return count;};

	/**
	 * Increase reference count when retained.
	 */
	void retain(void);

	/**
	 * Decrease reference count when released.  If no longer retained, then
	 * the object is dealloc'd.
	 */
	void release(void);
};

/**
 * Used as base class for temporary objects.  Temporary objects may be
 * allocated from the heap within a method call and removed automatically
 * removed when a method call completes.  This base class is meant to be
 * used in conjunction with an auto (stack frame) object referenced through 
 * the auto_delete class as managed through the temporary template. 
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Temporary
{
protected:
	friend class auto_delete;
	virtual ~Temporary();
};

/**
 * A helper class for the temporary object template.  This class is assumed
 * to be created as an auto variable that will contain a pointer to an
 * object built from the Temporary base class.  When auto_delete falls out
 * of scope as a member function exits it deletes the heap based Temporary 
 * object as well.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT auto_delete
{
protected:
	Temporary *object;

public:
	~auto_delete();
};

/**
 * A general purpose smart pointer helper class.  This is particularly
 * useful in conjunction with reference counted objects which can be
 * managed and automatically removed from the heap when they are no longer
 * being referenced by a smart pointer.  The smart pointer itself would
 * normally be constructed and initialized as an auto variable in a method
 * call, and will dereference the object when the pointer falls out of scope.
 * This is actually a helper class for the typed pointer template.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT auto_pointer
{
protected:
	Object *object;
	
	auto_pointer();

public:
	/**
	 * Construct an auto-pointer referencing an existing object.
	 * @param object we point to.
	 */
	auto_pointer(Object *object);
	
	/**
	 * Construct an auto-pointer as a copy of another pointer.  The
	 * retention of the object being pointed to will be increased.
	 * @param pointer we are a copy of.
	 */
	auto_pointer(const auto_pointer &pointer);

	/**
	 * Delete auto pointer.  When it falls out of scope, the retention
	 * of the object it references is reduced.  If it falls to zero in
	 * a reference counted object, then the object is auto-deleted.
	 */
	~auto_pointer();

	/**
	 * Manually release the pointer.  This reduces the retention level
	 * of the object and resets the pointer to point to nobody.
	 */
	void release(void);

	/**
	 * Test if the pointer is not set.
	 * @return true if the pointer is not referencing anything.
	 */
	bool operator!() const;

	/**
	 * Test if the pointer is referencing an object.
	 * @return true if the pointer is currently referencing an object.
	 */
	operator bool() const;

	/**
	 * test if the object being referenced is the same as the object we specify.
	 * @param object we compare to.
	 * @return true if this is the object our pointer references.
	 */
	bool operator==(Object *object) const;

	/**
	 * test if the object being referenced is not the same as the object we specify.
	 * @param object we compare to.
	 * @return true if this is not the object our pointer references.
	 */
	bool operator!=(Object *object) const;

	/**
	 * Set our pointer to a specific object.  If the pointer currently
	 * references another object, that object is released.  The pointer
	 * references our new object and that new object is retained.
	 * @param object to assign to.
	 */
	auto_pointer &operator=(Object *object);
};	

/**
 * A sparse array of managed objects.  This might be used as a simple
 * array class for reference counted objects.  This class assumes that
 * objects in the array exist when assigned, and that gaps in the array
 * are positions that do not reference any object.  Objects are automatically
 * created (create on access/modify when an array position is referenced
 * for the first time.  This is an abstract class because it is a type
 * factory for objects who's derived class form constructor is not known
 * in advance and is a helper class for the sarray template.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT sparse_array
{
private:
	Object **vector;
	unsigned max;

protected:
	/**
	 * Object factory for creating members of the spare array when they
	 * are initially requested.
	 * @return new object.
	 */
	virtual Object *create(void) = 0;

	/**
	 * Purge the array by deleting all created objects.
	 */
	void purge(void);

	/**
	 * Get (reference) an object at a specified offset in the array.
	 * @param offset in array.
	 * @return new or existing object.
	 */
	Object *get(unsigned offset);

	/**
	 * Create a sparse array of known size.  No member objects are
	 * created until they are referenced.
	 * @param size of array.
	 */
	sparse_array(unsigned size);

public:
	/**
	 * Destroy sparse array and delete all generated objects.
	 */
	virtual ~sparse_array();

	/**
	 * Get count of array elements.
	 * @return array elements.
	 */
	unsigned count(void);
};
	
/**
 * Generate a typed sparse managed object array.  Members in the array
 * are created when they are first referenced.  The types for objects
 * that are generated by sarray must have Object as a base class.  Managed
 * sparse arrays differ from standard arrays in that the member elements
 * are not allocated from the heap when the array is created, but rather
 * as they are needed.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template <class T>
class sarray : public sparse_array
{
public:
	/**
	 * Generate a sparse typed array of specified size.
	 * @param size of array to create.
	 */
	inline sarray(unsigned size) : sparse_array(size) {};

	/**
	 * Get typed member of array.  If the object does not exist, it is
	 * created.
	 * @param offset in array for object.
	 * @return pointer to typed object.
	 */
	inline T *get(unsigned offset)
		{static_cast<T*>(sparse_array::get(offset));};

	/**
	 * Array operation to access member object.  If the object does not
	 * exist, it is created.
	 * @param offset in array for object.
	 * @return pointer to typed object.
	 */
	inline T *operator[](unsigned offset)
		{return get(offset);};

private:
	__LOCAL Object *create(void)
		{return new T;};
};

/**
 * Manage temporary object stored on the heap.  This is used to create a
 * object on the heap who's scope is controlled by the scope of a member
 * function call.  Sometimes we have data types and structures which cannot 
 * themselves appear as auto variables.  We may also have a limited stack
 * frame size in a thread context, and yet have a dynamic object that we
 * only want to exist during the life of the method call.  Using temporary
 * allows any type to be created from the heap but have a lifespan of a
 * method's stack frame.  All types managed as temporary must have Temporary
 * as a base class.
 * @author David Sugar <dyfet@gnutelephony.org>
 */  
template <class T>
class temporary : public auto_delete
{
public:
	/**
	 * Construct a temporary object, create our stack frame reference.
	 */
	inline temporary() 
		{object = new T;};

	/**
	 * Access heap object through our temporary directly.
	 * @return reference to heap resident object.
	 */
	inline T& operator*() const
		{return *(static_cast<T*>(object));};

	/**
	 * Access members of our heap object through our temporary.
	 * @return member reference of heap object.
	 */
	inline T* operator->() const
		{return static_cast<T*>(object);};
};

/**
 * Template for embedding a data structure into a reference counted object.  
 * This is a convenient means to create reference counted heap managed data 
 * structure.  This template can be used for embedding data into other kinds
 * of managed object classes in addition to reference counting.  For example,
 * it can be used to embed a data structure into a linked list, as shown in 
 * the linked_value template.
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
template <class T, class O = CountedObject>
class object_value : public O
{
protected:
	/**
	 * Assign our value from a typed data object.  This is a helper method.
	 * @param object to assign our value from.
	 */
	inline void set(T &object)
		{value = object;};

public:
	T value;	/**< Embedded data value */

	/**
	 * Construct composite value object.
	 */
	inline object_value() : O() {};

	/**
	 * Construct composite value object and assign from existing data value.
	 * @param existing typed value to assign.
	 */
	inline object_value(T existing) : O() 
		{value = existing;};

	/**
	 * Pointer reference to embedded data value.
	 * @return embedded value.
	 */
	inline T& operator*()
		{return value;};

	/**
	 * Assign embedded data value.
	 * @param data value to assign.
	 */
	inline void operator=(T data)
		{value = data;};

	/**
	 * Retrieve data value by casting reference.
	 * @return embedded value.
	 */
	inline operator T() 
		{return value;};

	/**
	 * Retrieve data value by expression reference.
	 * @return embedded value.
	 */
	inline T &operator()()
		{return value;};

	/**
	 * Set data value by expression reference.
	 * @param data value to assign.
	 */
	inline void operator()(T data)
		{value = data;};
};

/**
 * Typed smart pointer class.  This is used to manage references to
 * a specific typed object on the heap that is derived from the base Object 
 * class.  This is most commonly used to manage references to reference
 * counted heap objects so their heap usage can be auto-managed while there
 * is active references to such objects.  Pointers are usually created on
 * the stack frame and used to reference an object during the life of a
 * member function.  They can be created in other objects that live on the
 * heap and can be used to maintain active references so long as the object 
 * they are contained in remains in scope as well.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template <class T, class P = auto_pointer>
class pointer : public P
{
public:
	/**
	 * Create a pointer with no reference.
	 */
	inline pointer() : P() {};

	/**
	 * Create a pointer with a reference to a heap object.
	 * @param object we are referencing.
	 */
	inline pointer(T* object) : P(object) {};

	/**
	 * Reference object we are pointing to through pointer indirection.
	 * @return object we are pointing to.
	 */
	inline T& operator*() const
		{return *(static_cast<T*>(P::object));};

	/**
	 * Reference member of object we are pointing to.
	 * @return reference to member of pointed object.
	 */
	inline T* operator->() const
		{return static_cast<T*>(P::object);};

	/**
	 * Get pointer to object.
	 * @return pointer or NULL if we are not referencing an object.
	 */
	inline T* get(void) const
		{return static_cast<T*>(P::object);};

	/**
	 * Iterate our pointer if we reference an array on the heap.
	 * @return next object in array.
	 */
	inline T* operator++()
		{P::operator++(); return get();};

	/**
	 * Iterate our pointer if we reference an array on the heap.
	 * @return previous object in array.
	 */
    inline void operator--()
        {P::operator--(); return get();};
};

/**
 * Convenence function to access object retention.
 * @param object we are retaining.
 */
inline void retain(Object *object)
	{object->retain();};

/**
 * Convenence function to access object release.
 * @param object we are releasing.
 */
inline void release(Object *object)
	{object->release();};

/**
 * Convenence function to access object copy.
 * @param object we are copying.
 */
inline Object *copy(Object *object)
	{return object->copy();};

/**
 * Convenience function to validate object.  This solves issues where
 * some compilers get confused between bool and pointer operators.
 * @param object we are testing.
 * @return true if object valid.
 */
template<class T>
inline bool is(T& object)
	{return object.operator bool();};

/**
 * Convenience function to test pointer object.  This solves issues where
 * some compilers get confused between bool and pointer operators.
 * @param object we are testing.
 * @return true if object points to NULL.
 */
template<class T>
inline bool isnull(T& object)
	{return (bool)(object.operator*() == NULL);};

/**
 * Convenience function to swap objects.
 * @param o1 to swap.
 * @param o2 to swap.
 */
template<class T>
inline void swap(T& o1, T& o2)
	{T tmp; tmp = o1; o1 = o2; o2 = tmp;};

/**
 * Convenience function to return max of two objects.
 * @param o1 to check.
 * @param o2 to check.
 * @return max object.
 */
template<class T>
inline T max(T& o1, T& o2)
{
	return o1 > o2 ? o1 : o2;
} 

/**
 * Convenience function to return min of two objects.
 * @param o1 to check.
 * @param o2 to check.
 * @return min object.
 */
template<class T>
inline T min(T& o1, T& o2)
{
	return o1 < o2 ? o1 : o2;
} 

END_NAMESPACE

#endif
