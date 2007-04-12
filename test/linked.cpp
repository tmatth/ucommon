#define	DEBUG
#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

typedef linked_value<int> ints;

static OrderedIndex list;

extern "C" int main()
{
	linked_pointer<ints> ptr;
	ints v1(&list, 3);
	ints v2(&list);
	v2 = 5;

	ptr = &list;
	while(ptr) {
		printf("VALUE %d\n", ptr->value);
		++ptr;
	}
	printf("DONE!\n");
};
