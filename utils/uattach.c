#include <inc/process.h>
#include <inc/signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <unistd.h>

static const char *iodev = "-";

static RETSIGTYPE sigfault(int signo)
{
	openlog("attach", LOG_CONS, LOG_DAEMON);
	syslog(LOG_CRIT, "failed; device=%s, reason=%d", iodev, signo);
	closelog();
	exit(-1);
}

int main(int argc, char **argv)
{
	struct stat ino;

	if(argc < 3) {
		fprintf(stderr, "use: attach /dev/.. command ...\n");
		exit(-1);
	}
	cpr_signal(SIGABRT, sigfault);
	iodev = *(++argv);
	if(stat(iodev, &ino)) {
		fprintf(stderr, "*** attach: %s; cannot access\n", iodev);
		exit(-1);
	}

	if(!S_ISCHR(ino.st_mode)) {
		fprintf(stderr, "*** attach: %s; not valid device\n", iodev);
		exit(-1);
	}

	if(access(iodev, R_OK | W_OK)) {
		fprintf(stderr, "*** attach: %s; cannot access for rw\n", iodev);
		exit(-1);
	}
 
	cpr_pattach(iodev);
	++argv;
	execvp(*argv, argv);
	sigfault(-1);
}
