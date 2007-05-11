#include <ucommon/service.h>
#include <ucommon/string.h>
#include <config.h>
#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

int main(int argc, char **argv)
{
	MappedMemory *view;

	if(argc != 2) {
		fprintf(stderr, "use: ushmdump shmname\n");
		exit(-1);
	}

	if(argv[1][0] != '/' || strchr(argv[1] + 1, '/')) {
		fprintf(stderr, "*** %s: invalid shm name\n", argv[1]);
		exit(-1);
	}
	view = new MappedMemory(argv[1]);
	if(!(*view)) {
		fprintf(stderr, "*** %s: cannot access\n", argv[1]);
		exit(-1);
	}

	fwrite(view->get(0), view->len(), 1, stdout);
}
