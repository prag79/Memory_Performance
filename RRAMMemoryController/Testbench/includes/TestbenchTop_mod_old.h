/*******************************************************************
 * File : TestTeraSTop.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of TeraS Controller Test bench
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/

 
#ifndef __TESTBENCH_TOP_H__
#define __TESTBENCH_TOP_H__
  
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
 
    TestBenchTop(sc_module_name nm,  uint32_t ioSize, uint32_t blockSize, uint32_t cwSize, \
		uint32_t numCmd, uint32_t pageSize, uint32_t bankNum, 
		uint8_t numDie, uint32_t pageNum, uint8_t chanNum, uint32_t ddrSpeed,\
		uint32_t slotNum, uint16_t seqLBAPct, uint16_t cmdPct, \
		uint32_t cmdQueueSize, bool enMultiSim, bool mode, \
		int numCore,bool enableWrkld, std::string wrkloadFiles, uint32_t pollWaitTime)
		: sc_module(nm)
		, mWrkLoad(wrkloadFiles)
		, mCmdGenObj(blockSize, cwSize, pageSize, bankNum, numDie, pageNum, \
		  chanNum)
		, mDataGenObj(cwSize,ioSize)
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
		, mFirstCmdTime(sc_time(0,SC_NS))
		, mWrkldIoSize(0)
		, mTrafficGen(65536, cwSize, 512, ioSize, cmdPct, seqLBAPct)
		, mPollWaitTime(pollWaitTime)
        {
		   srand((unsigned int)time(NULL));

		   mTrafficGen.openWriteModeFile();
		   mTrafficGen.writeCommands(mNumCmd);
		  // mTrafficGen.setFilePtrToBeg();
		   mTrafficGen.closeWriteModeFile();

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

			mCmdGenObj.generateCmdType(mNumCmd);

			for (uint32_t slotIndex = 0; slotIndex < mSlotNum; slotIndex++)
			{
				mLatency.push_back(CmdLatencyData());
			}

			SC_HAS_PROCESS(TestBenchTop);
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
			if ( (mMode == false) && (mEnableWorkLoad == true))
			{
				SC_THREAD(testCaseFromWorkLoad);
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
				mCompletionQueueTxTime = (double)(2 * COMMAND_BL * 1000) / (double)(2 * mDdrSpeed);
			}

			string ctrlLogFile = "./Logs/TeraSTestBench";
			mWrkLoad.openReadModeFile();
			std::string typeCmd;
			uint64_t lba;
			uint32_t cwCnt;
			mWrkLoad.readFile(typeCmd, lba, cwCnt);
			mWrkldIoSize = 512 * cwCnt;
			mWrkLoad.closeFile();
		    try
			{
				if (mEnMultiSim){
					string filename = "./Reports/lba_report";
					mLBAReport = new Report(appendParameters(filename,".log"));
					filename = "./Reports/DDR_bus_utilization";
					mBusUtilReport = new Report(appendParameters(filename,".log"));
					mBusUtilReportCsv = new Report(appendParameters("./Reports/DDR_bus_utilization", ".csv"));
					filename = "./Reports/IOPS_utilization";
					mIOPSReport = new Report(appendParameters(filename, ".log"));
					mIOPSReportCsv = new Report(appendParameters(filename, ".csv"));
					ctrlLogFile = appendParameters(ctrlLogFile, ".log");
					filename = "./Reports/latency_TB_Report";
					mLatencyRep = new Report(appendParameters(filename, ".log"));
					mLatencyRepCsv = new Report(appendParameters(filename, ".csv"));
				}
				else{
					mLBAReport = new Report("./Reports/lba_report.log");
					mBusUtilReport = new Report("./Reports/DDR_bus_utilization.log");
					mBusUtilReportCsv = new Report("./Reports/DDR_bus_utilization.csv");
					mIOPSReport = new Report("./Reports/IOPS_utilization.log");
					mIOPSReportCsv = new Report("./Reports/IOPS_utilization.csv");
					mLatencyRep = new Report("./Reports/latency_TB_Report.log");
					mLatencyRepCsv = new Report("./Reports/latency_TB_Report.csv");
				}
				mLogFileHandler.open(ctrlLogFile, std::fstream::trunc | std::fstream::out);
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
			mData = new uint8_t[MAX_BLOCK_SIZE];
			memset(mData, 0x0, MAX_BLOCK_SIZE);
						
			mReadCompletionQueue = new uint8_t[MAX_COMPLETION_BYTES_READ];
			memset(mReadCompletionQueue, 0x0, MAX_COMPLETION_BYTES_READ);
			mCmdPayload = new uint8_t[8];

        }
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
		delete mLatencyRep;
		delete mLatencyRepCsv;
		delete mLBAReport;
		delete mBusUtilReport;
		delete mIOPSReport;
		delete mBusUtilReportCsv;
		delete mIOPSReportCsv;
		mLogFileHandler.close();
	}
 
private:
	uint32_t mBlockSize, mCwSize, mNumCmd;
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
	std::vector<cmdType> cmdQueue;

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

	std::vector<CmdLatencyData> mLatency;
	
	SlotManager mSlotManagerObj;
	DataGenerator mDataGenObj;
	CommandGenerator mCmdGenObj;
	WorkLoadManager mWrkLoad;
	trafficGenerator mTrafficGen;

	void testCaseWriteReadThread();
	void testCaseFromWorkLoad();
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
	void getCompletionQueue(uint8_t& queueCount);
	void readCompletionCommand(const uint32_t& ddrAddress, uint8_t* cmdDataPayload, \
		const uint32_t& size);
	void readCompletionQueue(const uint16_t& numSlot, bool& cmdStatus,
		const sc_time& time);
	uint8_t generateNoSlotReq(const uint32_t& rwCmdSize);
	uint32_t generateRwCmdSize();
	void checkData(const uint16_t& slotIndex, const uint16_t& rcvdCwCnt, const uint64_t& lba);
	void decodeCompletionQueueData(const uint8_t& queueCount, uint8_t& validCount, uint32_t& cmdCompleteCount);
	void sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba);
	void sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba);
	void sendWrite(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt);
	void sendRead(uint64_t& effectiveLba, uint64_t& cmdPayload, uint16_t& numSlot, uint64_t& lba, const uint32_t& cwCnt);
	void sendReadWriteCommands(uint32_t& cmdIndex);
	string appendParameters(string name, string format);
	uint32_t getBankMask();
	uint32_t getPageMask();
	uint32_t getChanMask();
	uint64_t getLBAMask();
	uint64_t getPayloadMask();
	
	Report*  mLBAReport;
	Report*  mLBACmdReport;
	Report*  mBusUtilReport;
	Report*  mBusUtilReportCsv;
	Report*  mIOPSReport;
	Report*  mIOPSReportCsv;
	Report*  mLatencyRepCsv;
	Report*  mLatencyRep;
	std::fstream mLogFileHandler;
	sc_mutex bus_arbiter;
	//sc_process_handle handle[8];
	uint32_t cmdCounter;
	bool mLogIOPS;
	sc_time mFirstCmdTime;
};

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
	genCommandPayload(WRITE, lba, \
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
	genCommandPayload(WRITE, lba, \
		numSlot, cwCnt, blkSize, effectiveLba, cmdPayload);
	mDataGenObj.generateData(effectiveLba, mData, mCwSize * cwCnt);

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
	genCommandPayload(READ, lba, \
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
	genCommandPayload(READ, lba, \
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

	uint16_t slotIndex=0;
	uint32_t numOfQueueReads = 0;
	std::ostringstream msg;
	uint64_t lbaIndex = 0;
	uint64_t lba = 0;
	uint64_t count = 0;
	uint32_t cmdIndex = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint64_t totalLbaCount = (uint64_t)(mMemSize / mCwSize);
	uint64_t maxValidLba = (uint64_t)(totalLbaCount / (uint64_t)(lbaSkipGap * 2));
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t ddrAddress = 0;
	uint16_t numSlot;
	bool logIOPS = false;
	bool firstSlotIndexed = false;
	uint32_t numSlotsReserved = 0;
	bool slotNotAvailFlag = false;
	bool fetchCompletionQueue = false;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	

	sc_core::sc_time delay = sc_time(0, SC_NS);
	//If Number of commands are less than QD
	//make QD equal to Number of commands
	if (mCmdQueueSize > mNumCmd)
	{
		mCmdQueueSize = mNumCmd;
	}
	mQueueDepth = mCmdQueueSize;

	uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize);
	/*if (mCmdQueueSize >= numSlotReq)
	{
		mCmdQueueSize = mCmdQueueSize / numSlotReq;
	}*/
	if (mIoSize >= mBlockSize)
	{
		lbaSkipGap = mBlockSize / mCwSize;
	} else
	{
		lbaSkipGap = mIoSize / mCwSize;
	}
	
	mTrafficGen.openReadModeFile();
	sc_time firstCmdTime = sc_time_stamp();

	while (cmdIndex < mNumCmd)
	{
		sendReadWriteCommands(cmdIndex);
	
		int16_t slotReqIndex = 0;
		uint16_t firstSlot;
		uint16_t tags;
		bool pollFlag = true;
		//This loop is run after commands equal to QD is sent
		while (!mSlotQueue.empty())	
		{

    		if (cmdIndex < mNumCmd)
			{
				if (pollFlag)
				{
					wait(mPollWaitTime, SC_NS);
					pollFlag = false;
				}
				getCompletionQueue(queueCount);

				if (queueCount)	
				{
					uint32_t cmdCompleteCount;
					decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
					slotNotAvailFlag = false;
										
					while (cmdCompleteCount && (cmdIndex < mNumCmd)) 
					{
						uint8_t numSlotCount = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize);
						cmdField payload;
						mTrafficGen.getCommands(payload);

						while (slotReqIndex < (int16_t)numSlotCount)
						{
								fetchCompletionQueue = false;
								if (mSlotManagerObj.getAvailableSlot(numSlot))
								{
									if (!firstSlotIndexed)
									{
										firstSlot = numSlot;
										mSlotManagerObj.getFreeTags(tags);
										firstSlotIndexed = true;
										mSlotManagerObj.addFirstSlotNum(tags, numSlot);
										mLatency.at(tags).startDelay = sc_time_stamp().to_double();
									}

									mSlotManagerObj.addSlotToList(tags, numSlot);
									
									mLogFileHandler << "TestCase: "
										<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
										<< " Slot Number= " << dec << (uint32_t)numSlot
										<< endl;
							
									uint64_t tempLba = payload.lba + lba;
									mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba , " ");
									if (payload.d == ioDir::write)
									{
										sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
									}
									else
									{
										sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
									}
									mSlotQueue.push(numSlot);
									lba += lbaSkipGap;
									queueCount--;
									validCount--;
									slotReqIndex++;
									
								}//if (mSlotManagerObj.getAvailableSlot(numSlot))
								else
								{
									//if slots are not available
									//wait for slots to free up
									while (1)
									{
										if (pollFlag)
										{
											wait(mPollWaitTime, SC_NS);
											pollFlag = false;
										}
										getCompletionQueue(queueCount);
										if (queueCount)
										{
											decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
											if (cmdCompleteCount)
											{
												break;
											}
											else
											{
												continue;
											}
										}
										else
										{
											wait(mPollWaitTime, SC_NS);
											continue;
										}
									}//while(1)
								}//else
							
						}//while (slotReqIndex < (int16_t)numSlotCount)
						if (slotReqIndex == numSlotCount)
						{
							slotReqIndex = 0;
							lba = 0;
						}
						if (slotNotAvailFlag == false)
						{
							cmdIndex++;
							firstSlotIndexed = false;
							cmdCompleteCount--;
							if (cmdCompleteCount == 0)
							{
								mIsCmdCompleted = false;
							}
						}
						
						if (cmdIndex == mNumCmd)
						{
							break;
						}
						if (fetchCompletionQueue)
						{
							break;
						}
					}//while (validCount)
				}//if (queueCount)
				else
				{
					wait(mPollWaitTime, SC_NS);
				}

			}//if (cmdIndex < mNumCmd)

			else
			{
				if (pollFlag)
				{
					wait(mPollWaitTime, SC_NS);
					pollFlag = false;
				}
				getCompletionQueue(queueCount);
				if (queueCount)
				{
					uint32_t cmdCompleteCount;
					decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
					queueCount -= validCount;
				}
				else
				{
					wait(mPollWaitTime, SC_NS);
				}
			}
		}//while (!mSlotQueue.empty())
	}//while (cmdIndex < mNumCmd)
		
	if (logIOPS == false)
	{
		sc_time lastCmdTime = sc_time_stamp();
		sc_time delayTime = lastCmdTime - firstCmdTime;
		mIOPSReport->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), " ");
		mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(mNumCmd * 1000 / delayTime.to_double()), ",");
		logIOPS = true;
		mTrafficGen.closeReadModeFile();
	}
		
		
	if (mSlotQueue.empty())
	{
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
void TestBenchTop::sendReadWriteCommands(uint32_t& cmdIndex)
{
	
	int32_t queueIndex = 0;
	uint16_t numSlot = 0;
	cmdType cmd;
	bool firstSlotIndexed = false;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t cmdCompleteCount;
	uint64_t lba = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);

	while (queueIndex < (int32_t)mCmdQueueSize)
	{
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize);
		uint16_t firstSlot;
		uint16_t tags;
		int16_t slotReqIndex = 0;
		cmdField payload;
		mTrafficGen.getCommands(payload);
		while (slotReqIndex < (int16_t)numSlotReq)
		{
			if (mSlotManagerObj.getAvailableSlot(numSlot))
			{
				mSlotQueue.push(numSlot);
				if (!firstSlotIndexed)
				{
					firstSlot = numSlot;
					mSlotManagerObj.getFreeTags(tags);
					firstSlotIndexed = true;
					mSlotManagerObj.addFirstSlotNum(tags, numSlot);
					mLatency.at(tags).startDelay = sc_time_stamp().to_double();
				}
				mSlotManagerObj.addSlotToList(tags, numSlot);
				
				uint64_t tempLba = payload.lba + lba;
				
				mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");
				
				if (payload.d == ioDir::write)
				{
					cmd = WRITE;
				}
				else
				{
					cmd = READ;
				}
				mLogFileHandler << "TestCase: "
					<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< " Slot Number= " << dec << (uint32_t)numSlot
					<< endl;

				if (cmd == WRITE)
				{
					sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
				}
				else if (cmd == READ)
				{
					sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
				}
				lba += lbaSkipGap;
				slotReqIndex++;
			}//if (mSlotManagerObj.getAvailableSlot(numSlot))
			else
			{
				//Ping completion queue till any slot is free
				while (1)
				{
					getCompletionQueue(queueCount);
					if (queueCount)
					{
						decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
						break;
					}
					else
					{
						wait(mPollWaitTime, SC_NS);
						continue;
					}
				}//while (1)
			}//else
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		lba = 0;
		cmdIndex++;
		queueIndex++;

	}//while(queueIndex < (int32_t)mCmdQueueSize)
}

void TestBenchTop::testCaseFromWorkLoad()
{
	uint16_t slotIndex = 0;
	uint16_t numSlot = 0;
	uint32_t numOfQueueReads = 0;
	std::ostringstream msg;
	uint64_t lbaIndex = 0;
	uint64_t lba = 0;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint64_t count = 0;
	bool firstSlotIndexed = false;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint64_t totalLbaCount = (uint64_t)(mMemSize / mCwSize);
	cmdType cmd;
	uint64_t maxValidLba = (uint64_t)(totalLbaCount / (uint64_t)(lbaSkipGap * 2));
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t ddrAddress = 0;
	bool slotNotAvailFlag = false;
	bool logIOPS = false;
	std::string cmdType;
	uint32_t cwCnt;
	string text;
	uint32_t cmdCompleteCount = 0;
	uint32_t cmdIndex = 0;

	sc_core::sc_time delay = sc_time(0, SC_NS);
	if (mCmdQueueSize > mSlotNum)
	{
		mCmdQueueSize = mSlotNum;
	}
	
	mQueueDepth = mCmdQueueSize;

	mWrkLoad.openReadModeFile();
	mWrkLoad.readFile(cmdType, lba, cwCnt);
	mWrkldIoSize = 512 * cwCnt;
	mWrkLoad.closeFile();
	
	mWrkLoad.openReadModeFile();
	uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mWrkldIoSize);
	/*if (mCmdQueueSize >= numSlotReq)
	{
		mCmdQueueSize = mSlotNum / numSlotReq;
	}*/
	
	if (mWrkldIoSize >= mBlockSize)
	{
		lbaSkipGap = mBlockSize / mCwSize;
	}
	else
	{
		lbaSkipGap = mWrkldIoSize / mCwSize;
	}
	
	sc_time firstCmdTime = sc_time_stamp();
	bool pollFlag1 = true;
	bool pollFlag2 = true;

	while (!mWrkLoad.isEOF())
	{
		uint32_t queueIndex = 0;

		//loop till commands till the configured queue depth
		// is sent , exit if end of file is reached
		while (queueIndex < mCmdQueueSize && !mWrkLoad.isEOF())
		{
			mWrkLoad.readFile(cmdType, lba, cwCnt);
			cwCnt = cwCnt / numSlotReq;

			uint16_t tags;
			int16_t slotReqIndex = 0;

			while (slotReqIndex < (int16_t)numSlotReq)
			{
				if (mSlotManagerObj.getAvailableSlot(numSlot))
				{
					mSlotQueue.push(numSlot);
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
						cmd = READ;
					}
					else {
						cmd = WRITE;
					}

					mLogFileHandler << "TestCase: "
						<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
						<< " Slot Number= " << dec << (uint32_t)numSlot
						<< endl;

					if (cmd == WRITE)
					{
						sendWrite(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
					}

					if (cmd == READ)
					{
						sendRead(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
					}
					lba += lbaSkipGap;
					slotReqIndex++;
				}//if (mSlotManagerObj.getAvailableSlot(numSlot))
				else
				{
					//If slots are not available then poll
					while (1)
					{
						if (pollFlag1)
						{
							wait(mPollWaitTime, SC_NS);
							pollFlag1 = false;
						}
						//wait(500, SC_NS);
						getCompletionQueue(queueCount);
						if (queueCount)
						{
							decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
							if (cmdCompleteCount >= numSlotReq)
							{
								cmdCompleteCount = 0;
								break;
							}
							else
							{
								continue;
							}
						}
						else
						{
							wait(mPollWaitTime, SC_NS);
							continue;
						}
					}//while (1)
				}//else
			}//while(slotReqIndex < (int16_t)numSlotReq)
			cmdIndex++;
			firstSlotIndexed = false;
			queueIndex++;
		}//while(queueIndex < mCmdQueueSize)

		int16_t slotReqIndex = 0;
		uint16_t tags;
		bool pollFlag3 = true;
		//poll for empty slots and keep sending commands till
		//end of file is reached
		while (!mSlotQueue.empty())
		{
			if (!mWrkLoad.isEOF())
			{
				if (pollFlag2)
				{
					wait(mPollWaitTime, SC_NS);
					pollFlag2 = false;
				}
				getCompletionQueue(queueCount);

				if (queueCount)
				{
					decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
					if (!cmdCompleteCount)
					{
						continue;
					}
					slotNotAvailFlag = false;

					while (cmdCompleteCount && !mWrkLoad.isEOF())
					{
						mWrkLoad.readFile(cmdType, lba, cwCnt);
						mWrkldIoSize = 512 * cwCnt;
						cwCnt = cwCnt / numSlotReq;
						
						while (slotReqIndex < (int16_t)numSlotReq)
						{
							if (mSlotManagerObj.getAvailableSlot(numSlot))
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

								mLogFileHandler << "TestCase: "
									<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
									<< " Slot Number= " << dec << (uint32_t)numSlot
									<< endl;

								if (cmdType == "R")
								{
									sendRead(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
								}
								else
								{
									sendWrite(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
								}
								mSlotQueue.push(numSlot);
								slotReqIndex++;
								queueCount--;
								validCount--;

								lba += lbaSkipGap;
							}//if (mSlotManagerObj.getAvailableSlot(numSlot))
							else
							{
								//if slots are not available
								//wait for slots to free up
								while (1)
								{
									//wait(500, SC_NS);
									getCompletionQueue(queueCount);
									if (queueCount)
									{
										decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
										if (cmdCompleteCount)
										{
											//cmdCompleteCount = 0;
											break;
										}
										else
										{
											continue;
										}
									}
									else
									{ 
										wait(500, SC_NS);
										continue;
									}
								}//while(1)
							}//else
						}//while (slotReqIndex < (int16_t)numSlotCount)
							
						
						if (slotReqIndex == numSlotReq)
						{
							slotReqIndex = 0;
						}
						if (slotNotAvailFlag == false)
						{
							cmdIndex++;
							cmdCompleteCount--;
							if (cmdCompleteCount == 0)
							{
								mIsCmdCompleted = false;
							}
							firstSlotIndexed = false;
						}
					
					}//while (validCount)
				}//if (queueCount)
				else
				{
					wait(mPollWaitTime, SC_NS);
				}
			}//if (!mWrkLoad.isEOF())
			else
			{
				if (pollFlag3)
				{
					wait(mPollWaitTime, SC_NS);
					pollFlag3 = false;
				}
				getCompletionQueue(queueCount);
				if (queueCount)
				{
					decodeCompletionQueueData(queueCount, validCount, cmdCompleteCount);
					queueCount -= validCount;
				}
				else
				{
					wait(mPollWaitTime, SC_NS);
				}
			}//else
		}//while (!mSlotQueue.empty())
	}//while (!mWrkLoad.isEOF())

	if (logIOPS == false)
	{
		sc_time lastCmdTime = sc_time_stamp();
		sc_time delayTime = lastCmdTime - firstCmdTime;
		mIOPSReport->writeFile(delayTime.to_double(), (double)(cmdIndex * 1000 / delayTime.to_double()), " ");
		mIOPSReportCsv->writeFile(delayTime.to_double(), (double)(cmdIndex * 1000 / delayTime.to_double()), ",");
		logIOPS = true;
	}
	
	if (mSlotQueue.empty())
	{
		sc_stop();
	}

}

void TestBenchTop::mcore_threadFromWorkLoad(int cpuNum, int numCmdPerThread, uint32_t residualCmd)
{
	uint64_t lba = 0;
	cmdType cmd;
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

	numCmdPerThread += residualCmd;
	
	mWrkLoad.openReadModeFile();
	for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)numCmdPerThread; cmdIndex++)
	{
		//std::cout << "CPU Number " << dec << cpuNum << std::endl;
		mWrkLoad.readFile(cmdType, lba, cwCnt);
		mWrkldIoSize = 512 * cwCnt;
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mWrkldIoSize);
		if (mIoSize >= mBlockSize)
		{
			lbaSkipGap = mBlockSize / mCwSize;
		}
		else
		{
			lbaSkipGap = mIoSize / mCwSize;
		}
		uint16_t numSlot = (uint16_t)cpuNum;
		//uint16_t maxSlotNum = numSlot + (numSlot + 1) * numSlotReq;
		cwCnt = cwCnt / numSlotReq;

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
				cmd = READ;
			}
			else {
				cmd = WRITE;
			}

			mLogFileHandler << "TestCase: "
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< " Slot Number= " << dec << (uint32_t)numSlot
				<< " CPU Number" << dec << cpuNum
				<< endl;

			if (cmd == WRITE)
			{
				bus_arbiter.lock();
				sendWrite(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}

			if (cmd == READ)
			{
				bus_arbiter.lock();
				sendRead(effectiveLba, cmdPayload, numSlot, lba, cwCnt);
				bus_arbiter.unlock();
				waitTime = sc_time(mPollWaitTime, SC_NS);
			}
			lba += lbaSkipGap;
			slotReqIndex++;

			while (1)
			{
				readCompletionQueue(numSlot, cmdStatus, waitTime);
				waitTime = sc_time(mPollWaitTime, SC_NS);
				if (cmdStatus)
				{
					if (cmd == READ)
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
							cmdCounter++;
						}
						mSlotManagerObj.freeSlot(numSlot);
					}
					break;
				}
			}//while(1)
			//}//else
		}//while(slotReqIndex < (int16_t)numSlotReq)
		firstSlotIndexed = false;
		
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
	cmdType cmd;
	uint16_t numSlot = (uint16_t)cpuNum;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	//uint32_t numCmdPerThread = 0;
	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	sc_time waitTime = sc_time(0, SC_NS);
	bool cmdStatus = false;
	bool firstSlotIndexed = false;
	cmdField *payload;
	numCmdPerThread += residualCmd;
	
	payload = new cmdField[numCmdPerThread];
	
	//memset(payload, 0x0, numCmdPerThread);
	mTrafficGen.openReadModeFile();
	mTrafficGen.getCommands(payload, numCmdPerThread, cpuNum);

	uint64_t cmdCount = 0;
	/*if (mCmdType == 0){
		cmd = WRITE;
	}
	else if (mCmdType == 1){
		cmd = READ;
	}*/
	
	for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)numCmdPerThread; cmdIndex++)
	{
		//mTrafficGen.getCommands(payload[cmdIndex]);
		uint8_t numSlotReq = mSlotManagerObj.getNumSlotRequired(mBlockSize, mIoSize);
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
			/*cmdField payload;
			bus_arbiter.lock();
			mTrafficGen.setFilePtr(cmdCount);
			mTrafficGen.getCommands(payload);
			cmdCount++;
			bus_arbiter.unlock();*/
			uint64_t tempLba = payload[cmdIndex].lba + lba;
			mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), tempLba, " ");
		
		mLogFileHandler << "TestCase: "
			<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
			<< " Slot Number= " << dec << (uint32_t)numSlot
			<< " CPU Number" << dec << cpuNum
			<< endl;
		if (payload[cmdIndex].d == ioDir::read)
		{
			cmd = READ;
		}
		else
		{
			cmd = WRITE;
		}
		if (cmd == WRITE)
		{
			bus_arbiter.lock();
			sendWrite(effectiveLba, cmdPayload, numSlot, tempLba);
			bus_arbiter.unlock();
			waitTime = sc_time(mPollWaitTime, SC_NS);
		}

		if (cmd == READ)
		{
			bus_arbiter.lock();
			sendRead(effectiveLba, cmdPayload, numSlot, tempLba);
			bus_arbiter.unlock();
			waitTime = sc_time(mPollWaitTime, SC_NS);
		}
		slotReqIndex++;
		lba += lbaSkipGap;
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
				//std::cout << "Command Complete: CPU Num: " <<dec<< cpuNum<< std::endl;
				if (cmd == READ)
				{
					int cwCnt = (mBlockSize / mCwSize);
					bus_arbiter.lock();
					checkData(numSlot, cwCnt, payload[cmdIndex].lba);
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
					//	cmdCounter++;
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
	wait(delay);
	
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
void TestBenchTop::getCompletionQueue(uint8_t& queueCount){
	std::ostringstream msg;
	uint32_t ddrAddress;
	ddrAddress = getDDRAddress(0x0, COMPLETIONQUEUE, BL8);
	
	readCompletionCommand(ddrAddress, mReadCompletionQueue, MAX_COMPLETION_BYTES_READ);
	queueCount = mReadCompletionQueue[0]; 
}

/** decodeCompletionQueueData
* command responses checked
* @param queuCount  num of response 
* processed by the controller
* @param validCount valid response 
* in completion queue
* @return void
**/
void TestBenchTop::decodeCompletionQueueData(const uint8_t& queueCount, uint8_t& validCount, uint32_t& cmdCompleteCount)
{
	uint16_t slotIndex;
	uint8_t	rxCmdType;
	uint16_t rxCwCount;
	uint64_t lba;
	//cmdCompleteCount = 0;
	validCount = 0;
	validCount = (queueCount >= MAX_VALID_COMPLETION_ENTRIES) ? (MAX_VALID_COMPLETION_ENTRIES) : (queueCount);
	
	for (uint8_t byteIndex = 0; byteIndex < validCount * 3; byteIndex += 3)
	{
		slotIndex = ((mReadCompletionQueue[byteIndex + 2] >> 1) & 0x7F) | ((mReadCompletionQueue[byteIndex + 3] & 0x1) << 7);
		rxCwCount = mReadCompletionQueue[byteIndex + 1] | ((mReadCompletionQueue[byteIndex + 2] & 0x1) << 8);
		rxCmdType = ((mReadCompletionQueue[byteIndex + 3] >> 1) & 0x3);
		
		mSlotTableEntry = mSlotManagerObj.getCmd(slotIndex);
		
		lba = (mSlotTableEntry >> 9) & getLBAMask();
		std::ostringstream msg;
		msg.str("");
		msg << "getCompletionQueue: "
			<< " slot index recieved: " << hex << (uint32_t)slotIndex
			<< " lba expected: " << hex << (uint64_t)lba
			<< " Queue Count: " << dec << (uint32_t)mValidCnt
			<< " slot index : " << dec << (uint32_t)slotIndex
			<< " CwCnt recieved: " << dec << (uint32_t)rxCwCount
			<< " CwCnt expected: " << dec << (uint32_t)(mSlotTableEntry & 0x1ff)
			<< " Cmd Type recieved: " << hex << (uint32_t)rxCmdType
			<< " Cmd Type expected: " << hex << (uint32_t)((mSlotTableEntry >> 46) & 0x3)		
			<< " @Time: " << sc_time_stamp().to_double() << endl;
		REPORT_INFO(testFile, __FUNCTION__, msg.str());

		//assert(rxCmdType == ((uint8_t)((mSlotTableEntry >> (mLBAMaskBits + 9)) & 0x3)));
		
		///assert(rxCwCount == (mSlotTableEntry & 0x1FF));
		
		if (rxCmdType == READ){
			checkData(slotIndex, rxCwCount,lba);
		} 
		
		uint16_t tags;
		if (mSlotManagerObj.removeSlotFromList(tags, slotIndex))
		{
			if (mSlotQueue.size() != 0)
				mSlotQueue.pop();
			if (mSlotManagerObj.getListSize(tags) == 0)
			{
				mLatency.at(tags).endDelay = sc_time_stamp().to_double();
				mLatency.at(tags).latency = mLatency.at(tags).endDelay - mLatency.at(tags).startDelay;
				mSlotManagerObj.resetTags(tags);
				uint16_t slotNum = mSlotManagerObj.getFirstSlotNum(tags);
				mLatencyRep->writeFile(slotNum, mLatency.at(tags).latency/1000, " ");
				mLatencyRepCsv->writeFile(slotNum, mLatency.at(tags).latency/1000, ",");
				cmdCompleteCount++;
				mIsCmdCompleted = true;
			}
			mSlotManagerObj.freeSlot(slotIndex);
		}
		
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
	numSlot = (mBlockSize <= rwCmdSize) ?  (rwCmdSize / mBlockSize):1;
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
	ddrAddress = (uint32_t)slotNo | ((uint32_t)CASBL << 12) | ((uint32_t)cs << 15) ;
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
	
	mDataGenObj.getData(expData, lba, (rxCwCount * mCwSize));
	
	loadData(ddrAddress, rxData, (rxCwCount * mCwSize));

	
//	for (unsigned int i = 0; i < (rxCwCount * mCwSize); i++) {
//#if 1
//		//assert(rcvdData[i] == expData[i]);
//#else
//		std::cout << "Rx Data: " << hex << (uint32_t)rcvdData[i]
//			<< " Expected Data: " << hex << (uint32_t)expData[i] << endl;
//#endif
//	}
//
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
#endif