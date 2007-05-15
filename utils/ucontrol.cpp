#include <ucommon/service.h>
#include <ucommon/string.h>
#include <sys/stat.h>
#include <config.h>

using namespace UCOMMON_NAMESPACE;

#ifndef _MSWINDOWS_

static RETSIGTYPE signotify(int signo)
{
	switch(signo) {
	case SIGUSR1:
		exit(0);
	case SIGUSR2:
		exit(1);
	case SIGALRM:
		exit(2);
	default:
		exit(signo);
	}
}
#endif

int main(int argc, char **argv)
{
	int timeout = 0;

	if(argc > 1 && argv[1][0] == '-') {
		timeout = atoi(argv[1] + 1);
		if(timeout) {
			++argv;
			--argc;
		}
	}

	if(argc != 3) {
		fprintf(stderr, "use: control [-timeout|-reload|-shutdown|-terminate|-dump] service|id [\"command\"] ...\n");
		exit(-1);
	}

	if(!timeout && (!stricmp(argv[1], "-dump") || !stricmp(argv[1], "-d"))) {
		MappedMemory *view = new MappedMemory(argv[1]);
		if(!(*view)) {
			fprintf(stderr, "*** %s: cannot access\n", argv[1]);
			exit(-1);
		}

		fwrite(view->get(0), view->len(), 1, stdout);
		exit(0);
	}

	if(!timeout && (!stricmp(argv[1], "-reload") || !stricmp(argv[1], "-r"))) {
		if(!service::reload(argv[2])) {
			fprintf(stderr, "*** %s: cannot reload\n", argv[2]);
			exit(-1);
		}
		exit(0);
	}

	if(!timeout && (!stricmp(argv[1], "-shutdown") || !stricmp(argv[1], "-s"))) {
		if(!service::shutdown(argv[2])) {
			fprintf(stderr, "*** %s: cannot shutdown\n", argv[2]);
			exit(-1);
		}
		exit(0);
	}

	if(!timeout && (!stricmp(argv[1], "-terminate") || !stricmp(argv[1], "-t"))) {
		if(!service::terminate(argv[2])) {
			fprintf(stderr, "*** %s: cannot terminate\n", argv[2]);
			exit(-1);
		}
		exit(0);
	}

#ifdef _MSWINDOWS_
	if(!service::control(argv[1], "%s\n", argv[2])) {
		fprintf(stderr, "*** control: %s; cannot access\n", argv[1]);
		exit(-1);
	}
#else
	signal(SIGUSR1, signotify);
	signal(SIGUSR2, signotify);
	signal(SIGALRM, signotify);

	if(timeout)
		alarm(timeout);

	if(!service::control(argv[1], "%d %s\n", getpid(), argv[2])) {
		fprintf(stderr, "*** control: %s; cannot access\n", argv[1]);
		exit(-1);
	}

	if(timeout)
		pause();
#endif
	exit(0);
}

