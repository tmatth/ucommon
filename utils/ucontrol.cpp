#include <ucommon/process.h>
#include <ucommon/file.h>
#include <sys/stat.h>
#include <config.h>
#include <stdio.h>

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
	MessageQueue *mq;
	char cpath[65];
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

	snprintf(cpath, sizeof(cpath), "/%s", argv[1]);

	fd = cpr_openctrl(cpath);
	if(!cpr_isopen(fd)) {
		mq = new MessageQueue(cpath);
		if(*mq)
			goto queue;
		fprintf(stderr, "*** control: %s; cannot access\n", argv[1]);
		exit(-1);
	}

	snprintf(ctrlcmd, sizeof(ctrlcmd), "%d %s\n", getpid(), argv[2]);

	if(timeout)
		alarm(timeout);

	cpr_writefile(fd, ctrlcmd, strlen(ctrlcmd));
	cpr_closefile(fd);
	pause();
	exit(-1);
queue:
	mq->puts(argv[2]);
	delete mq;
	exit(0);
}
