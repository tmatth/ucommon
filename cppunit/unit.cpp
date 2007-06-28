// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <ucommon/ucommon.h>

using namespace UCOMMON_NAMESPACE;

extern "C" void callThreading();

class ucommonTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ucommonTest);
	CPPUNIT_TEST(testThreading);
	CPPUNIT_TEST(testStrings);
	CPPUNIT_TEST(testLinked);
	CPPUNIT_TEST_SUITE_END();

public:
	void testThreading();
	void testStrings();
	void testLinked();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ucommonTest);

void ucommonTest::testStrings()
{
	static string testing("second test");
	char buff[33];
	string::fill(buff, 32, ' ');
	stringbuf<128> mystr;
	mystr = (string)"hello" + (string)" this is a test";
	CPPUNIT_ASSERT(!stricmp("hello this is a test", *mystr));
	CPPUNIT_ASSERT(!stricmp("second test", *testing));
	CPPUNIT_ASSERT(!stricmp(" is a test", mystr(-10)));
	mystr = "  abc 123 \n  ";
	CPPUNIT_ASSERT(!stricmp("abc 123", string::strip(mystr.c_mem(), " \n")));
}

void ucommonTest::testLinked()
{
	typedef linked_value<int> ints;
	OrderedIndex list;
	linked_pointer<ints> ptr;
	unsigned count = 0;
	ints v1(&list, 3);
	ints v2(&list);
	v2 = 5;

	ptr = &list;
	while(ptr) {
		switch(++count)
		{
		case 1:
			CPPUNIT_ASSERT(ptr->value == 3);
			break;
		case 2:
			CPPUNIT_ASSERT(ptr->value == 5);
		}
		++ptr;
	}
	CPPUNIT_ASSERT(count == 2);
}

void ucommonTest::testThreading()
{
	class testThread : public JoinableThread
	{
	public:
		int runflag;

		testThread() : JoinableThread() {
			runflag = 0;
		}

		void run(void) {
			++runflag;
			Thread::sleep(10000);
		};
	};

	time_t now, later;
	testThread *thread;

	time(&now);
	thread = new testThread();
	start(thread);
	Thread::sleep(1000);
	CPPUNIT_ASSERT(thread->runflag == 1);
	cancel(thread);
	delete thread;
	time(&later);

	CPPUNIT_ASSERT(later = now + 1);
}

int main(int argc, char **argv)
{
	CppUnit::Test *suite = 
		CppUnit::TestFactoryRegistry::getRegistry().makeTest();

	CppUnit::TextUi::TestRunner runner;
	runner.addTest( suite );

	runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), std::cerr ) );

	bool result = runner.run();
	return result ? 0 : 1;
}

