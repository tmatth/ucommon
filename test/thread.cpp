#define	DEBUG
#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

class mythread : public JoinableThread
{
public:
	inline mythread() : JoinableThread() {};

	void run(void);
};

void mythread::run(void)
{
	printf("starting thread...\n");
	pause(10000);
	printf("finishing thread\n");
};

extern "C" int main()
{
	mythread *thr = new mythread();

	printf("before\n");
	thr->start();
	printf("sleeping main...\n");
	cpr_sleep(1000);
	printf("wakeup main...\n");
	thr->release();
	printf("joining\n");
	delete thr;
	printf("ending\n");
};
