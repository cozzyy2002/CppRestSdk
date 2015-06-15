// maquette_unittest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	log4cplus::initialize();
	log4cplus::PropertyConfigurator::doConfigure(_T("log4cplus.properties"));

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
