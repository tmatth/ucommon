#define	DEBUG
#include <ucommon/ucommon.h>

#include <iostream>
#include <cstdio>

using namespace UCOMMON_NAMESPACE;

static string testing("second test");

int main()
{
	char buff[33];
	cpr_strfill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string)"hello" + (string)" this is a test";
	std::cout << "STARTING " << *mystr << std::endl;
	std::cout << "SECOND " << *testing << std::endl;
	std::cout << "AN OFFET " << mystr(-10) << std::endl;
};
