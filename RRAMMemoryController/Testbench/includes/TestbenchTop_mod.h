/*******************************************************************
 * File : TestTeraSTop.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Author: Pragnajit Datta Roy
 * Description: This file contains detail of TeraS Controller Test bench
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/

 
#ifndef __TESTBENCH_TOP_H__
#define __TESTBENCH_TOP_H__
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include "tlm.h"
#include "trafficGenerator.h"
#include "SlotManager.h"
#include "DataGenerator.h"
#include "TestCommon.h"
#include "math.h"
#include "Report.h"
#include "CommandGenerator.h"
#include "reporting.h"
#include "WorkLoadManager.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"
#include <queue>

static const char *testFile = "TestbenchTop.h";

class TestBenchTop : public sc_core::sc_module{
 

public:
	
    tlm_utils::simple_initiator_socket<TestBenchTop, DDR_BUS_WIDTH, tlm::tlm_base_protocol_types> ddrInterface;
	tlm_utils::tlm_quantumkeeper m_qk;
 
	TestBenchTop(sc_module_name nm, uint32_t ioSize, uint32_t blockSize, uint32_t cwSize, \
		uint32_t numCmd, uint32_t pageSize, uint32_t bankNum,
		uint8_t numDie, uint32_t pageNum, uint8_t chanNum, uint32_t ddrSpeed, \
		uint32_t slotNum, uint16_t seqLBAPct, uint16_t cmdPct, \
		uint32_t cmdQueueSize, bool enMultiSim, bool mode, \
		int numCore, bool enableWrkld, std::string wrkloadFiles, uint32_t pollWaitTime);

	/*Garbage Collector*/
	~TestBenchTop(){
		
		delete[] mData;
		delete[] mReadCompletionQueue;
		delete [] mCmdPayload;
		mLBAReport->closeFile();
		mBusUtilReport->closeFile();
		mIOPSReport->closeFile();
		mIOPSReportCsv->closeFile();
		mBusUtilReportCsv->closeFile();
		mLatencyRep->closeFile();
		mLatencyRepCsv->closeFile();
		mNumCmdReport->closeFile();
		mIoSizeReport->closeFile();
		delete mNumCmdReport;
		delete mLatencyRep;
		delete mLatencyRepCsv;
		delete mLBAReport;
		delete mBusUtilReport;
		delete mIOPSReport;
		delete mBusUtilReportCsv;
		delete mIOPSReportCsv;
		delete mIoSizeReport;
		mLogFileHandler.close();
	}
 
private:
	uint32_t mBlockSize, mCwSize, mNumCmd, mCmdCount;
	uint32_t mIoSize;
	uint32_t mWrkldIoSize;
	uint32_t mQueueDepth;
	uint8_t* mData;
	uint64_t mMemSize;
	uint32_t mDdrSpeed;
	uint32_t mChanMaskBits;
	uint32_t mBankMaskBits;
	uint32_t mPageMaskBits;
	uint32_t mLBAMaskBits;
	uint32_t mChanMask;
	uint32_t mBankMask;
	uint32_t mCwBankMask;
	uint64_t mLBAMask;
	uint32_t mPageMask;
	uint32_t mPollWaitTime;

	uint16_t mCwBankNum;
	uint32_t mNumWrites;
	uint32_t mNumReads;
	uint32_t mCmdQueueSize;
	uint32_t mSlotNum;

	uint32_t numCmdPerThread[8];
	uint32_t residualCmd[8];
	uint64_t mLba;
	uint64_t mNumCmdWrkld;
	uint16_t mSeqLBAPct;
	uint16_t mCmdPct;
	bool	 mEnMultiSim;
	bool     mMode;
	bool     mEnableWorkLoad;
	bool     mIsCmdCompleted;
	std::queue<uint16_t> mSlotQueue;
	std::vector<uint16_t>::iterator it;
	std::vector<uint16_t> mSlotContainer;
	std::vector<testCmdType> cmdQueue;
	uint16_t mCompletionCmdEntry[MAX_VALID_COMPLETION_ENTRIES + 1];
	uint8_t* mReadCompletionQueue;
	uint8_t* mCmdPayload;
	uint64_t mSlotTableEntry;
	uint8_t mValidCnt;
	

	double mCmdTransferTime;
	double mDataTransferTime;
	double mQueueCmdTransferTime;
	double mCompletionQueueTxTime;
	double mDDRBusUtilization;

	int mNumCore;

	std::vector<testCmdLatencyData> mLatency;
	std::vector<testCmdLatencyData> mQueueLatency;
	SlotManager mSlotManagerObj;
	DataGenerator mDataGenObj;
	CommandGenerator mCmdGenObj;
	WorkLoadManager mWrkLoad;
	trafficGenerator mTrafficGen;

	void testCaseWriteReadThread();
	
	void testCacheModelThread();

	/* Processes Commands and saves it in a queue to be submitted 
	   to the DIMM*/
	void submissionQueueThread();
	void sQThreadWorkload();

	bool pollStatusWaitTime(bool pollFlag)
	{
		if (pollFlag)
		{
			wait(mPollWaitTime, SC_NS);
			pollFlag = false;
		}				
		return pollFlag;
	}

	void testCaseFromWorkLoad();
	void popCmdFromSQ(std::vector<CmdQueueField>::iterator& sqPtr, \
		uint32_t& wrkldIoSize, uint32_t& lastCwCnt, uint8_t& numSlotReq,\
		bool& isNoCmdLeft, int16_t& slotReqIndex, cmdField& payload);
	void setCmdParameter(uint32_t ioSize, uint16_t& cwCount, uint32_t& lastCount, uint8_t& numSlotReq)
	{
		uint8_t remSlot;
		uint32_t tempCwCnt;
		/*Number of slots required */
		numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, ioSize, mCwSize, remSlot);
		
		if (ioSize >= mBlockSize)
		{
			tempCwCnt = (uint32_t)abs((double)mBlockSize / mCwSize);
			if (remSlot != 0)
			{
				lastCount = (uint32_t)abs((double)(cwCount - numSlotReq* (mBlockSize / mCwSize)));
			}
			else
			{
				lastCount = 0;
			}
			cwCount = tempCwCnt;
		}
		else if (ioSize < mBlockSize)
		{
			cwCount = ioSize / mCwSize;
			lastCount = 0;
		}
		numSlotReq += remSlot;
	}

	void sendWrkldCmdsQD(uint64_t& cmdIndex);

	void processCompletionQueue(bool& isSlotBusy);
	void pushSlotToList(uint16_t tags, uint16_t numSlot)
	{
		mSlotManagerObj.getFreeTags(tags);
		mSlotManagerObj.addFirstSlotNum(tags, numSlot);
		mLatency.at(tags).startDelay = sc_time_stamp().to_double();
	}

	void mcore_thread(int cpuNum, int numCore, uint32_t residualCmd);
	void multiThread();
	void multiThreadFromWorkLoad();
	void mcore_threadFromWorkLoad(int cpuNum, int numCmdPerThread, uint32_t residualCmd);
	uint32_t getDDRAddress(const uint16_t& slotNo, const uint8_t& cs, bool CASBL);
	//generate Cmd payload
	void genCommandPayload(bool cmdType, const uint64_t& lba, const uint16_t& slotNum, \
		const uint32_t& cwCount, const uint32_t& blockSize, uint64_t& effectiveLba, uint64_t& cmdPayload);
	void sendCommand(const uint32_t& ddrAddress, uint8_t* cmdDataPayload, \
		const uint32_t& size);
	void sendData(const uint32_t& ddrAddress, uint8_t* data,  \
		const uint32_t& size);
	void loadData(const uint32_t& ddrAddress, uint8_t* data,
		const uint32_t& size);
	void getCompletionQueue(uint8_t& queueCount, uint8_t& validCount);
	void readCompletionCommand(const uint32_t& ddrAddress, uint8_t* cmdDataPayload, \
		const uint32_t& size);
	void readCompletionQueue(const uint16_t& numSlot, bool& cmdStatus,
		const sc_time& time);
	uint8_t generateNoSlotReq(const uint32_t& rwCmdSize);
	uint32_t generateRwCmdSize();
	void checkData(const uint16_t& slotIndex, const uint16_t& rcvdCwCnt, const uint64_t& lba);
	void readCompletionQueueData(uint8_t& cmdIndex, uint16_t& slotIndex);

	bool getCmdTypeFromCmplQueue(uint8_t& cmdIndex)
	{
		return ((bool)((*((uint16_t*)mReadCompletionQueue + cmdIndex * COMPLETION_WORD_SIZE + 1) >> 15) & 0x1));
	}

	uint8_t getSlotFromCmplQueue(uint8_t& cmdIndex)
	{
		uint8_t slotNum;
		slotNum = ((*((uint16_t*)mReadCompletionQueue + (cmdIndex * COMPLETION_WORD_SIZE + 1)) >> 7) & 0xff);
		return slotNum;
	}
	uint8_t getCwCntFromCmplQueue(uint8_t& cmdIndex);

	void unpack(uint8_t* payload, uint16_t& slotNum, uint8_t& cwCnt, bool& cmd);
	void freeSlot(uint16_t& slotIndex, uint8_t slotCount);
	
	void freeSlot(uint16_t& slotIndex);
	void freeSlotWithWorkload(uint16_t& slotIndex);
	void emptySlot(uint16_t& slotIndex);
	void findStartLatency(double& startLatency, uint16_t slotNum);
	bool findQueueEntry(std::vector<CmdQueueField>::iterator& index, const uint16_t slotNum);

	void sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba);
	void sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba);
	void sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt);
	void sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt);
	void sendReadWriteCommands(uint64_t& cmdIndex);
	string appendParameters(string name, string format);

	void popSlotFromQueue(uint16_t slotIndex)
	{
		for (it = mSlotContainer.begin(); it != mSlotContainer.end(); ++it)
		{
			if (slotIndex == *it)
			{
				mSlotContainer.erase(it);
				it = mSlotContainer.begin();
				break;
			}
		}
	}
	uint32_t getBankMask();
	uint32_t getPageMask();
	uint32_t getChanMask();
	uint64_t getLBAMask();
	uint64_t getPayloadMask();
	
	Report*  mLBAReport;
	Report*  mNumCmdReport;
	Report*  mLBACmdReport;
	Report*  mBusUtilReport;
	Report*  mBusUtilReportCsv;
	Report*  mIOPSReport;
	Report*  mIOPSReportCsv;
	Report*  mLatencyRepCsv;
	Report*  mLatencyRep;
	Report*  mIoSizeReport;

	std::fstream mLogFileHandler;
	sc_mutex bus_arbiter;
	//sc_process_handle handle[8];
	uint32_t cmdCounter;
	bool mLogIOPS;
	sc_time mFirstCmdTime;

	sc_event mTrigCmdDispatchEvent;
	sc_event mTrigSubCmdEvent;

	//std::vector<CmdQueueField> mSubQueue;
	std::vector<CmdQueueField> mSubQueue;
	std::vector<CmdQueueField>::iterator mSqPtr;
	std::queue<uint16_t> mSlotIndexQueue;
	std::vector<int16_t> mCmdQueueIndex;
	uint32_t mFreeSpace;
	uint8_t mQueueCount;
	uint8_t mByteIndex;
	uint8_t mValidCount;

	bool mEnableSameBankTest;
};


#pragma region CONSTRUCTOR
TestBenchTop::TestBenchTop(sc_module_name nm, uint32_t ioSize, uint32_t blockSize, uint32_t cwSize, \
	uint32_t numCmd, uint32_t pageSize, uint32_t bankNum,
	uint8_t numDie, uint32_t pageNum, uint8_t chanNum, uint32_t ddrSpeed, \
	uint32_t slotNum, uint16_t seqLBAPct, uint16_t cmdPct, \
	uint32_t cmdQueueSize, bool enMultiSim, bool mode, \
	int numCore, bool enableWrkld, std::string wrkloadFiles, uint32_t pollWaitTime)
	: sc_module(nm)
	, mWrkLoad(wrkloadFiles)
	, mCmdGenObj(blockSize, cwSize, pageSize, bankNum, numDie, pageNum, \
	chanNum)
	, mDataGenObj(cwSize, ioSize)
	, mSlotManagerObj(slotNum)
	, mSlotNum(slotNum)
	, mIoSize(ioSize)
	, mBlockSize(blockSize)
	, mCwSize(cwSize)
	, mNumCmd(numCmd)
	, mDdrSpeed(ddrSpeed)
	, mChanMaskBits((uint32_t)log2(double(chanNum)))
	, mCwBankNum((uint16_t)((bankNum * pageSize) / cwSize))
	, mPageMaskBits((uint32_t)log2(double(pageNum)))
	, mSeqLBAPct(seqLBAPct)
	, mCmdPct(cmdPct)
	, mEnMultiSim(enMultiSim)
	, mCmdQueueSize(cmdQueueSize)
	, mCmdTransferTime((double)(COMMAND_BL * 1000) / (double)(2 * mDdrSpeed))
	, mCompletionQueueTxTime((double)(2 * COMMAND_BL * 1000) / (double)(2 * mDdrSpeed))
	, mDDRBusUtilization(0)
	, mQueueDepth(cmdQueueSize)
	, mMode(mode)
	//, numCmdPerThread(0)
	, mNumCore(numCore)
	, bus_arbiter("bus_arbiter")
	, cmdCounter(0)
	//, residualCmd(0)
	, mLogIOPS(false)
	, mLba(0)
	, mEnableWorkLoad(enableWrkld)
	, mNumCmdWrkld(0)
	, mFirstCmdTime(sc_time(0, SC_NS))
	, mWrkldIoSize(0)
	, mTrafficGen(65536, cwSize, 512, ioSize, cmdPct, seqLBAPct, numDie, bankNum, chanNum)
	, mPollWaitTime(pollWaitTime)
	, mCmdCount(0)
	, mEnableSameBankTest(false)
{
	srand((unsigned int)time(NULL));

	
	if (mCwBankNum > 2) {
		mBankMaskBits = (uint32_t)log2(double(mCwBankNum));
	}
	else {
		mBankMaskBits = 1;
	}

	mBankMask = getBankMask();
	mLBAMaskBits = mPageMaskBits + mBankMaskBits + mChanMaskBits + 1;
	mLBAMask = getLBAMask();
	mChanMask = getChanMask();
	mPageMask = getPageMask();

	mMemSize = (uint64_t)((uint64_t)bankNum * (uint64_t)numDie \
		* (uint64_t)(pageSize)* (uint64_t)(pageNum)* (uint64_t)(chanNum));

	

	for (uint32_t slotIndex = 0; slotIndex < mSlotNum; slotIndex++)
	{
		mLatency.push_back(testCmdLatencyData());
	}

	
	SC_HAS_PROCESS(TestBenchTop);

	if (!enableWrkld)
	{
		if (mNumCmd < (uint32_t)mNumCore)
		{
			mNumCore = mNumCmd;
		}
		if (mNumCmd / mNumCore >= 1)
		{
			for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
			{
				numCmdPerThread[numCores] = mNumCmd / mNumCore;
			}
		}
		else
		{
			for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
			{
				numCmdPerThread[numCores] = mNumCmd;
			}
		}

		uint32_t reminder = mNumCmd%mNumCore;
		for (uint32_t numCores = 0; numCores < reminder; numCores++)
		{
			residualCmd[numCores] = 1;
		}

	}

	if ((mMode == false) && (mEnableWorkLoad == true))
	{
		SC_THREAD(testCaseFromWorkLoad);
		sensitive << mTrigCmdDispatchEvent;
		dont_initialize();
		SC_THREAD(sQThreadWorkload);
	}
	else if ((mMode == true) && (mEnableWorkLoad == false))
	{
		SC_THREAD(multiThread);
		mCompletionQueueTxTime = (double)(COMMAND_BL * 1000) / (double)(2 * mDdrSpeed);
	}
	else if ((mMode == true) && (mEnableWorkLoad == true))
	{
		SC_THREAD(multiThreadFromWorkLoad);
		mCompletionQueueTxTime = (double)(COMMAND_BL * 1000) / (double)(2 * mDdrSpeed);
	}
	else
	{
		SC_THREAD(testCaseWriteReadThread);
		sensitive << mTrigCmdDispatchEvent;
		dont_initialize();
		mCompletionQueueTxTime = (double)(2 * COMMAND_BL * 1000) / (double)(2 * mDdrSpeed);
		
		SC_THREAD(submissionQueueThread);

	}
	

	string ctrlLogFile = "./Logs/TeraSTestBench";
	/*mWrkLoad.openReadModeFile();
	std::string typeCmd;
	uint64_t lba;
	uint32_t cwCnt;
	mWrkLoad.readFile(typeCmd, lba, cwCnt);
	mWrkldIoSize = 512 * cwCnt;
	mWrkLoad.closeFile();*/
	mWrkldIoSize = mBlockSize;
	try
	{
		if (mEnMultiSim){
			string filename = "./Reports/lba_report";
			mLBAReport = new Report(appendParameters(filename, ".log"));
			filename = "./Reports/DDR_bus_utilization";
			mBusUtilReport = new Report(appendParameters(filename, ".log"));
			mBusUtilReportCsv = new Report(appendParameters("./Reports/DDR_bus_utilization", ".csv"));
			filename = "./Reports/IOPS_utilization";
			mIOPSReport = new Report(appendParameters(filename, ".log"));
			mIOPSReportCsv = new Report(appendParameters(filename, ".csv"));
			ctrlLogFile = appendParameters(ctrlLogFile, ".log");
			filename = "./Reports/latency_TB_Report";
			mLatencyRep = new Report(appendParameters(filename, ".log"));
			mLatencyRepCsv = new Report(appendParameters(filename, ".csv"));
			filename = "./Reports/iosize_report";
			mIoSizeReport = new Report(appendParameters(filename, ".log"));
		}
		else{
			mLBAReport = new Report("./Reports/lba_report.log");
			mBusUtilReport = new Report("./Reports/DDR_bus_utilization.log");
			mBusUtilReportCsv = new Report("./Reports/DDR_bus_utilization.csv");
			mIOPSReport = new Report("./Reports/IOPS_utilization.log");
			mIOPSReportCsv = new Report("./Reports/IOPS_utilization.csv");
			mLatencyRep = new Report("./Reports/latency_TB_Report.log");
			mLatencyRepCsv = new Report("./Reports/latency_TB_Report.csv");
			mIoSizeReport = new Report("./Reports/iosize_report.log");
		}
		mLogFileHandler.open(ctrlLogFile, std::fstream::trunc | std::fstream::out);
		mNumCmdReport = new Report("./Reports/num_commands.log");
	}
	catch (std::bad_alloc& ba)
	{
		std::cerr << "TestbenchTop:Bad File allocation" << ba.what() << std::endl;
	}


	if (!mLBAReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "LBA_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mBusUtilReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "DDR_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mIOPSReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "IOPS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}

	if (!mBusUtilReportCsv->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "DDR_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mIOPSReportCsv->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "IOPS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}

	if (!mLatencyRep->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "LATENCY_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}

	if (!mLatencyRepCsv->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "LATENCY_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mNumCmdReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "NUM_COMMAND_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}

	if (!mIoSizeReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "IOSIZE_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(testFile, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	mData = new uint8_t[MAX_BLOCK_SIZE];
	memset(mData, 0x0, MAX_BLOCK_SIZE);

	mReadCompletionQueue = new uint8_t[MAX_COMPLETION_BYTES_READ];
	memset(mReadCompletionQueue, 0x0, MAX_COMPLETION_BYTES_READ);
	mCmdPayload = new uint8_t[8];

}

#pragma endregion

void TestBenchTop::unpack(uint8_t* payload, uint16_t& slotNum, uint8_t& cwCnt, bool& cmd)
{
	uint16_t* data = reinterpret_cast<uint16_t*> (payload);
	cwCnt = *data & 0x7f;
	slotNum = (uint16_t)(*data >> 7) & 0xff;
	cmd = (bool)((*data >> 15) & 0x1);
}
/** sendWrite
* Overloaded member function used to send Memory Write command and data to the controller.
* @return void
**/
void TestBenchTop::sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba)
{
	uint32_t blkSize;
	if (mIoSize >= mBlockSize)
	{
		blkSize = mBlockSize;
	}
	else
	{
		blkSize = mIoSize;
	}
	genCommandPayload(WRITE_CMD, lba, \
		numSlot, blkSize / mCwSize, blkSize, effectiveLba, cmdPayload);
	mDataGenObj.generateData(effectiveLba, mData, blkSize);

	uint32_t ddrAddress = getDDRAddress(numSlot, DATABUFFER0, BL8); // pass enum

	std::ostringstream msg;
	msg.str("");
	msg << "WRITE SEND: "
		<< " LBA: " << hex << (uint32_t)effectiveLba
		<< " Slot NUM: " << dec << (uint32_t)numSlot
		<< " Channel Number: " << hex << (effectiveLba & mChanMask)
		<< " CW Bank: " << hex << ((effectiveLba >> 4) & mBankMask)
		<< " Chip Select " << hex << ((effectiveLba >> 6) & 0x1)
		<< " Page Address " << hex << ((effectiveLba >> 7) & mPageMask);
	REPORT_INFO(testFile, __FUNCTION__, msg.str());

	sendData(ddrAddress, mData, blkSize);
	mSlotManagerObj.insertCmd(cmdPayload, numSlot);
	ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);
	sendCommand(ddrAddress, (uint8_t*)&cmdPayload, COMMAND_PAYLOAD_SIZE);
}

/** sendWrite
* Overloaded member function used to send Memory Write command and data to the controller.
* It is called when workload file is an input
* @return void
**/
void TestBenchTop::sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt)
{
	uint32_t blkSize;
	uint32_t wrkldIoSize = cwCnt * mCwSize;
	if (wrkldIoSize >= mBlockSize)
	{
		blkSize = mBlockSize;
	}
	else
	{
		blkSize = wrkldIoSize;
	}
	genCommandPayload(WRITE_CMD, lba, \
		numSlot, cwCnt, blkSize, effectiveLba, cmdPayload);
	mDataGenObj.generateData(effectiveLba, mData, mCwSize * cwCnt);

	uint32_t ddrAddress = getDDRAddress(numSlot, DATABUFFER0, BL8); // pass enum

	std::ostringstream msg;
	msg.str("");
	msg << "WRITE SEND: "
		<< " LBA: " << hex << (uint32_t)effectiveLba
		<< " Slot NUM: " << dec << (uint32_t)numSlot
		<< " Channel Number: " << hex << (effectiveLba & mChanMask)
		<< " CW Bank: " << hex << ((effectiveLba >> mChanMaskBits) & mBankMask)
		<< " Chip Select " << hex << ((effectiveLba >> (mBankMaskBits + mChanMaskBits)) & 0x1)
		<< " Page Address " << hex << ((effectiveLba >> (mBankMaskBits + mChanMaskBits + 1)) & mPageMask);
	REPORT_INFO(testFile, __FUNCTION__, msg.str());

	sendData(ddrAddress, mData, blkSize);
	mSlotManagerObj.insertCmd(cmdPayload, numSlot);
	ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);
	sendCommand(ddrAddress, (uint8_t*)&cmdPayload, COMMAND_PAYLOAD_SIZE);
}

/** sendRead
* Overloaded member function used to send Memory Read command to the controller.
* @return void
**/
void TestBenchTop::sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba)
{
	uint32_t blkSize;
	if (mIoSize >= mBlockSize)
	{
		blkSize = mBlockSize;
	}
	else
	{
		blkSize = mIoSize;
	}
	genCommandPayload(READ_CMD, lba, \
		numSlot, blkSize / mCwSize, blkSize, effectiveLba, cmdPayload);
	mSlotManagerObj.insertCmd(cmdPayload, numSlot);
	uint32_t ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);

	std::ostringstream msg;
	msg.str("");
	msg << "READ SEND: "
		<< " LBA: " << hex << (uint32_t)lba
		<< " Slot NUM: " << dec << (uint32_t)numSlot
		<< " Channel Number: " << hex << (lba & mChanMask)
		<< " CW Bank: " << hex << ((lba >> 4) & mCwBankMask)
		<< " Chip Select " << hex << ((lba >> 6) & 0x1)
		<< " Page Address " << hex << ((lba >> 7) & mPageMask)
		<< ":@Time: " << sc_time_stamp().to_double();
	REPORT_INFO(testFile, __FUNCTION__, msg.str());

	sendCommand(ddrAddress, (uint8_t*)&cmdPayload, COMMAND_PAYLOAD_SIZE);
}

/** sendRead
* Overloaded member function used to send Memory Read command to the controller.
* It is called when workload file is an input
* @return void
**/
void TestBenchTop::sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt)
{
	uint32_t blkSize;
	uint32_t wrkldIoSize = cwCnt * mCwSize;
	if (wrkldIoSize >= mBlockSize)
	{
		blkSize = mBlockSize;
	}
	else
	{
		blkSize = wrkldIoSize;
	}
	genCommandPayload(READ_CMD, lba, \
		numSlot, cwCnt, blkSize, effectiveLba, cmdPayload);
	mSlotManagerObj.insertCmd(cmdPayload, numSlot);
	uint32_t ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);

	std::ostringstream msg;
	msg.str("");
	msg << "READ SEND: "
		<< " LBA: " << hex << (uint32_t)lba
		<< " Slot NUM: " << dec << (uint32_t)numSlot
		<< " Channel Number: " << hex << (lba & mChanMask)
		<< " CW Bank: " << hex << ((lba >> 4) & mCwBankMask)
		<< " Chip Select " << hex << ((lba >> 6) & 0x1)
		<< " Page Address " << hex << ((lba >> 7) & mPageMask)
		<< ":@Time: " << sc_time_stamp().to_double();
	REPORT_INFO(testFile, __FUNCTION__, msg.str());

	sendCommand(ddrAddress, (uint8_t*)&cmdPayload, COMMAND_PAYLOAD_SIZE);
}

/** getBankMask
* calculates bank mask bits based on no. of bits
* @return uint32_t
**/
uint32_t TestBenchTop::getBankMask()
{
	uint32_t bankMask = 0;

	for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
		bankMask |= (0x1 << bitIndex);
	return bankMask;

}

/** getPageMask
* calculates Page mask bits based on no. of bits
* @return uint32_t
**/
uint32_t TestBenchTop::getPageMask()
{
	uint32_t pageMask = 0;

	for (uint8_t bitIndex = 0; bitIndex < mPageMaskBits; bitIndex++)
		pageMask |= (0x1 << bitIndex);
	return pageMask;

}

/** getChanMask
* calculates Channel mask bits based on no. of bits
* @return uint32_t
**/
uint32_t TestBenchTop::getChanMask()
{
	uint32_t chanMask = 0;

	for (uint8_t bitIndex = 0; bitIndex < mChanMaskBits; bitIndex++)
		chanMask |= (0x1 << bitIndex);
	return chanMask;

}

/** getLBAMask
* calculates LBA mask bits based on no. of bits
* @return uint32_t
**/
uint64_t TestBenchTop::getLBAMask()
{
	uint64_t lbaMask = 0;

	for (uint64_t bitIndex = 0; bitIndex < mLBAMaskBits; bitIndex++)
		lbaMask |= ((uint64_t)0x1 << bitIndex);
	return lbaMask;

}

/** getPayloadMask
* calculates Payload mask bits based on lba mask bits
* @return uint32_t
**/
uint64_t TestBenchTop::getPayloadMask()
{
	uint64_t pldMask = 0;

	for (uint64_t bitIndex = 0; bitIndex < (mLBAMaskBits + 19); bitIndex++)
		pldMask |= ((uint64_t)0x1 << bitIndex);
	return pldMask;

}

/** testCaseWriteReadThread()
* sc_thread;creates cmd payload(read or write)
* and sends  to the controller .also generates
* LBA(sequential or random)
* @return void
**/
void TestBenchTop::testCaseWriteReadThread()
{

	uint16_t slotIndex = 0;
	std::ostringstream msg;
	uint64_t lba = 0;
	uint64_t cmdIndex = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t ddrAddress = 0;
	uint16_t numSlot;
	bool logIOPS = false;
	bool firstSlotIndexed = false;
	uint32_t numSlotsReserved = 0;
	bool fetchCompletionQueue = false;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t slotReqIndex = 0;
	uint16_t tags;
	uint8_t remSlot;
	//uint8_t numSlotCount = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);//Find number of slots needed to send command
	bool pollFlag = true;
	bool slotNotFree = false;
	cmdField payload;
	uint64_t maxNumCmd;
	std::vector<CmdQueueField>::iterator sqPtr;
	sc_core::sc_time delay = sc_time(0, SC_NS);
	//If Number of commands are less than QD
	//make QD equal to Number of commands
	if (mCmdQueueSize > mNumCmd)
	{
		mCmdQueueSize = mNumCmd;
	}
	mQueueDepth = mCmdQueueSize;

	uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);
	
	if (mIoSize >= mBlockSize)
	{
		lbaSkipGap = mBlockSize / mCwSize;
		maxNumCmd = mNumCmd * (mIoSize / mBlockSize);
	}
	else
	{
		lbaSkipGap = mIoSize / mCwSize;
		maxNumCmd = mNumCmd;
	}

	sendReadWriteCommands(cmdIndex);
	
	//This loop is run after commands equal to QD is sent
	while (!mSlotContainer.empty())
	{
		uint8_t validCount = 0;
		CmdQueueField queueEntry;
		
		getCompletionQueue(queueCount, validCount);

		//if commands are completed
		if (queueCount)
		{
			sc_time currTime = sc_time_stamp();
			for (uint8_t CmplQIndex = 0; CmplQIndex < validCount; CmplQIndex++)
			{
				uint16_t slotIndex = 0;
				
				//Read one slot
				readCompletionQueueData(CmplQIndex, slotIndex);
				freeSlot(slotIndex);
				
				popSlotFromQueue(slotIndex);
				if (cmdIndex < maxNumCmd)
				{
					if (mCmdCount < mNumCmd)
					{
						wait(mTrigCmdDispatchEvent);
					}
					//if slot is free and commands are still left to process					
					if (!slotNotFree)
					{
						/*Find the SQ entry which needs to be dispatched*/
						for (sqPtr = mSubQueue.begin(); sqPtr != mSubQueue.end(); sqPtr++)
						{
							if ((sqPtr->isDispatched == false) && (sqPtr->isCmdDone == false))
							{
								payload = sqPtr->payload;
								break;
							}
						}
						
					}
					//Send sub Commands
					while (slotReqIndex < numSlotReq)
					{
						if (mSlotManagerObj.getAvailableSlot(numSlot))
						{
							if (!firstSlotIndexed)
							{
								firstSlotIndexed = true;
								mSlotManagerObj.getFreeTags(tags);
								mSlotManagerObj.addFirstSlotNum(tags, numSlot);
								sqPtr->isDispatched = true;
							}
							mSlotContainer.push_back(numSlot);
							slotNotFree = false;
							mSlotManagerObj.addSlotToList(tags, numSlot);
							mLogFileHandler << "TestCase: "
								<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
								<< " Slot Number= " << dec << (uint32_t)numSlot
								<< endl;

							uint64_t tempLba = payload.lba + lba;
							mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");
							if (payload.d == ioDir::write)
							{
								sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
							}
							else
							{
								sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
							}

							cmdIndex++;
							lba += lbaSkipGap;
							slotReqIndex++;
							
						}//if (mSlotManagerObj.getAvailableSlot(numSlot))
						else
						{
							slotNotFree = true;
							break;
						}//else

					}//while (slotReqIndex < (int16_t)numSlotCount)

					if (!slotNotFree)
					{
						//if all the sub commands are sent
						slotReqIndex = 0;
						lba = 0;
						firstSlotIndexed = false;
					}
				}//if (cmdIndex < mNumCmd)
			}//for(uint8_t CompCmdIndex = 0; CompCmdIndex < validCount; CompCmdIndex++)
		}//if (queueCount)
		else
		{
			//if command is not there in the completion queue
			// then wait for some time and poll again
			wait(mPollWaitTime, SC_NS);
		}
	
	}//while (!mSlotQueue.empty())

	if (mSlotContainer.empty())
	{

		sc_time lastCmdTime = sc_time_stamp();
		sc_time delayTime = lastCmdTime - mFirstCmdTime;
		mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
		mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
		logIOPS = true;
		mTrafficGen.closeReadModeFile();
		sc_stop();
	}
}

/** sendReadWriteCommands(uint32_t& cmdIndex)
* creates cmd payload(read or write)
* and sends  to the controller.also generates
* LBA(sequential or random)
* Called by testCaseReadWriteThread()
* @return void
**/
void TestBenchTop::sendReadWriteCommands(uint64_t& cmdIndex)
{

	int32_t queueIndex = 0;
	uint16_t numSlot = 0;
	testCmdType cmd;
	bool firstSlotIndexed = false;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint64_t lba = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint8_t remSlot;
	bool isSlotBusy = false;
	CmdQueueField queueEntry;
	cmdField payload;

	for (std::vector<CmdQueueField>::iterator it = mSubQueue.begin(); it != mSubQueue.end(); it++)
	{
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);
		uint16_t firstSlot;
		uint16_t tags;
		int16_t slotReqIndex = 0;
		
		uint8_t validCount = 0;
		
		/*Fetch data from the front of the vector*/
		queueEntry = *it;// mSubQueue.front();
		//mSubQueue.pop();
		payload = queueEntry.payload;
		//mTrafficGen.getCommands(payload);
		while (slotReqIndex < (int16_t)numSlotReq)
		{
			if (mSlotManagerObj.getAvailableSlot(numSlot))
			{
				//mSlotQueue.push(numSlot);
				if (!firstSlotIndexed)
				{
					firstSlot = numSlot;
					mSlotManagerObj.getFreeTags(tags);
					firstSlotIndexed = true;
					mSlotManagerObj.addFirstSlotNum(tags, numSlot);

					/*SQ is indexed using slot number*/
					it->isDispatched = true;
					it->slotNum = numSlot;
		
				}

				mSlotManagerObj.addSlotToList(tags, numSlot);
				mSlotContainer.push_back(numSlot);
				uint64_t tempLba = payload.lba + lba;

				mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");

				if (payload.d == ioDir::write)
				{
					cmd = WRITE_CMD;
				}
				else
				{
					cmd = READ_CMD;
				}
				mLogFileHandler << "TestCase: "
					<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< " Slot Number= " << dec << (uint32_t)numSlot
					<< endl;

				if (cmd == WRITE_CMD)
				{
					sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
				}
				else if (cmd == READ_CMD)
				{
					sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
				}
				cmdIndex++;
				lba += lbaSkipGap;
				slotReqIndex++;
			}//if (mSlotManagerObj.getAvailableSlot(numSlot))
			else
			{
				//Ping completion queue till any slot is free
				processCompletionQueue(isSlotBusy);
			}
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		lba = 0;
		queueIndex++;
		//mSqPtr = it;
	}//while(queueIndex < (int32_t)mCmdQueueSize)
}

#pragma region WORKLOAD_TEST_CASE
void TestBenchTop::testCaseFromWorkLoad()
{
	std::ostringstream msg;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint64_t lba = 0;
	uint32_t ddrAddress = 0;
	std::string cmdString;
	bool logIOPS = false;

	uint32_t cwCnt;
	uint64_t cmdIndex = 0;//used to calculate IOPS
	int16_t slotReqIndex = 0;
	uint16_t tags;
	bool pollFlag3 = true;
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint16_t numSlot = 0;
	uint32_t lastCwCnt;
	uint16_t slotIndex = 0;
	bool firstSlotIndexed = false;
	uint8_t numSlotReq;
	sc_core::sc_time delay = sc_time(0, SC_NS);
	std::vector<CmdQueueField>::iterator sqPtr;
	bool slotNotFree = false;
	bool isNoCmdLeft = false;
	cmdField payload;
	uint32_t wrkldIoSize;

	if (mCmdQueueSize > mSlotNum)
	{
		mCmdQueueSize = mSlotNum;
	}

	mQueueDepth = mCmdQueueSize;

	sc_time firstCmdTime = sc_time_stamp();

	bool pollFlag2 = true;
	//loop till commands till the configured queue depth
	// is sent , exit if end of file is reached

	sendWrkldCmdsQD(cmdIndex);

	//poll for empty slots and keep sending commands till
	//end of file is reached
	while (!mSlotContainer.empty() )
	{
		/*if (!mWrkLoad.isEOF())
		{*/
		if (pollFlag2)
		{
			wait(mPollWaitTime, SC_NS);
			pollFlag2 = false;
		}
		getCompletionQueue(queueCount, validCount);

		if (queueCount)
		{
			for (uint8_t CmplQIndex = 0; CmplQIndex < validCount; CmplQIndex++)
			{
				//Read one slot
				readCompletionQueueData(CmplQIndex, slotIndex);
				popSlotFromQueue(slotIndex);
				freeSlot(slotIndex);
				
				
				//if (!mWrkLoad.isEOF())
				//{
					mIsCmdCompleted = false;
					if (!slotNotFree)
					{
						/*Find the SQ entry which needs to be dispatched*/
						popCmdFromSQ(sqPtr, wrkldIoSize, lastCwCnt, numSlotReq, isNoCmdLeft, slotReqIndex, payload);
				
					}
					/*If there is no command left in the submission queue to be
					dispatched then do not execute the while loop*/

					if (!isNoCmdLeft)
					{
						
						while (slotReqIndex < (int16_t)numSlotReq)
						{
							if ((numSlotReq - slotReqIndex) == 1)
							{
								if (lastCwCnt != 0)
									cwCnt = lastCwCnt;

							}
							if (mSlotManagerObj.getAvailableSlot(numSlot))
							{
								slotNotFree = false;
								if (!firstSlotIndexed)
								{
									mSlotManagerObj.getFreeTags(tags);
									firstSlotIndexed = true;
									mSlotManagerObj.addFirstSlotNum(tags, numSlot);
									sqPtr->isDispatched = true;
									sqPtr->slotNum = numSlot;
								}
								/*Push the slot number into a vector
								This is done to keep track of the number
								of slots that needs to be processed*/
								mIoSizeReport->writeFile("IOSIZE", sc_time_stamp().to_double(), 512 * cwCnt, " ");
								mSlotContainer.push_back(numSlot);
								mSlotManagerObj.addSlotToList(tags, numSlot);
								mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), lba, " ");

								mLogFileHandler << "TestCase: "
									<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
									<< " Slot Number= " << dec << (uint32_t)numSlot
									<< endl;

								if (payload.d == ioDir::read)
								{
									sendRead(effectiveLba, cmdPayload, numSlot, payload.lba, payload.cwCnt);
								}
								else
								{
									sendWrite(effectiveLba, cmdPayload, numSlot, payload.lba, payload.cwCnt);
								}

								cmdIndex++;
								slotReqIndex++;
								lbaSkipGap = cwCnt;
								lba += lbaSkipGap;
							}//if (mSlotManagerObj.getAvailableSlot(numSlot))
							else
							{
								slotNotFree = true;
								break;
							}//else
						}//while (slotReqIndex < (int16_t)numSlotCount)
						//if all the sub commands are sent

						if (!slotNotFree)
						{
							slotReqIndex = 0;
							lba = 0;
    						firstSlotIndexed = false;
						}
					}
				//}//if (!mWrkLoad.isEOF())
			}//for
		}//if (queueCount)
		else
		{
			wait(mPollWaitTime, SC_NS);
		}
	}//while (!mSlotQueue.empty())
	//}//while (!mWrkLoad.isEOF())

	if (mSlotContainer.empty() && mWrkLoad.isEOF())
	{
		sc_time lastCmdTime = sc_time_stamp();
		sc_time delayTime = lastCmdTime - firstCmdTime;
		mIOPSReport->writeFile(delayTime.to_double(), (double)(cmdIndex * 1000 / delayTime.to_double()), " ");
		mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(cmdIndex * 1000 / delayTime.to_double()), ",");
		mNumCmdReport->writeFile(cmdIndex);
		sc_stop();
	}

}

void TestBenchTop::popCmdFromSQ(std::vector<CmdQueueField>::iterator& sqPtr, uint32_t& wrkldIoSize,\
	uint32_t& lastCwCnt, uint8_t& numSlotReq,\
	bool& isNoCmdLeft, int16_t& slotReqIndex, cmdField& payload)
{
	for (sqPtr = mSubQueue.begin(); sqPtr != mSubQueue.end(); sqPtr++)
	{
		if ((sqPtr->isDispatched == false) && (sqPtr->isCmdDone == false))
		{
			payload = sqPtr->payload;
			isNoCmdLeft = false;
			/*Calculate IOSize*/
			wrkldIoSize = 512 * payload.cwCnt;

			/*Calculate number of slots required for the command*/
			setCmdParameter(wrkldIoSize, payload.cwCnt, lastCwCnt, numSlotReq);
			slotReqIndex = 0;
			break;
		}
		else
		{
			isNoCmdLeft = true;
		}
	}
}
void TestBenchTop::sendWrkldCmdsQD( uint64_t& cmdIndex)
{

	uint32_t queueIndex = 0;
	uint16_t numSlot = 0;

	bool firstSlotIndexed = false;
	uint8_t numSlotReq;

	uint32_t wrkldIoSize;
	uint32_t lastCwCnt;
	testCmdType cmd;
	uint16_t tags;
	int16_t slotReqIndex = 0;
	bool slotNotFree = false;
	bool isSlotBusy = false;
	CmdQueueField queueEntry;
	cmdField payload;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t lbaSkipGap = 0;
	for (std::vector<CmdQueueField>::iterator it = mSubQueue.begin(); it != mSubQueue.end(); it++)
	{
		/*Fetch data from the front of the vector*/
		queueEntry = *it;

		/*Fetch the payload*/
		payload = queueEntry.payload;

		/*Calculate IOSize*/
		wrkldIoSize = 512 * payload.cwCnt;

		/*Calculate the number of slots required*/
		setCmdParameter(wrkldIoSize, payload.cwCnt, lastCwCnt, numSlotReq);
		slotReqIndex = 0;

		while (slotReqIndex < (int16_t)numSlotReq)
		{
			/*If last sub command then cw Cnt is the residue*/
			if ((numSlotReq - slotReqIndex) == 1)
			{
				if (lastCwCnt != 0)
					payload.cwCnt = lastCwCnt;
			}

			if (mSlotManagerObj.getAvailableSlot(numSlot))
			{

				if (!firstSlotIndexed)
				{
					mSlotManagerObj.getFreeTags(tags);
					firstSlotIndexed = true;
					mSlotManagerObj.addFirstSlotNum(tags, numSlot);
					it->isDispatched = true;
					it->slotNum = numSlot;

				}
				slotNotFree = false;
				/* List of sub commands that make up a command
				add it to the list*/
				mSlotManagerObj.addSlotToList(tags, numSlot);

				/*Used to track no. of slots done*/
				mSlotContainer.push_back(numSlot);

				mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), payload.lba, " ");
				mIoSizeReport->writeFile("IOSIZE", sc_time_stamp().to_double(), 512 * payload.cwCnt, " ");

				if (payload.d == ioDir::read)
				{
					cmd = READ_CMD;
				}
				else {
					cmd = WRITE_CMD;
				}

				mLogFileHandler << "TestCase: "
					<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< " Slot Number= " << dec << (uint32_t)numSlot
					<< endl;

				if (cmd == WRITE_CMD)
				{
					sendWrite(effectiveLba, cmdPayload, numSlot, payload.lba, payload.cwCnt);
				}

				if (cmd == READ_CMD)
				{
					sendRead(effectiveLba, cmdPayload, numSlot, payload.lba, payload.cwCnt);
				}

				/* Increment Command Count*/
				cmdIndex++;
				lbaSkipGap = (uint8_t)payload.cwCnt;
				payload.lba += lbaSkipGap;
				slotReqIndex++;
			}//if (mSlotManagerObj.getAvailableSlot(numSlot))
			else
			{
				processCompletionQueue(isSlotBusy);

			}//else
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		queueIndex++;

	}//while(queueIndex < mCmdQueueSize)
}

void TestBenchTop::sQThreadWorkload()
{
	/*Open the workload file*/
	mWrkLoad.openReadModeFile();
	std::string cmdType;
	uint32_t cwCnt;
	uint64_t lba;

	cmdField payload;
	uint32_t cmdIndex = 0;

	mFreeSpace = mQueueDepth;
	mFirstCmdTime = sc_time_stamp();

	for (uint32_t queueIndex = 0; queueIndex < mQueueDepth; queueIndex++)
	{
		if (!mWrkLoad.isEOF())
		{
			mWrkLoad.readFile(cmdType, lba, cwCnt);
			CmdQueueField queueEntry;

			queueEntry.startDelay = sc_time_stamp().to_double();
			queueEntry.payload.cwCnt = cwCnt;
			queueEntry.payload.lba = lba;
			if (cmdType == "R")
			{
				queueEntry.payload.d = ioDir::read;
			}
			else
				queueEntry.payload.d = ioDir::write;

			queueEntry.isDispatched = false;
			queueEntry.isCmdDone = false;
			mSubQueue.push_back(queueEntry);
			//mCmdQueueIndex.at(cmdIndex) = cmdIndex;
			mCmdCount++;
			cmdIndex++;
		}
		//}
	}
	mTrigCmdDispatchEvent.notify(SC_ZERO_TIME);
	wait(mTrigSubCmdEvent);
	while (1)
	{
		
			while (!mSlotIndexQueue.empty() && !mWrkLoad.isEOF())
			{

				uint16_t slotNum = mSlotIndexQueue.front();
				mSlotIndexQueue.pop();

				mWrkLoad.readFile(cmdType, lba, cwCnt);

				for (std::vector<CmdQueueField>::iterator it = mSubQueue.begin(); it != mSubQueue.end(); it++)
				{
					if ((it->slotNum == slotNum) && (it->isDispatched == true) && (it->isCmdDone == true))
					{
						it->payload.cwCnt = cwCnt;
						it->payload.lba = lba;
						if (cmdType == "R")
						{
							it->payload.d = ioDir::read;
						}
						else
							it->payload.d = ioDir::write;

						it->startDelay = sc_time_stamp().to_double();
						it->isDispatched = false;
						it->isCmdDone = false;

						break;
					}


				}
				cmdIndex++;
				mCmdCount++;

			}
			mFreeSpace = 0;
			mTrigCmdDispatchEvent.notify(SC_ZERO_TIME);
			wait(mTrigSubCmdEvent);
		
			if (mWrkLoad.isEOF())
			{
				wait();
			}

	}
}
#pragma endregion

void TestBenchTop::mcore_threadFromWorkLoad(int cpuNum, int numCmdPerThread, uint32_t residualCmd)
{
	uint64_t lba = 0;
	testCmdType cmd;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	//uint32_t numCmdPerThread = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	sc_time waitTime = sc_time(0, SC_NS);
	bool cmdStatus = false;
	std::string cmdType;
	uint32_t cwCnt;
	//uint32_t wrkldIoSize = 0;
	bool firstSlotIndexed = false;
	uint8_t remSlot;
	uint32_t tempCwCnt;
	uint32_t lastCwCnt;
	uint16_t numSlot = cpuNum;
	numCmdPerThread += residualCmd;

	mWrkLoad.openReadModeFile();
	for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)numCmdPerThread; cmdIndex++)
	{
		mWrkLoad.readFile(cmdType, lba, cwCnt);
		/*Calculate IOSize*/
		uint32_t wrkldIoSize = 512 * cwCnt;

		/*Number of slots required */
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, wrkldIoSize, mCwSize, remSlot);
		if (wrkldIoSize >= mBlockSize)
		{
			tempCwCnt = (uint32_t)abs((double)mBlockSize / mCwSize);
			if (remSlot != 0)
			{
				lastCwCnt = (uint32_t)abs((double)(cwCnt - numSlotReq * (mBlockSize / mCwSize)));
			}
			else
			{
				lastCwCnt = 0;
			}
			cwCnt = tempCwCnt;
		}
		else if (wrkldIoSize < mBlockSize)
		{
			cwCnt = wrkldIoSize / mCwSize;
			lastCwCnt = 0;
		}
		numSlotReq += remSlot;

		uint16_t tags;
		int16_t slotReqIndex = 0;
		while (slotReqIndex < (int16_t)numSlotReq)
		{
			if (!firstSlotIndexed)
			{
				mSlotManagerObj.getFreeTags(tags);
				firstSlotIndexed = true;
				mSlotManagerObj.addFirstSlotNum(tags, numSlot);
				mLatency.at(tags).startDelay = sc_time_stamp().to_double();
			}
			mSlotManagerObj.addSlotToList(tags, numSlot);
			mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), lba, " ");
			if (cmdType == "R")
			{
				cmd = READ_CMD;
			}
			else {
				cmd = WRITE_CMD;
			}

			mLogFileHandler << "TestCase: "
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< " Slot Number= " << dec << (uint32_t)numSlot
				<< " CPU Number" << dec << cpuNum
				<< endl;

			if (cmd == WRITE_CMD)
			{
				bus_arbiter.lock();
				if ((numSlotReq - slotReqIndex) == 1)
				{
					if (lastCwCnt != 0)
						cwCnt = lastCwCnt;

				}
				sendWrite(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}

			if (cmd == READ_CMD)
			{
				bus_arbiter.lock();
				if ((numSlotReq - slotReqIndex) == 1)
				{
					if (lastCwCnt != 0)
						cwCnt = lastCwCnt;

				}
				sendRead(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}
			lbaSkipGap = cwCnt;
			lba += lbaSkipGap;
			slotReqIndex++;

			while (1)
			{
				readCompletionQueue(numSlot, cmdStatus, waitTime);
				waitTime = sc_time(mPollWaitTime, SC_NS);
				if (cmdStatus)
				{
					if (cmd == READ_CMD)
					{
						//int cwCnt = (mBlockSize / mCwSize);
						bus_arbiter.lock();
						checkData(numSlot, cwCnt, lba);
						bus_arbiter.unlock();
					}
					uint16_t tags;
					if (mSlotManagerObj.removeSlotFromList(tags, numSlot))
					{
						if (mSlotQueue.size() != 0)
							mSlotQueue.pop();
						if (mSlotManagerObj.getListSize(tags) == 0)
						{
							mLatency.at(tags).endDelay = sc_time_stamp().to_double();
							mLatency.at(tags).latency = mLatency.at(tags).endDelay - mLatency.at(tags).startDelay;
							mSlotManagerObj.resetTags(tags);
							uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
							mLatencyRep->writeFile(numSlot, mLatency.at(tags).latency / 1000, " ");
							mLatencyRepCsv->writeFile(numSlot, mLatency.at(tags).latency / 1000, ",");
							//cmdCounter++;
						}
						mSlotManagerObj.freeSlot(numSlot);
					}
					break;
				}
			}//while(1)
			//}//else
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		cmdCounter++;
	}//for (uint32_t cmdIndex = 0; 
	/*if (cmdCounter == mNumCmdWrkld)
	{
	if (mLogIOPS == false)
	{
	sc_time lastCmdTime = sc_time_stamp();
	sc_time delayTime = lastCmdTime - mFirstCmdTime;
	mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
	mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
	mLogIOPS = true;
	}
	wait(500, SC_NS);
	sc_stop();

	}*/

}

void TestBenchTop::multiThreadFromWorkLoad()
{
	mWrkLoad.openReadModeFile();

	std::string text;
	while (!mWrkLoad.isEOF())
	{
		mWrkLoad.readLine(text);
		mNumCmdWrkld++;
	}
	mWrkLoad.closeFile();
	//mNumCmdWrkld--;
	if (mNumCmdWrkld < (uint64_t)mNumCore)
	{
		mNumCore = (uint32_t)mNumCmdWrkld;
	}
	if (mNumCmdWrkld / mNumCore >= 1)
	{
		for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
		{
			numCmdPerThread[numCores] = (uint32_t)mNumCmdWrkld / mNumCore;
		}
	}
	else
	{
		for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
		{
			numCmdPerThread[numCores] = (uint32_t)mNumCmdWrkld;
		}
	}
	residualCmd[0] = (uint32_t)mNumCmdWrkld % mNumCore;
	for (uint32_t numCores = 1; numCores < (uint32_t)mNumCore; numCores++)
	{
		residualCmd[numCores] = 0;
	}
	mFirstCmdTime = sc_time_stamp();

	for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
	{
		sc_spawn(sc_bind(&TestBenchTop::mcore_threadFromWorkLoad, \
			this, numCores, numCmdPerThread[numCores], residualCmd[numCores]), sc_gen_unique_name("mcore_threadFromWorkLoad"));
	}

	while (1)
	{
		if (cmdCounter == mNumCmdWrkld)
		{

			if (mLogIOPS == false)
			{
				sc_time lastCmdTime = sc_time_stamp();
				sc_time delayTime = lastCmdTime - mFirstCmdTime;
				mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmdWrkld * 1000 / delayTime.to_double()), " ");
				mLogIOPS = true;
			}
			wait(mPollWaitTime, SC_NS);
			break;
		}
		else
		{
			wait(mPollWaitTime, SC_NS);
		}
	}
	sc_stop();
}

void TestBenchTop::mcore_thread(int cpuNum, int numCmdPerThread, uint32_t residualCmd)
{
	uint64_t lba = 0;
	testCmdType cmd;
	uint16_t numSlot = (uint16_t)cpuNum;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	//uint32_t numCmdPerThread = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	sc_time waitTime = sc_time(0, SC_NS);
	bool cmdStatus = false;
	bool firstSlotIndexed = false;
	cmdField payload;
	uint8_t remSlot;
	numCmdPerThread += residualCmd;
		
	mTrafficGen.openReadModeFile();
	
	uint64_t cmdCount = 0;
	/*if (mCmdType == 0){
	cmd = WRITE;
	}
	else if (mCmdType == 1){
	cmd = READ;
	}*/

	for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)numCmdPerThread; cmdIndex++)
	{
		mTrafficGen.readCommand(payload);
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);
		if (mIoSize >= mBlockSize)
		{
			lbaSkipGap = mBlockSize / mCwSize;
		}
		else
		{
			lbaSkipGap = mIoSize / mCwSize;
		}


		uint16_t tags;
		int16_t slotReqIndex = 0;
		while (slotReqIndex < (int16_t)numSlotReq)
		{
			if (!firstSlotIndexed)
			{
				mSlotManagerObj.getFreeTags(tags);
				firstSlotIndexed = true;
				mSlotManagerObj.addFirstSlotNum(tags, numSlot);
				mLatency.at(tags).startDelay = sc_time_stamp().to_double();
			}
			mSlotManagerObj.addSlotToList(tags, numSlot);
			uint64_t tempLba = payload.lba;
			mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");

			mLogFileHandler << "TestCase: "
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< " Slot Number= " << dec << (uint32_t)numSlot
				<< " CPU Number" << dec << cpuNum
				<< endl;
			if (payload.d == ioDir::read)
			{
				cmd = READ_CMD;
			}
			else
			{
				cmd = WRITE_CMD;
			}
			if (cmd == WRITE_CMD)
			{
				bus_arbiter.lock();
				sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
				slotReqIndex++;
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}

			if (cmd == READ_CMD)
			{
				bus_arbiter.lock();
				sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
				slotReqIndex++;
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}
			
			bool pollFlag = true;
			while (1)
			{
				if (pollFlag)
				{
					wait(mPollWaitTime, SC_NS);
					pollFlag = false;
				}
				readCompletionQueue(numSlot, cmdStatus, waitTime);

				waitTime = sc_time(mPollWaitTime, SC_NS);
				if (cmdStatus)
				{
					
					if (cmd == READ_CMD)
					{
						int cwCnt = (mBlockSize / mCwSize);
						bus_arbiter.lock();
						checkData(numSlot, cwCnt, payload.lba);
						bus_arbiter.unlock();
					}
					uint16_t tags;
					if (mSlotManagerObj.removeSlotFromList(tags, numSlot))
					{
						if (mSlotQueue.size() != 0)
							mSlotQueue.pop();
						if (mSlotManagerObj.getListSize(tags) == 0)
						{
							mLatency.at(tags).endDelay = sc_time_stamp().to_double();
							mLatency.at(tags).latency = mLatency.at(tags).endDelay - mLatency.at(tags).startDelay;
							mSlotManagerObj.resetTags(tags);
							uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
							mLatencyRep->writeFile(numSlot, mLatency.at(tags).latency / 1000, " ");
							mLatencyRepCsv->writeFile(numSlot, mLatency.at(tags).latency / 1000, ",");
						}
						mSlotManagerObj.freeSlot(numSlot);
					}
					break;
				}
			}//while(1)
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		cmdCounter++;
		lba = 0;
	}//for (uint32_t cmdIndex = 0;

	/*if (cmdCounter == mNumCmd)
	{
	sc_stop();
	}*/
	if (cmdCounter == mNumCmd)
	{
		if (mLogIOPS == false)
		{
			sc_time lastCmdTime = sc_time_stamp();
			sc_time delayTime = lastCmdTime - mFirstCmdTime;
			mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
			mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
			mLogIOPS = true;
		}
		//wait(500, SC_NS);
		sc_stop();

	}
}

void TestBenchTop::multiThread()
{
	sc_time firstCmdTime = sc_time_stamp();
	mTrafficGen.openReadModeFile();
	for (uint8_t numCores = 0; numCores < mNumCore; numCores++)
	{
		sc_spawn(sc_bind(&TestBenchTop::mcore_thread, \
			this, numCores, numCmdPerThread[numCores], residualCmd[numCores]), sc_gen_unique_name("mcore_thread"));
	}
	mTrafficGen.closeReadModeFile();
	/*while (1)
	{
	if (cmdCounter == mNumCmd)
	{

	if (mLogIOPS == false)
	{
	sc_time lastCmdTime = sc_time_stamp();
	sc_time delayTime = lastCmdTime - firstCmdTime;
	mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
	mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
	mLogIOPS = true;
	}
	wait(500, SC_NS);
	break;
	}
	else
	{
	wait(500, SC_NS);
	}
	}
	sc_stop();*/
}
/** Generate command payload
* @param cmdType command Type
* @param lba starting LBA
* @param slot slot index
* @param cwCnt CW count
* @param blockSize blocksize
* @param effectiveLba effective LBA
* @param cmdPayload command payload
* @return void
**/
void TestBenchTop::genCommandPayload(bool cmdType, const uint64_t& lba, \
	const uint16_t& slotNum, const uint32_t& cwCount, const uint32_t& blockSize, uint64_t& effectiveLba, uint64_t& cmdPayload)
{


	std::ostringstream msg;
	uint8_t chanBits = (uint8_t)(lba & getChanMask());
	uint8_t bankBits = (uint8_t)((lba >> mChanMaskBits) & getBankMask());
	bool chipSelectBits = (bool)((lba >> (mBankMaskBits + mChanMaskBits)) & 0x1);
	uint32_t pageBits = (uint32_t)((lba >> (mBankMaskBits + mChanMaskBits + 0x1)) & getPageMask());

	effectiveLba = (uint64_t)chanBits | ((uint64_t)bankBits << mChanMaskBits) | ((uint64_t)chipSelectBits << (mBankMaskBits + mChanMaskBits))
		| ((uint64_t)pageBits << (mBankMaskBits + mChanMaskBits + 0x1));

	uint64_t pldMask = getPayloadMask();

	msg.str("");
	msg << "Test Bench: Generated Command: "
		//<< "cmd or data payload: " <<hex<< (*transObj.get_data_ptr())
		<< " Channel: " << hex << (uint32_t)chanBits
		<< " Banks: " << hex << (uint32_t)bankBits
		<< " chipSelect: " << hex << (uint32_t)chipSelectBits
		<< " page: " << hex << pageBits;

	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	cmdPayload = (((uint64_t)slotNum << (mLBAMaskBits + 11)) | ((uint64_t)(cmdType) << (mLBAMaskBits + 9)) | \
		((uint64_t)effectiveLba << 9) | ((uint64_t)cwCount)) & pldMask;

}

/** Send command through DDR interface to controller
* @param ddrAddress DDR address
* @param cmdDataPayload command payload
* @param size size of command and data payload
* @return void
**/
void TestBenchTop::sendCommand(const uint32_t& ddrAddress, uint8_t* cmdDataPayload,
	const uint32_t& size)
{
	std::ostringstream msg;
	// create gp and send
	tlm::tlm_generic_payload transObj;

	sc_core::sc_time delay = sc_time(mCmdTransferTime, SC_NS);
	tlm::tlm_command cmd = tlm::TLM_WRITE_COMMAND;
	mDDRBusUtilization = mCmdTransferTime;

	transObj.set_command(cmd);
	transObj.set_address(ddrAddress);
	transObj.set_data_ptr(cmdDataPayload);
	transObj.set_data_length(size);

	//delay = m_qk.get_local_time(); // Annotate b_transport with local time
	msg.str("");
	msg << "Test Bench: b_transport called: "
		//<< "cmd or data payload: " <<hex<< (*transObj.get_data_ptr())
		<< " Address: " << hex << transObj.get_address()
		<< ":@Time: " << dec << delay << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	ddrInterface->b_transport(transObj, delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, " ");
	mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, ",");
	sc_time currTime = sc_time_stamp();
	wait(delay);
	currTime = sc_time_stamp();

	//currTime = sc_time_stamp();
	//m_qk.set(delay); // Update qk with time consumed by target
	//m_qk.inc(sc_time(100, SC_NS)); // Further time consumed by initiator
	//if (m_qk.need_sync()) m_qk.sync(); // Check local time against quantum
}

/** readCompletionCommand
* reads completion queue of the controller
* @param ddrAddress DDR address
* @param cmdDataPayload command payload
* @param size size of completion queue
* @return void
**/
void TestBenchTop::readCompletionCommand(const uint32_t& ddrAddress, uint8_t* cmdDataPayload,
	const uint32_t& size)
{
	std::ostringstream msg;
	// create gp and send
	tlm::tlm_generic_payload transObj;

	//sc_core::sc_time delay = m_qk.get_local_time() + duration;

	sc_core::sc_time delay = sc_time(mCompletionQueueTxTime, SC_NS);
	tlm::tlm_command cmd = tlm::TLM_READ_COMMAND;

	transObj.set_command(cmd);
	transObj.set_address(ddrAddress);
	transObj.set_data_ptr(cmdDataPayload);
	transObj.set_data_length(size);
	//transObj.set_streaming_width(1);           
	//delay = m_qk.get_local_time(); // Annotate b_transport with local time
	msg.str("");
	msg << "Test Bench: b_transport called: "
		<< " Address: " << hex << transObj.get_address()
		<< ":@Time: " << dec << delay << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	ddrInterface->b_transport(transObj, delay);
	mDDRBusUtilization = mCompletionQueueTxTime;

	wait(delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, " ");
	mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, ",");

	//m_qk.set(delay); // Update qk with time consumed by target
	//m_qk.inc(sc_time(100, SC_NS)); // Further time consumed by initiator
	//if (m_qk.need_sync()) m_qk.sync(); // Check local time against quantum
}

void TestBenchTop::readCompletionQueue(const uint16_t& numSlot, bool& cmdStatus,
	const sc_time& time)
{
	std::ostringstream msg;
	// create gp and send
	tlm::tlm_generic_payload transObj;
	uint32_t ddrAddress = getDDRAddress(numSlot, COMPLETIONQUEUE, BL4);
	//sc_core::sc_time delay = m_qk.get_local_time() + duration;
	wait(time);
	sc_core::sc_time delay = sc_time(mCompletionQueueTxTime, SC_NS);
	tlm::tlm_command cmd = tlm::TLM_READ_COMMAND;
	uint8_t*  dataPayload = new uint8_t;
	memset(dataPayload, 0x00, sizeof(uint8_t));
	transObj.set_command(cmd);
	transObj.set_address(ddrAddress);
	transObj.set_data_ptr(dataPayload);
	transObj.set_data_length(1);
	//transObj.set_streaming_width(1);           
	//delay = m_qk.get_local_time(); // Annotate b_transport with local time
	msg.str("");
	msg << "Test Bench: b_transport called: "
		<< " Address: " << hex << transObj.get_address()
		<< ":@Time: " << dec << delay << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	if (mMode)
		bus_arbiter.lock();
	ddrInterface->b_transport(transObj, delay);
	mDDRBusUtilization = mCompletionQueueTxTime;

	wait(delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, " ");
	mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, ",");
	if (mMode)
		bus_arbiter.unlock();
	uint8_t* rxData = transObj.get_data_ptr();
	cmdStatus = (bool)(*rxData & 0xf);
	//m_qk.set(delay); // Update qk with time consumed by target
	//m_qk.inc(sc_time(100, SC_NS)); // Further time consumed by initiator
	//if (m_qk.need_sync()) m_qk.sync(); // Check local time against quantum
}

/** Send data through DDR interface to controller
* @param ddrAddress DDR address
* @param data data payload
* @param size size of data payload
* @return void
**/
void TestBenchTop::sendData(const uint32_t& ddrAddress, uint8_t* data,
	const uint32_t& size)
{
	std::ostringstream msg;
	tlm::tlm_command cmd = tlm::TLM_WRITE_COMMAND;
	// create gp and send
	tlm::tlm_generic_payload transObj;
	//sc_core::sc_time delay = m_qk.get_local_time() + duration;

	mDataTransferTime = (double)(size * 1000) / (double)(16 * mDdrSpeed);
	sc_core::sc_time delay = sc_time(mDataTransferTime, SC_NS);
	mDDRBusUtilization = mDataTransferTime;

	transObj.set_command(cmd);
	transObj.set_address(ddrAddress);
	transObj.set_data_ptr(data);
	transObj.set_data_length(size);
	//transObj.set_streaming_width(1);           
	//delay = m_qk.get_local_time(); // Annotate b_transport with local time
	msg.str("");
	msg << "Test Bench: b_transport called: "
		<< " Address: " << hex << transObj.get_address()
		<< ":@Time: " << dec << delay << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	ddrInterface->b_transport(transObj, delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, " ");
	mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, ",");
	wait(delay);

	//m_qk.set(delay); // Update qk with time consumed by target
	//m_qk.inc(sc_time(100, SC_NS)); // Further time consumed by initiator
	//if (m_qk.need_sync()) m_qk.sync(); // Check local time against quantum
}

/** loadData
* reading data through DDR interface from the controller
* @param ddrAddress DDR address
* @param data data payload
* @param size size of data payload
* @return void
**/
void TestBenchTop::loadData(const uint32_t& ddrAddress, uint8_t* data,
	const uint32_t& size)
{
	std::ostringstream msg;
	tlm::tlm_command cmd = tlm::TLM_READ_COMMAND;
	// create gp and send
	tlm::tlm_generic_payload transObj;
	//sc_core::sc_time delay = m_qk.get_local_time() + duration;

	mDataTransferTime = (double)(size * 1000) / (double)(16 * mDdrSpeed);
	sc_core::sc_time delay = sc_time(mDataTransferTime, SC_NS);
	mDDRBusUtilization = mDataTransferTime;

	transObj.set_command(cmd);
	transObj.set_address(ddrAddress);
	transObj.set_data_ptr(data);
	transObj.set_data_length(size);
	//transObj.set_streaming_width(1);           
	//delay = m_qk.get_local_time(); // Annotate b_transport with local time
	msg.str("");
	msg << "Test Bench: b_transport called: "
		<< " Address: " << hex << transObj.get_address()
		<< ":@Time: " << dec << delay << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	ddrInterface->b_transport(transObj, delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, " ");
	mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization, ",");
	wait(delay);
	//mBusUtilReport->writeFile(sc_time_stamp().to_double(), 0, " ");
	//mBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), 0, ",");

	//m_qk.set(delay); // Update qk with time consumed by target
	//m_qk.inc(sc_time(100, SC_NS)); // Further time consumed by initiator
	//if (m_qk.need_sync()) m_qk.sync(); // Check local time against quantum
}

/** Read and check completion command queue
* by comparing it's field with slot table  entry
* @return void
**/
void TestBenchTop::getCompletionQueue(uint8_t& queueCount, uint8_t& validCount)
{
	std::ostringstream msg;
	uint32_t ddrAddress;
	ddrAddress = getDDRAddress(0x0, COMPLETIONQUEUE, BL8);

	readCompletionCommand(ddrAddress, mReadCompletionQueue, MAX_COMPLETION_BYTES_READ);
	memcpy(&queueCount, (mReadCompletionQueue + 0), 1);
	memset(mCompletionCmdEntry, 0x00, MAX_VALID_COMPLETION_ENTRIES);

	//read completion entries and store it in the array
	//for (uint8_t cmdIndex = 0; cmdIndex < MAX_VALID_COMPLETION_ENTRIES; cmdIndex++)
	//{
	//	memcpy((uint8_t*)(mCompletionCmdEntry + cmdIndex), (uint8_t*)(mReadCompletionQueue + cmdIndex * COMPLETION_WORD_SIZE + 1), COMPLETION_WORD_SIZE);
	//	//mCompletionCmdEntry[cmdIndex] = *((uint16_t*)mReadCompletionQueue + cmdIndex + 1);
	//}
	validCount = 0;
	validCount = (queueCount >= MAX_VALID_COMPLETION_ENTRIES) ? (MAX_VALID_COMPLETION_ENTRIES) : (queueCount);
}


uint8_t TestBenchTop::getCwCntFromCmplQueue(uint8_t& cmdIndex)
{
	return (*(mReadCompletionQueue + cmdIndex * COMPLETION_WORD_SIZE + 1) & 0x7f);
}

/** decodeCompletionQueueData
* command responses checked
* @param queuCount  num of response
* processed by the controller
* @param validCount valid response
* in completion queue
* @return void
**/
void TestBenchTop::readCompletionQueueData(uint8_t& cmdIndex, uint16_t& slotIndex)
{
	//uint16_t slotIndex;
	bool	rxCmdType;
	uint8_t rxCwCount;
	uint64_t lba;
	//cmdCompleteCount = 0;
	unpack((mReadCompletionQueue + (cmdIndex *COMPLETION_WORD_SIZE + 1)), slotIndex, rxCwCount, rxCmdType);
	/*slotIndex = getSlotFromCmplQueue(cmdIndex);
	rxCwCount = getCwCntFromCmplQueue(cmdIndex);
	rxCmdType = getCmdTypeFromCmplQueue(cmdIndex);*/
	mSlotTableEntry = mSlotManagerObj.getCmd(slotIndex);
	lba = (mSlotTableEntry >> 9) & getLBAMask();

	std::ostringstream msg;
	msg.str("");
	msg << "getCompletionQueue: "
		<< " slot index received: " << hex << (uint32_t)slotIndex
		<< " lba expected: " << hex << (uint64_t)lba
		<< " Queue Count: " << dec << (uint32_t)mValidCnt
		<< " slot index : " << dec << (uint32_t)slotIndex
		<< " CwCnt received: " << dec << (uint32_t)rxCwCount
		<< " CwCnt expected: " << dec << (uint32_t)(mSlotTableEntry & 0x1ff)
		<< " Cmd Type received: " << hex << (uint32_t)rxCmdType
		<< " Cmd Type expected: " << hex << (uint32_t)((mSlotTableEntry >> 46) & 0x3)
		<< " @Time: " << sc_time_stamp().to_double() << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	//assert(rxCmdType == ((uint8_t)((mSlotTableEntry >> (mLBAMaskBits + 9)) & 0x3)));
	//assert(rxCwCount == (mSlotTableEntry & 0x1FF));

	if (rxCmdType == READ_CMD)
	{
		checkData(slotIndex, rxCwCount, lba);
	}


}

/** Generate random read write command size
* @return uint32_t
**/
uint32_t TestBenchTop::generateRwCmdSize(){
	uint32_t rwCommandSize;

	rwCommandSize = mBlockSize * (1 << (rand() % ((uint32_t)log2(MAX_BLOCK_SIZE / mBlockSize))));
	return rwCommandSize;
}

/** Generate No. of slots required
* corresponding to RW command size
* @param rwCommandSize Random R/W command size
* @return uint8_t
**/
uint8_t TestBenchTop::generateNoSlotReq(const uint32_t& rwCmdSize){
	uint8_t numSlot;
	numSlot = (mBlockSize <= rwCmdSize) ? (rwCmdSize / mBlockSize) : 1;
	return numSlot;
}

/** Generate DDR address from
* slot no, chip elect, burst length
* @param slotNo Starting slot index
* @param cs Chip/Rank select
* @param CASBL = CAS[12] - on the fly change BL
* @return uint32_t
**/
uint32_t TestBenchTop::getDDRAddress(const uint16_t& slotNo, const uint8_t& cs, bool CASBL){
	uint32_t ddrAddress = 0;
	ddrAddress = (uint32_t)slotNo | ((uint32_t)CASBL << 12) | ((uint32_t)cs << 15);
	return ddrAddress;
}

/** Check data if command in completion queue
* is Read command
* @param slotIndex slot table index
* @param rcvdCwCnt Cnt will determine the xfer length for read
* @param lba Staring LBA address
* @return void
**/
void TestBenchTop::checkData(const uint16_t& slotIndex, const uint16_t& rxCwCount, const uint64_t& lba){
	std::ostringstream msg;
	uint8_t* expData;
	uint8_t* rxData;
	uint32_t ddrAddress;

	msg.str("");
	msg << "check data triggered" << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	expData = new uint8_t[rxCwCount * mCwSize];
	rxData = new uint8_t[rxCwCount* mCwSize];
	ddrAddress = getDDRAddress(slotIndex, DATABUFFER0, BL8);

	//mDataGenObj.getData(expData, lba, (rxCwCount * mCwSize));

	loadData(ddrAddress, rxData, (rxCwCount * mCwSize));
		
	delete[] expData;
	delete[] rxData;
}


string TestBenchTop::appendParameters(string name, string format){

	char temp[64];
	name.append("_iosize");
	if (mEnableWorkLoad == true)
	{
		_itoa(mWrkldIoSize, temp, 10);
	}
	else
	{
		_itoa(mIoSize, temp, 10);
	}
	name.append("_");
	name.append(temp);
	name.append("_blksize");

	_itoa(mBlockSize, temp, 10);
	name.append("_");
	name.append(temp);
	name.append("_qd");
	_itoa(mQueueDepth, temp, 10);
	name.append("_");
	name.append(temp);
	name.append(format);
	return name;
}


void TestBenchTop::freeSlot(uint16_t& slotIndex, uint8_t slotCount)
{
	uint16_t tags = mSlotManagerObj.getTag(slotIndex);
	uint8_t numSlot = (uint8_t)mSlotManagerObj.getListSize(tags);

	if (mSlotManagerObj.removeSlotFromList(tags, slotIndex))
	{
		if (mSlotManagerObj.getListSize(tags) == 0)
		{
			mLatency.at(tags).endDelay = sc_time_stamp().to_double();
			mLatency.at(tags).latency = mLatency.at(tags).endDelay - mLatency.at(tags).startDelay;
			mSlotManagerObj.resetTags(tags);
			uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
			mLatencyRep->writeFile(slotNum, mLatency.at(tags).latency / 1000, " ");
			mLatencyRepCsv->writeFile(slotNum, mLatency.at(tags).latency / 1000, ",");
			//cmdCompleteCount++;
			if (numSlot == slotCount)
			{
				mIsCmdCompleted = true;
			}
			else
				mIsCmdCompleted = false;
		}
		mSlotManagerObj.freeSlot(slotIndex);
	}//if
}

void TestBenchTop::freeSlot(uint16_t& slotIndex)
{
	uint16_t tags;
	//uint8_t numSlot = mSlotManagerObj.getListSize(tags);

	if (mSlotManagerObj.removeSlotFromList(tags, slotIndex))
	{
		if (mSlotManagerObj.getListSize(tags) == 0)
		{
			uint32_t qIndex = 0;

			mSlotManagerObj.resetTags(tags);
			uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
			std::vector<CmdQueueField>::iterator index;

			/*Find the entry in the submission queue corresponding to the command being completed and
			then set the isCmdDone flag to true to signal that the corresponding entry in the SQ is available to be
			populated*/
			if (findQueueEntry(index, slotNum))
			{
				CmdQueueField queueEntry = *index;
				double latency = sc_time_stamp().to_double() - queueEntry.startDelay;
				mFreeSpace++;

				mLatencyRep->writeFile(slotNum, latency / 1000, " ");
				mLatencyRepCsv->writeFile(slotNum, latency / 1000, ",");

				mTrigSubCmdEvent.notify(SC_ZERO_TIME);
				mSlotIndexQueue.push(slotNum);

				//wait(mTrigCmdDispatchEvent);
				
				mIsCmdCompleted = true;
			}


		}
		mSlotManagerObj.freeSlot(slotIndex);
	}//if
}


void TestBenchTop::freeSlotWithWorkload(uint16_t& slotIndex)
{
	uint16_t tags;
	//uint8_t numSlot = mSlotManagerObj.getListSize(tags);

	if (mSlotManagerObj.removeSlotFromList(tags, slotIndex))
	{
		if (mSlotManagerObj.getListSize(tags) == 0)
		{
			uint32_t qIndex = 0;
			
			mSlotManagerObj.resetTags(tags);
			uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
			std::vector<CmdQueueField>::iterator index;

			/*Find the entry in the submission queue corresponding to the command being completed and 
			then set the isCmdDone flag to true to signal that the corresponding entry in the SQ is available to be
			populated*/
			if (findQueueEntry(index, slotNum))
			{
				CmdQueueField queueEntry = *index;
				double latency = sc_time_stamp().to_double() - queueEntry.startDelay;
				mFreeSpace++;

				mLatencyRep->writeFile(slotNum, latency / 1000, " ");
				mLatencyRepCsv->writeFile(slotNum, latency / 1000, ",");
				
				mTrigSubCmdEvent.notify(SC_ZERO_TIME);
				mSlotIndexQueue.push(slotNum);

				if (!mWrkLoad.isEOF())
				{
					wait(mTrigCmdDispatchEvent);
				}
				mIsCmdCompleted = true;
			}
			
		
		}
		mSlotManagerObj.freeSlot(slotIndex);
	}//if
}

/*Empty Slot when run out of slots during dispatching ofcommands
 from the SQ*/
void TestBenchTop::emptySlot(uint16_t& slotIndex)
{
	uint16_t tags;
	//uint8_t numSlot = mSlotManagerObj.getListSize(tags);

	if (mSlotManagerObj.removeSlotFromList(tags, slotIndex))
	{
		if (mSlotManagerObj.getListSize(tags) == 0)
		{
			uint32_t qIndex = 0;

			/*mLatency.at(tags).endDelay = sc_time_stamp().to_double();
			mLatency.at(tags).latency = mLatency.at(tags).endDelay - mLatency.at(tags).startDelay;*/
			mSlotManagerObj.resetTags(tags);
			uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
			std::vector<CmdQueueField>::iterator index;
			findQueueEntry(index, slotNum);
			CmdQueueField queueEntry;
			queueEntry = *index;// mSubQueue.front();
			
			//CmdQueueField queueEntry = *index;
			//double startDelay;
			//findStartLatency(startDelay, slotNum);
			double latency = sc_time_stamp().to_double() - queueEntry.startDelay;
			//mSubQueue.pop();

			mFreeSpace++;
			//free Submission queue
			//mSubQueue.erase(index);
			mLatencyRep->writeFile(slotNum, latency / 1000, " ");
			mLatencyRepCsv->writeFile(slotNum, latency / 1000, ",");
			
			mSlotIndexQueue.push(slotNum);
		
			mTrigSubCmdEvent.notify(SC_ZERO_TIME);
			//cmdCompleteCount++;
			mIsCmdCompleted = true;


		}
		mSlotManagerObj.freeSlot(slotIndex);
	}
}

void TestBenchTop::findStartLatency(double& startLatency, uint16_t slotNum)
{

	for (std::vector<testCmdLatencyData>::iterator it = mQueueLatency.begin(); it != mQueueLatency.end(); it++)
	{
		if((it->slotNum == slotNum) && (it->isDone == true))
		{
			startLatency = it->startDelay;
			//mQueueLatency.erase(it);
			break;
		}
	}
}

#if 0
void TestBenchTop::submissionQueueThread()
{
	if (mNumCmd < mQueueDepth)
	{
		mQueueDepth = mNumCmd;
	}
	cmdField payload;
	uint32_t cmdIndex = 0;

	while (1)
	{
		if (cmdIndex < mNumCmd)
		{

			for (int queueIndex = 0; queueIndex < mQueueDepth; queueIndex++)
			{
				mTrafficGen.getCommands(payload);
				CmdQueueField queueEntry;
				sc_time startTime = sc_time_stamp();
				CmdLatencyData latency;
				latency.startDelay = startTime.to_double();
				latency.endDelay = 0;
				latency.latency = 0;
				queueEntry.mLatency.push_back(latency);
				queueEntry.payload = payload;
				mSubQueue.push(queueEntry);

				cmdIndex++;
				if (mSubQueue.size() == mQueueDepth)
				{
					wait(mTrigSubCmdEvent);
				}
			}
			mTrigCmdDispatchEvent.notify(SC_ZERO_TIME);
		}
	}
}
#endif

void TestBenchTop::processCompletionQueue(bool& isSlotBusy)
{
	bool pollFlag1 = true;
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	//If slots are not available then poll
	while (1)
	{
		/*if (pollFlag1)
		{
		wait(mPollWaitTime, SC_NS);
		pollFlag1 = false;
		}*/
		if (!isSlotBusy)
		{
			getCompletionQueue(queueCount, validCount);
			mQueueCount = queueCount;
			mValidCount = validCount;
			mByteIndex = 0;
		}
		uint16_t slotIndex;
		if (mQueueCount)
		{
			
			while(mByteIndex < mValidCount)
			{
				
				/*Read Completion queue and extract slot number from the
				payload*/
				readCompletionQueueData(mByteIndex, slotIndex);
				isSlotBusy = true;
				/*Free Slot so that it can be re-used*/
				//freeSlot(slotIndex);
				emptySlot(slotIndex);

				/*Erase a specific slot from the slot queue
				*/
				popSlotFromQueue(slotIndex);
				mByteIndex++;
				if (mIsCmdCompleted)
				{
					mIsCmdCompleted = false;
					break;
				}

			}
			if (mByteIndex == mValidCount)
			{
               isSlotBusy = false;
			}
			break;
			/*if all the sub-commands of a particular ioSize is done
			then resume slot processing*/
			//if (mIsCmdCompleted)
			//{
			//	
			//	//if even a single command is complete
			//	//exit the while loop and send more cmds.
			//	mIsCmdCompleted = false;
			//	break;
			//}

		}
		else
		{
			wait(mPollWaitTime, SC_NS);
		}
	}//while (1)
}

void TestBenchTop::submissionQueueThread()
{
	if (mNumCmd < mQueueDepth)
	{
		mQueueDepth = mNumCmd;
	}
	cmdField payload;
	uint32_t cmdIndex = 0;
	mTrafficGen.openReadModeFile();
    mFreeSpace = mQueueDepth;
	mFirstCmdTime = sc_time_stamp();
	
		mTrafficGen.openWriteModeFile();
		if (mEnableSameBankTest)
		{
			/*This method generates successive commands with LBA to
			pointing to the same bank albeit different page*/
			mTrafficGen.writeCommandsToSameBanks(mNumCmd);
		}
		else
		{
			mTrafficGen.writeCommands(mNumCmd);
		}
		mTrafficGen.closeWriteModeFile();
	
	for (uint32_t queueIndex = 0; queueIndex < mQueueDepth; queueIndex++)
	{
		if (cmdIndex < mNumCmd)
		{
			mTrafficGen.getCommands(payload);
			CmdQueueField queueEntry;

			queueEntry.startDelay = sc_time_stamp().to_double();
		
			queueEntry.payload = payload;
			queueEntry.isDispatched = false;
			queueEntry.isCmdDone = false;
			mSubQueue.push_back(queueEntry);
			mCmdCount++;
			cmdIndex++;
			
		}
	}
	mTrigCmdDispatchEvent.notify(SC_ZERO_TIME);
	wait(mTrigSubCmdEvent);
	while (1)
	{

		while (!mSlotIndexQueue.empty())
		{

			uint16_t slotNum = mSlotIndexQueue.front();
			mSlotIndexQueue.pop();

			mTrafficGen.getCommands(payload);
			
			for (std::vector<CmdQueueField>::iterator it = mSubQueue.begin(); it != mSubQueue.end(); it++)
			{
				if ((it->slotNum == slotNum) && (it->isDispatched == true) && (it->isCmdDone == true))
				{
					it->payload = payload;
					it->startDelay = sc_time_stamp().to_double();
					it->isDispatched = false;
					it->isCmdDone = false;
					/*testCmdLatencyData latencyData;
					latencyData.startDelay = sc_time_stamp().to_double();
					latencyData.endDelay = 0;
					latencyData.slotNum = 0;
					latencyData.isDone = false;
					mQueueLatency.push_back(latencyData);*/
					break;
				}

			}
			cmdIndex++;
			mCmdCount++;
		}
		mFreeSpace = 0;
		mTrigCmdDispatchEvent.notify(SC_ZERO_TIME);
		wait(mTrigSubCmdEvent);

		if (cmdIndex == mNumCmd)
		{
			wait();
		}

	}
}

bool TestBenchTop::findQueueEntry(std::vector<CmdQueueField>::iterator& index, const uint16_t slotNum)
{
	std::ostringstream msg;
	std::vector<CmdQueueField>::iterator it;

	for (it = mSubQueue.begin(); it != mSubQueue.end(); it++)
	{

		if ((it->slotNum == slotNum) && (it->isCmdDone == false) && (it->isDispatched==true))
		{
			it->isCmdDone = true;
			//it->isDispatched = false;
			index = it;
			
			return true;
			
		}
	
	}
	return false;
	
}


void TestBenchTop::testCacheModelThread()
{
	uint16_t slotIndex = 0;
	std::ostringstream msg;
	uint64_t lba = 0;
	uint64_t cmdIndex = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t ddrAddress = 0;
	uint16_t numSlot;
	bool logIOPS = false;
	bool firstSlotIndexed = false;
	uint32_t numSlotsReserved = 0;
	bool fetchCompletionQueue = false;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t slotReqIndex = 0;
	uint16_t tags;
	uint8_t remSlot;
	//uint8_t numSlotCount = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);//Find number of slots needed to send command
	bool pollFlag = true;
	bool slotNotFree = false;
	cmdField payload;
	uint64_t maxNumCmd;
	std::vector<CmdQueueField>::iterator sqPtr;
	sc_core::sc_time delay = sc_time(0, SC_NS);

	//If Number of commands are less than QD
	//make QD equal to Number of commands
	if (mCmdQueueSize > mNumCmd)
	{
		mCmdQueueSize = mNumCmd;
	}
	mQueueDepth = mCmdQueueSize;

	uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize, mCwSize, remSlot);

	if (mIoSize >= mBlockSize)
	{
		lbaSkipGap = mBlockSize / mCwSize;
		maxNumCmd = mNumCmd * (mIoSize / mBlockSize);
	}
	else
	{
		lbaSkipGap = mIoSize / mCwSize;
		maxNumCmd = mNumCmd;
	}

	sendReadWriteCommands(cmdIndex);

	//This loop is run after commands equal to QD is sent
	while (!mSlotContainer.empty())
	{
		uint8_t validCount = 0;
		CmdQueueField queueEntry;

		getCompletionQueue(queueCount, validCount);

		//if commands are completed
		if (queueCount)
		{
			sc_time currTime = sc_time_stamp();
			for (uint8_t CmplQIndex = 0; CmplQIndex < validCount; CmplQIndex++)
			{
				uint16_t slotIndex = 0;

				//Read one slot
				readCompletionQueueData(CmplQIndex, slotIndex);
				freeSlot(slotIndex);

				popSlotFromQueue(slotIndex);
				if (cmdIndex < maxNumCmd)
				{
					if (mCmdCount < mNumCmd)
					{
						wait(mTrigCmdDispatchEvent);
					}
					//if slot is free and commands are still left to process					
					if (!slotNotFree)
					{
						/*Find the SQ entry which needs to be dispatched*/
						for (sqPtr = mSubQueue.begin(); sqPtr != mSubQueue.end(); sqPtr++)
						{
							if ((sqPtr->isDispatched == false) && (sqPtr->isCmdDone == false))
							{
								payload = sqPtr->payload;
								break;
							}
						}

					}
					//Send sub Commands
					while (slotReqIndex < numSlotReq)
					{
						if (mSlotManagerObj.getAvailableSlot(numSlot))
						{
							if (!firstSlotIndexed)
							{
								firstSlotIndexed = true;
								mSlotManagerObj.getFreeTags(tags);
								mSlotManagerObj.addFirstSlotNum(tags, numSlot);
								sqPtr->isDispatched = true;
							}
							mSlotContainer.push_back(numSlot);
							slotNotFree = false;
							mSlotManagerObj.addSlotToList(tags, numSlot);
							mLogFileHandler << "TestCase: "
								<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
								<< " Slot Number= " << dec << (uint32_t)numSlot
								<< endl;

							uint64_t tempLba = payload.lba + lba;
							mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");
							if (payload.d == ioDir::write)
							{
								sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
							}
							else
							{
								sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
							}

							cmdIndex++;
							lba += lbaSkipGap;
							slotReqIndex++;

						}//if (mSlotManagerObj.getAvailableSlot(numSlot))
						else
						{
							slotNotFree = true;
							break;
						}//else

					}//while (slotReqIndex < (int16_t)numSlotCount)

					if (!slotNotFree)
					{
						//if all the sub commands are sent
						slotReqIndex = 0;
						lba = 0;
						firstSlotIndexed = false;
					}
				}//if (cmdIndex < mNumCmd)
			}//for(uint8_t CompCmdIndex = 0; CompCmdIndex < validCount; CompCmdIndex++)
		}//if (queueCount)
		else
		{
			//if command is not there in the completion queue
			// then wait for some time and poll again
			wait(mPollWaitTime, SC_NS);
		}

	}//while (!mSlotQueue.empty())

	if (mSlotContainer.empty())
	{

		sc_time lastCmdTime = sc_time_stamp();
		sc_time delayTime = lastCmdTime - mFirstCmdTime;
		mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
		mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
		logIOPS = true;
		mTrafficGen.closeReadModeFile();
		sc_stop();
	}

}
#endif
