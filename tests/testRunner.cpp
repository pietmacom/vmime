//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2005 Vincent Richard <vincent@vincent-richard.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <sys/time.h>
#include <time.h>

#include <iostream>

#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestFailure.h>
#include <cppunit/SourceLine.h>
#include <cppunit/Exception.h>
#include <cppunit/tools/XmlDocument.h>
#include <cppunit/tools/XmlElement.h>

#include "vmime/vmime.hpp"
#include "vmime/platforms/posix/posixHandler.hpp"


class Clock
{
public:

	void reset()
	{
		struct timezone tz;

		gettimeofday(&m_start, &tz);
	}

	const double getDuration() const
	{
		struct timeval tv;
		struct timezone tz;

		gettimeofday(&tv, &tz);

		return static_cast <double>(tv.tv_sec - m_start.tv_sec)
			+ static_cast <double>(tv.tv_usec - m_start.tv_usec) / 1000000.0;
	}

private:

	struct timeval m_start;
};


class XmlTestListener : public CppUnit::TestListener
{
public:

	XmlTestListener()
		: m_doc("utf-8"), m_testElt(NULL)
	{
		m_doc.setRootElement(new CppUnit::XmlElement("TestRun"));
	}

	void startTest(CppUnit::Test* test)
	{
		m_testElt = new CppUnit::XmlElement("Test");
		m_suiteElt.back()->addElement(m_testElt);

		m_testElt->addElement(new CppUnit::XmlElement("Name", test->getName()));

		m_chrono.reset();
	}

	void addFailure(const CppUnit::TestFailure& failure)
	{
		CppUnit::XmlElement* failElt = new CppUnit::XmlElement("Failure");
		m_testElt->addElement(failElt);

		failElt->addElement(new CppUnit::XmlElement("FailureType",
			failure.isError() ? "Error" : "Assertion"));

		if (failure.sourceLine().isValid())
		{
			CppUnit::XmlElement* locElt = new CppUnit::XmlElement("Location");
			failElt->addElement(locElt);

			locElt->addElement(new CppUnit::XmlElement("File", failure.sourceLine().fileName()));
			locElt->addElement(new CppUnit::XmlElement("Line", failure.sourceLine().lineNumber()));
		}

		CppUnit::XmlElement* exElt = new CppUnit::XmlElement("Exception");
		failElt->addElement(exElt);

		exElt->addElement(new CppUnit::XmlElement("Message", failure.thrownException()->what()));
	}

	void endTest(CppUnit::Test* /* test */)
	{
		std::ostringstream ossTime;
		ossTime << (m_chrono.getDuration() * 1000.0);

		m_testElt->addElement(new CppUnit::XmlElement("Time", ossTime.str()));

		m_testElt = NULL;
	}

	void startSuite(CppUnit::Test* suite)
	{
		if (suite->getName() == "All Tests")
			return;

		CppUnit::XmlElement* suiteElt = new CppUnit::XmlElement("Suite");

		if (m_suiteElt.size() == 0)
			m_doc.rootElement().addElement(suiteElt);
		else
			m_suiteElt.back()->addElement(suiteElt);

		m_suiteElt.push_back(suiteElt);

		suiteElt->addElement(new CppUnit::XmlElement("Name", suite->getName()));
	}

	void endSuite(CppUnit::Test* /* suite */)
	{
		if (m_suiteElt.size())
			m_suiteElt.pop_back();
	}

	void startTestRun(CppUnit::Test* /* test */, CppUnit::TestResult* /* eventManager */)
	{
	}

	void endTestRun(CppUnit::Test* /* test */, CppUnit::TestResult* /* eventManager */)
	{
	}

	void output(std::ostream& os)
	{
		os << m_doc.toString();
	}

private:

	Clock m_chrono;

	CppUnit::XmlDocument m_doc;
	std::vector <CppUnit::XmlElement*> m_suiteElt;
	CppUnit::XmlElement* m_testElt;
};


int main(int argc, char* argv[])
{
        // VMime initialization
        vmime::platformDependant::setHandler<vmime::platforms::posix::posixHandler>();

	// Parse arguments
	bool xmlOutput = false;

	for (int c = 1 ; c < argc ; ++c)
	{
		const std::string arg = argv[c];

		if (arg == "--xml")
			xmlOutput = true;
	}

	// Run the tests
	if (xmlOutput)
	{
		// Get the test suites from the registry and add them to the list of test to run
		CppUnit::TestRunner runner;
		runner.addTest(CppUnit::TestFactoryRegistry::getRegistry("Parser").makeTest());
		runner.addTest(CppUnit::TestFactoryRegistry::getRegistry("Utility").makeTest());
		runner.addTest(CppUnit::TestFactoryRegistry::getRegistry("Misc").makeTest());

		std::auto_ptr <XmlTestListener> xmlListener(new XmlTestListener);

		CppUnit::TestResult controller;
		controller.addListener(xmlListener.get());

		CppUnit::TestResultCollector result;
		controller.addListener(&result);

		runner.run(controller);

		xmlListener->output(std::cout);

		// Return error code 1 if the one of test failed
		return result.wasSuccessful() ? 0 : 1;
	}
	else
	{
		// Get the top level suite from the registry
		CppUnit::TextUi::TestRunner runner;
		runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

		return runner.run() ? 0 : 1;
	}
}
