#define	DEBUG
#include <inc/config.h>
#include <inc/object.h>
#include <inc/timers.h>
#include <inc/string.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

static string testing("second test");

extern "C" int main()
{
	char buff[33];
	cpr_strfill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string)"hello" + (string)" this is a test";
	printf("STARTING %s\n", *mystr);
	printf("SECOND %s\n", *testing);
	printf("AN OFFSET %s\n", mystr[-10]);
};
