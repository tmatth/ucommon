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

