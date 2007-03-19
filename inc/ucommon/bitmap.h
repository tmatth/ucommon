#ifndef _UCOMMON_BITMAP_H_
#define	_UCOMMON_BITMAP_H_

#ifndef _UCOMMON_CONFIG_H_
#include ucommon/config.h
#endif

NAMESPACE_UCOMMON

class __EXPORT bitmap
{
protected:
	size_t size;

	typedef union
	{
		void *a;
		uint8_t *b;
		uint16_t *w;
		uint32_t *l;
		uint64_t *d;
	}	addr_t;

	addr_t addr;

public:
	typedef enum {
		BMALLOC,
		B8,
		B16,
		B32,
		B64
	} bus_t;

protected:
	bus_t bus;

	unsigned memsize(void);

public:
	bitmap(void *addr, size_t size, bus_t access = B8); 
	bitmap(size_t size);
	~bitmap();

	void clear(void);

	bool get(size_t offset);
	void set(size_t offset, bool bit);
};

END_NAMESPACE

#endif
