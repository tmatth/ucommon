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
using namespace std;

extern "C" void callThreading();

class ucommonTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ucommonTest);
	CPPUNIT_TEST(testThreading);
	CPPUNIT_TEST_SUITE_END();

public:
	void testThreading();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ucommonTest);

void ucommonTest::testThreading()
{
	class testThread : public JoinableThread
	{
	public:
		inline testThread() : JoinableThread() {};

		void run(void) {
			Thread::sleep(10000);
		};
	};

	time_t now, later;
	testThread *thread;

	time(&now);
	thread = new testThread();
	start(thread);
	Thread::sleep(1000);
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

	runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), cerr ) );

	bool result = runner.run();
	return result ? 0 : 1;
}

