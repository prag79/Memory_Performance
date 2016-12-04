#ifndef TESTS_TERAS_TESTBENCH_H_
#define TESTS_TERAS_TESTBENCH_H_
#include"TestTeraSTop.h"
#include "TeraSMemoryChip.h"


using namespace CrossbarTeraSLib;

class TeraSTestBench
{
public:
	TeraSTestBench(uint32_t numCommands, uint32_t emulationCacheSize, \
		uint32_t pageSize, uint32_t pageNum, uint32_t bankNum,\
		uint32_t readTime, uint32_t programTime, uint32_t onfiSpeed)
	{
		try {
			testTop=new TestTeraSTop(sc_gen_unique_name("testTop"), numCommands, emulationCacheSize, pageSize, pageNum, bankNum, onfiSpeed);
			memDevice=new TeraSMemoryDevice("memDevice", emulationCacheSize, 2, bankNum, pageNum, pageSize, sc_time(readTime, SC_NS), sc_time(programTime, SC_NS), onfiSpeed, 0x0);
		}
		catch (...)
		{
			throw;
		}
		testTop->chipSelect(mChipSelect);
		memDevice->chipSelect(mChipSelect);
		testTop->onfiBus.bind(memDevice->onfiBus);
	}
	void erase()
	{
		delete testTop;
		delete memDevice;
	}
	~TeraSTestBench()
	{}

	private:
	TestTeraSTop* testTop;
	TeraSMemoryDevice* memDevice;
	sc_signal<bool> mChipSelect;
};



#endif