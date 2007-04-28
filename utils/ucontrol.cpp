#include <ucommon/process.h>
#include <ucommon/file.h>
#include <sys/stat.h>
#include <config.h>
#include <stdio.h>

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
	char ctrlpath[65];
	char ctrlcmd[512];
	fd_t fd;
	int timeout = 0;

	if(argc > 1 && argv[1][0] == '-') {
		timeout = atoi(argv[1] + 1);
		++argv;
		--argc;
	}

	if(argc != 3) {
		fprintf(stderr, "use: control [-timeout] service \"command\" ...\n");
		exit(-1);
	}
	cpr_signal(SIGUSR1, signotify);
	cpr_signal(SIGUSR2, signotify);
	cpr_signal(SIGALRM, signotify);

	fd = cpr_openctrl(argv[1]);
	if(!cpr_isopen(fd)) {
		fprintf(stderr, "*** control: %s; cannot access\n", ctrlpath);
		exit(-1);
	}

	snprintf(ctrlcmd, sizeof(ctrlcmd), "%d %s\n", getpid(), argv[2]);

	if(timeout)
		alarm(timeout);

	write(fd, ctrlcmd, strlen(ctrlcmd));
	cpr_closefile(fd);
	pause();
}
