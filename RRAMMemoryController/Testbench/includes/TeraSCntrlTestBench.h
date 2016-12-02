/*******************************************************************
 * File : TeraSCntrlTestBench.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file is the top level file with details of 
 * module binding
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/
#ifndef TESTS_TERAS_CONTROLLER_TESTBENCH_H_
#define TESTS_TERAS_CONTROLLER_TESTBENCH_H_

#include "TeraSMemoryChip.h"
#include "TeraSController.h"
#include "TestbenchTop_mod.h"

using namespace CrossbarTeraSLib;

class TeraSCntrlTestBench
{
public:
	TeraSCntrlTestBench(uint32_t numCommands, uint32_t ioSize, uint32_t blockSize, uint32_t emulationCacheSize, \
		uint32_t pageSize, uint32_t bankNum, uint8_t numDie, uint32_t chanNum, uint32_t pageNum,
		uint32_t cmdQueueSize, uint8_t credit, uint32_t readTime, uint32_t programTime, uint32_t onfiSpeed,
		uint32_t queueFactor, uint32_t DDRSpeed, uint16_t seqLBAPct, uint16_t cmdPct, bool enMultiSim, bool mode, double cmdTransferTime, uint8_t numCores, bool enWrkld, std::string wrkloadFiles, uint32_t pollWaitTime)
		: mChanNum(chanNum)
	{
		try {

			mCwNum = numDie * bankNum * pageSize / emulationCacheSize;
			mQueueSize = chanNum * mCwNum * queueFactor;
			mCwCount = blockSize / emulationCacheSize;
			try {
				if (mCwCount == 0) {
					throw "Input Parameter Error: Code Word Size is less than Page Size";
				}
			}
			catch (const char* msg)
			{
				//throw "Bad Allocation";
				std::cerr << msg << endl;
			}
			mNumSlot = (uint16_t)(mQueueSize / mCwCount);
			if (mNumSlot > 256)
			{
				mNumSlot = 256;
			}
			try {
				if (mNumSlot == 0) {
					throw "Input Parameter Error: Maximum Number of Slots is zero";
				}
			}
			catch (const char* msg)
			{
				//throw "Bad Allocation";
				std::cerr << msg << endl;
			}
			try {
				if (mCwNum == 0) {
					throw "Input Parameter Error: Maximum Logical Banks Per Channel is zero";
				}
			}
			catch (const char* msg)
			{
				//throw "Bad Allocation";
				std::cerr << msg << endl;
			}
			cntrlr = new TeraSController(sc_gen_unique_name("cntrlr"), chanNum, blockSize, \
				emulationCacheSize, pageSize, mQueueSize, sc_time(readTime, SC_NS), sc_time(programTime, SC_NS), \
				credit, onfiSpeed, DDRSpeed, bankNum, numDie, pageNum, mNumSlot, cmdQueueSize, enMultiSim, mode, cmdTransferTime, ioSize);

			testBench = new TestBenchTop(sc_gen_unique_name("testBench"), ioSize, blockSize, \
				emulationCacheSize, numCommands, pageSize, bankNum, numDie, pageNum, chanNum, \
				DDRSpeed, mNumSlot, seqLBAPct, cmdPct, cmdQueueSize, enMultiSim, mode, numCores, enWrkld, wrkloadFiles, pollWaitTime);

			memChip = new TeraSMemoryDevice(sc_gen_unique_name("memDevice"), emulationCacheSize, \
				numDie, bankNum, pageNum, pageSize, sc_time(readTime, SC_NS), \
				sc_time(programTime, SC_NS), onfiSpeed, chanNum, blockSize, cmdQueueSize, enMultiSim, ioSize, false);

			for (uint8_t chanIndex = 0; chanIndex < chanNum; chanIndex++)
			{
				mChipSelect.push_back(new sc_signal<bool>(sc_gen_unique_name("chipSelect")));

				memChip->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
				cntrlr->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
				cntrlr->pOnfiBus.at(chanIndex)->bind(*(memChip->pOnfiBus.at(chanIndex)));
			}

			testBench->ddrInterface.bind(cntrlr->ddrInterface);
		}
		catch (const char* msg)
		{
			//throw "Bad Allocation";
			std::cerr << msg << endl;
		}
	}
	void erase()
	{
		
		delete cntrlr;
		delete memChip;
		
		delete testBench;
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			delete mChipSelect.at(chanIndex);
		}
	}
	~TeraSCntrlTestBench()
	{
		//erase();
	}

private:

	TestBenchTop* testBench;
	TeraSController* cntrlr;
	TeraSMemoryDevice* memChip;
	uint32_t mChanNum;
	std::vector<sc_signal<bool>* > mChipSelect;
	uint32_t mCwNum;
	uint16_t mCwBankNum;
	uint32_t mQueueSize;
	uint32_t mNumSlot;
	uint32_t mCwCount;
};



#endif