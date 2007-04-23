#include <inc/process.h>
#include <stdio.h>
#include <syslog.h>

static RETSIGTYPE sigfault(int signo)
{
	openlog("detach", LOG_CONS, LOG_DAEMON);
	syslog(LOG_CRIT, "failed; reason=%d", signo);
	closelog();
	exit(-1);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "use: detach command ...\n");
		exit(-1);
	}

	cpr_signal(SIGABRT, sigfault);

	++argv;
	cpr_pdetach();
	execvp(*argv, argv);
	sigfault(-1);	
}
