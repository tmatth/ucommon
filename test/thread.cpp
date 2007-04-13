#define	DEBUG
#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

class mythread : public CThread
{
public:
	inline mythread() : CThread() {};

	void run(void);
};

void mythread::run(void)
{
	printf("starting...\n");
	cpr_sleep(10000);
	printf("finishing\n");
};

extern "C" int main()
{
	mythread *thr = new mythread();

	printf("before\n");
	thr->start();
	cpr_sleep(1000);
	thr->release();
	printf("joining\n");
	delete thr;
	printf("ending\n");
};
