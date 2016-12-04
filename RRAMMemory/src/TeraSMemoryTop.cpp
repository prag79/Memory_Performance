/***
* TeraSMemoryTop.cpp 
: Defines the entry point for the console application.
***/

#define REPORT_DEFINE_GLOBALS
#include "reporting.h"
#include "TeraSTestBench.h"
#include "time.h"

int sc_main(int argc, char** argv)
{
	clock_t startTime = clock();
	
	sc_core::sc_set_time_resolution(1000, SC_PS);
	try {
		if (argc == 10){
			if (atoi(argv[8]) == 1)
			{
				REPORT_ENABLE_ALL_REPORTING();
			}
			else {
				REPORT_DISABLE_ALL_REPORTING();
			}
			
			TeraSTestBench test(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[9]));
			sc_start();
			clock_t stopTime = clock();
			double elapsedTime = (double)(stopTime - startTime) / (CLOCKS_PER_SEC * 60);
			std::cout << " Total CPU Time:" << std::dec << elapsedTime << " Minutes" << std::endl;
			test.erase();
			system("pause");
		}
	}
	catch (const char* msg)
	{
		std::cerr << msg << std::endl;
		system("pause");
	}
	return EXIT_SUCCESS;
}



