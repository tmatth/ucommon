#define	DEBUG
#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace UCOMMON_NAMESPACE;

static string testing("second test");

extern "C" int main()
{
	char buff[33];
	string::fill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string)"hello" + (string)" this is a test";
	printf("STARTING %s\n", *mystr);
	printf("SECOND %s\n", *testing);
	printf("AN OFFSET %s\n", mystr(-10));
};
