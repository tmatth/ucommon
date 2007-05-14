#include <ucommon/service.h>
#include <ucommon/string.h>
#include <sys/stat.h>
#include <config.h>

using namespace UCOMMON_NAMESPACE;

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
		fprintf(stderr, "use: control [-timeout|-reload|-shutdown|-terminate] service [\"command\"] ...\n");
		exit(-1);
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

	exit(0);
}

