/*******************************************************************
 * File : TestTeraSTop.h
 *
 * Copyright(C) HCL Technologies Ltd. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of TeraS Controller Testbench
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/

 
#ifndef __TESTBENCH_TOP_H__
#define __TESTBENCH_TOP_H__
  
#include "systemc.h"
#include "tlm.h"
#include "SlotManager.h"
#include "DataGenerator.h"
#include "TestCommon.h"
#include "math.h"
#include "Report.h"
#include "CommandGenerator.h"
#include "reporting.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

static const char *testFile = "TestbenchTop.h";

class TestBenchTop : public sc_core::sc_module{
 

public:
	
    tlm_utils::simple_initiator_socket<TestBenchTop, DDR_BUS_WIDTH, tlm::tlm_base_protocol_types> ddrInterface;
	tlm_utils::tlm_quantumkeeper m_qk;
 
    TestBenchTop(sc_module_name nm,  uint32_t blockSize, uint32_t cwSize, \
		uint32_t numCmd, uint32_t pageSize, uint32_t bankNum, 
		uint8_t numDie, uint32_t pageNum, uint8_t chanNum, uint32_t ddrSpeed,\
		uint32_t slotNum, bool enSeqLBA, uint8_t cmdType, uint32_t cmdQueueSize, bool enMultiSim)
		: sc_module(nm)
		, mCmdGenObj(blockSize, cwSize, pageSize, bankNum, numDie, pageNum, \
		  chanNum)
		, mDataGenObj(cwSize,blockSize)
		, mSlotManagerObj(slotNum)
		, mSlotNum(slotNum)
        , mBlockSize(blockSize)
        , mCwSize(cwSize)
        , mNumCmd(numCmd)
		, mDdrSpeed(ddrSpeed)
		, mChanMaskBits((uint32_t)log2(double(chanNum)))
		, mCwBankNum((uint16_t)((bankNum * pageSize) / cwSize))
		, mPageMaskBits((uint32_t)log2(double(pageNum)))
		, mEnSeqLBA(enSeqLBA)
		, mCmdType(cmdType)
		, mEnMultiSim(enMultiSim)
		, mCmdQueueSize(cmdQueueSize)
		, mCmdTransferTime((double)(COMMAND_BL * 1000) / (double)(2 * mDdrSpeed))
		, mCompletionQueueTxTime((double)(2 * COMMAND_BL * 1000) / (double)(2 * mDdrSpeed))
		, mDDRBusUtilization(0)
		, mQueueDepth(cmdQueueSize)
        {


		    srand(time(NULL));
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
            SC_THREAD(testCaseWriteReadThread);

		    try
			{
				if (mEnMultiSim){
					string filename = "./Reports/lba_report";
					mLBAReport = new Report(appendParameters(filename,mBlockSize,mQueueDepth));
					filename = "./Reports/DDR_bus_utilization";
					mBusUtilReport = new Report(appendParameters(filename, mBlockSize, mQueueDepth));
					filename = "./Reports/IOPS_utilization";
					mIOPSReport = new Report(appendParameters(filename, mBlockSize, mQueueDepth));
				}
				else{
					mLBAReport = new Report("./Reports/lba_report.log");
					mBusUtilReport = new Report("./Reports/DDR_bus_utilization.log");
					mIOPSReport = new Report("./Reports/IOPS_utilization.log");
				}
		
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
		delete mLBAReport;
		delete mBusUtilReport;
		delete mIOPSReport;
		
	}
 
private:
	uint32_t mBlockSize, mCwSize, mNumCmd;
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

	uint16_t mCwBankNum;
	uint32_t mNumWrites;
	uint32_t mNumReads;
	uint32_t mCmdQueueSize;
	uint32_t mSlotNum;
	bool	 mEnSeqLBA;
	uint8_t	 mCmdType;
	bool	 mEnMultiSim;
	
	std::queue<uint8_t> mSlotQueue;
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

	std::vector<CmdLatencyData> mLatency;

	SlotManager mSlotManagerObj;
	DataGenerator mDataGenObj;
	CommandGenerator mCmdGenObj;

	void testCaseWriteReadThread();
	uint32_t getDDRAddress(const uint8_t& slotNo, const uint8_t& cs, bool CASBL);
	//generate Cmd payload
	void genCommandPayload(bool cmdType, const uint64_t& lba, const uint8_t& slotNum, \
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
	uint8_t generateNoSlotReq(const uint32_t& rwCmdSize);
	uint32_t generateRwCmdSize();
	void checkData(const uint8_t& slotIndex, const uint16_t& rcvdCwCnt, const uint64_t& lba);
	void decodeCompletionQueueData(const uint8_t& queueCount, uint8_t& validCount);

	uint32_t getBankMask();
	uint32_t getPageMask();
	uint32_t getChanMask();
	uint64_t getLBAMask();
	uint64_t getPayloadMask();
	
	Report*  mLBAReport;
	Report*  mLBACmdReport;
	Report*  mBusUtilReport;
	Report*  mIOPSReport;
	
};

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

/** testCaseSingleWriteReadThread()
* sc_thread;creates cmd payload(both read and write) 
* and sends  to the controller. also generates 
* LBA(sequential or random)
* @return void
**/
//void TestBenchTop::testCaseSingleWriteReadThread(){
//
//	uint8_t slotIndex, slotNum;
//	std::ostringstream msg;
//	uint64_t lbaIndex = 0;
//	uint64_t lba = 0;
//
//	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
//	uint64_t totalLbaCount = (uint64_t)(mMemSize / mCwSize);
//	cmdType cmd;
//	uint64_t maxValidLba = (uint64_t)(totalLbaCount / (uint64_t)(lbaSkipGap * 2));
//	uint8_t queueCnt = 0;
//	slotNum = generateNoSlotReq(mBlockSize);
//
//
//	for (uint64_t cmdIndex = 0; cmdIndex < mNumCmd; cmdIndex++)
//	{
//		/*if (!mEnSeqLBA){ //Random LBA
//			lba = (rand() % ((uint64_t)pow(2.0, (mBankMaskBits + mPageMaskBits + mChanMaskBits + 1)) + 1));
//			}
//			else{ // Sequential LBA
//			*/
//		
//		//}
//		mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), lba);
//
//		if (mSlotManagerObj.availableSlot(slotNum, slotIndex))
//		{
//
//			mSlotQueue.push(slotIndex);
//			cmd = WRITE;
//
//			msg.str("");
//			msg << "CMD Type: " << cmd
//				<< ":@Time: " << sc_time_stamp().to_double()
//				<< " Transaction No.: " << dec << (uint32_t)cmdIndex
//				<< " Slot Number: " << dec << (uint32_t)slotIndex
//				<< ":@Time: " << sc_time_stamp().to_double();
//			REPORT_INFO(testFile, __FUNCTION__, msg.str());
//
//			generateCmdDataPacket(cmd, lba, \
//				slotIndex, mBlockSize / mCwSize, mBlockSize);
//			lba += lbaSkipGap;
//			uint32_t delay = rand() % 40000;
//			wait(delay, SC_NS);
//		}
//		else
//		{
//			wait(mNumCmd * 1000 * 2648, SC_US);
//			getCompletionQueue(queueCnt);
//			cmdIndex--;
//
//		}
//	}
//	lba = 0;
//	
//	for (uint64_t cmdIndex = 0; cmdIndex < mNumCmd; cmdIndex++)
//	{
//		
//		if (mSlotManagerObj.availableSlot(slotNum, slotIndex))
//		{
//			mSlotQueue.push(slotIndex);
//			cmd = READ;
//
//			msg.str("");
//			msg << "CMD Type: " << cmd
//				<< ":@Time: " << sc_time_stamp().to_double()
//				<< " Transaction No.: " << dec << (uint32_t)cmdIndex
//				<< " Slot Number: " << dec << (uint32_t)slotIndex
//				<< ":@Time: " << sc_time_stamp().to_double();
//			REPORT_INFO(testFile, __FUNCTION__, msg.str());
//
//			generateCmdDataPacket(cmd, lba, \
//				slotIndex, mBlockSize / mCwSize, mBlockSize);
//			lba += lbaSkipGap;
//			uint32_t delay = rand() % 40000;
//			wait(delay, SC_NS);
//		}
//		else
//		{
//			wait(mNumCmd * 1000 * 2648, SC_US);
//			getCompletionQueue(queueCnt);
//			cmdIndex--;
//
//		}
//	}
//		
//
//	while (!mSlotQueue.empty()){
//		wait(mNumCmd * 100000 * 2648, SC_US);
//		getCompletionQueue(queueCnt);
//	}
//	if (mSlotQueue.empty()){
//		wait(10000, SC_NS);
//		sc_stop();
//	}
//
//
//}

/** testCaseWriteReadThread()
* sc_thread;creates cmd payload(read or write) 
* and sends  to the controller .also generates 
* LBA(sequential or random)
* @return void
**/
void TestBenchTop::testCaseWriteReadThread(){

	uint8_t slotIndex=0, slotNum;
	uint32_t numSlot = 0;
	uint32_t numOfQueueReads = 0;
	std::ostringstream msg;
	uint64_t lbaIndex = 0;
	uint64_t lba = 0;
	uint64_t effectiveLba = 0;
	uint64_t cmdPayload = 0;
	uint64_t count = 0;

	uint8_t lbaSkipGap = (uint8_t)(mBlockSize / mCwSize);
	uint64_t totalLbaCount = (uint64_t)(mMemSize / mCwSize);
	cmdType cmd;
	uint64_t maxValidLba = (uint64_t)(totalLbaCount / (uint64_t)(lbaSkipGap * 2));
	uint8_t queueCount = 0;
	uint8_t validCount = 0;
	uint32_t ddrAddress = 0;
	bool logIOPS = false;

#if 0
		uint32_t mRwCommandSize = 0;
		mRwCommandSize = mBlockSize * (1 << (rand() % ((uint32_t)log2(MAX_BLOCK_SIZE / mBlockSize))));
		slotNum = generateNoSlotReq(mRwCommandSize);
#else
		slotNum = generateNoSlotReq(mBlockSize);
#endif
		
	sc_core::sc_time delay = sc_time(0, SC_NS);
	if (mCmdQueueSize > mSlotNum)
	{
		mCmdQueueSize = mSlotNum;
	}
	if (mCmdQueueSize > mNumCmd)
	{
		mCmdQueueSize = mNumCmd;
	}
	mQueueDepth = mCmdQueueSize;
	if (mCmdType == 0){
		cmd = WRITE;
	}
	else if (mCmdType == 1){
		cmd = READ;
	}
	for (uint32_t cmdIndex = 0; cmdIndex < mNumCmd;)
	{
		sc_time firstCmdTime = sc_time_stamp();
		for (uint32_t queueIndex = 0; queueIndex < mCmdQueueSize; queueIndex++)
		{

			if (numSlot < mSlotNum)
			{
				mSlotQueue.push(numSlot);
				mLBAReport->writeFile("LBA", sc_time_stamp().to_double(), lba);

			    if(mCmdType ==2)
				{
					cmd = mCmdGenObj.getCmdType(cmdIndex); //random Commands
		
				}
				mLatency.at(numSlot).startDelay = sc_time_stamp().to_double();
				if (cmd == WRITE)
				{
					genCommandPayload(cmd, lba, \
						numSlot, mBlockSize / mCwSize, mBlockSize, effectiveLba, cmdPayload);
					mDataGenObj.generateData(effectiveLba, mData);
					ddrAddress = getDDRAddress(numSlot, DATABUFFER0, BL8); // pass enum

					msg.str("");
					msg << "WRITE SEND: "
						<< " LBA: " << hex << (uint32_t)effectiveLba
						<< " Slot NUM: " << dec << (uint32_t)numSlot
						<< " Channel Number: " << hex << (effectiveLba & mChanMask)
						<< " CW Bank: " << hex << ((effectiveLba >> 4) & mBankMask)
						<< " Chip Select " << hex << ((effectiveLba >> 6) & 0x1)
						<< " Page Address " << hex << ((effectiveLba >> 7) & mPageMask);
					REPORT_INFO(testFile, __FUNCTION__, msg.str());
					
					sendData(ddrAddress, mData, mBlockSize);
					mSlotManagerObj.insertCmd(cmdPayload, numSlot);
					ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);
					 sendCommand(ddrAddress, (uint8_t*)&cmdPayload, COMMAND_PAYLOAD_SIZE);
					/*uint64_t temp = 0x4000001;
					sendCommand(ddrAddress, (uint8_t*)&temp, COMMAND_PAYLOAD_SIZE);*/
				}

				if (cmd == READ)
				{
					genCommandPayload(cmd, lba, \
						numSlot, mBlockSize / mCwSize, mBlockSize, effectiveLba, cmdPayload);
					mSlotManagerObj.insertCmd(cmdPayload, numSlot);
					ddrAddress = getDDRAddress(numSlot, PCMDQUEUE, BL4);
					
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
					
				if (!mEnSeqLBA){ //Random LBA
					lba = (rand() % ((uint64_t)pow(2.0, (mBankMaskBits + mPageMaskBits + mChanMaskBits + 1)) + 1));
				}
				else{ // Sequential LBA

					lba += lbaSkipGap;
				}
				
				numSlot++;
				cmdIndex++;
			}
			else
			{
				numSlot = 0;
						
				if (queueIndex == 0)
				{
					queueIndex = 0;
				}
				else
				{
					queueIndex--;
				}
				break;

			}
			
		}//for (int queueIndex
		
		if (mSlotQueue.size() >= mCmdQueueSize)
		{
			while (!mSlotQueue.empty())
			{
				
				sc_time currentTime = sc_time_stamp();
				getCompletionQueue(queueCount);
				
				if (queueCount)
				{
					decodeCompletionQueueData(queueCount, validCount);
					queueCount -= validCount;
				}
				else{
					wait(500, SC_NS);
				}
				/*if (queueCount == 0)
				{
					wait(500, SC_NS);
				}*/
				
			  }

			if (logIOPS == false)
			{
				sc_time lastCmdTime = sc_time_stamp();
				sc_time delayTime = lastCmdTime - firstCmdTime;
				mIOPSReport->writeFile(delayTime.to_double(), (double)(mCmdQueueSize * 1000 / delayTime.to_double()));
				logIOPS = true;
			}
		}
		if ((cmdIndex + mCmdQueueSize) >= mNumCmd)
		{
			mCmdQueueSize = (mNumCmd - cmdIndex);
		}
	
	}//for(cmdIndex
	
	if (mSlotQueue.empty())
	{
		sc_stop();
	}
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
	const uint8_t& slotNum, const uint32_t& cwCount, const uint32_t& blockSize, uint64_t& effectiveLba, uint64_t& cmdPayload)
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
	mDDRBusUtilization += mCmdTransferTime;
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization);
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
	wait(delay);
	mDDRBusUtilization += mCompletionQueueTxTime;
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization);
	
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
	mDDRBusUtilization += mDataTransferTime;
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization);

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
	mDDRBusUtilization += mDataTransferTime;
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
	wait(delay);
	mBusUtilReport->writeFile(sc_time_stamp().to_double(), mDDRBusUtilization);
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
void TestBenchTop::decodeCompletionQueueData(const uint8_t& queueCount, uint8_t& validCount)
{
	uint8_t slotIndex, rxCmdType;
	uint16_t rxCwCount;
	uint64_t lba;

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

		assert(rxCmdType == ((uint8_t)((mSlotTableEntry >> (mLBAMaskBits + 9)) & 0x3)));
		
		assert(rxCwCount == (mSlotTableEntry & 0x1FF));
		
		if (rxCmdType == READ){
			checkData(slotIndex, rxCwCount,lba);
		} 
		
		mSlotManagerObj.freeSlot(slotIndex);
		mSlotQueue.pop();
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
uint32_t TestBenchTop::getDDRAddress(const uint8_t& slotNo, const uint8_t& cs, bool CASBL){
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
void TestBenchTop::checkData(const uint8_t& slotIndex, const uint16_t& rxCwCount, const uint64_t& lba){
	std::ostringstream msg;
	uint8_t* expData;
	uint8_t* rxData;
	uint32_t ddrAddress;

	msg.str("");
	msg << "check data triggerred" << endl;
	REPORT_INFO(testFile, __FUNCTION__, msg.str());
	expData = new uint8_t[rxCwCount * mCwSize];
	rxData = new uint8_t[rxCwCount* mCwSize];
	ddrAddress = getDDRAddress(slotIndex, DATABUFFER0, BL8);
	
	mDataGenObj.getData(expData, lba, (rxCwCount * mCwSize));
	
	loadData(ddrAddress, rxData, (rxCwCount * mCwSize));

	
	for (unsigned int i = 0; i < (rxCwCount * mCwSize); i++) {
#if 1
		//assert(rcvdData[i] == expData[i]);
#else
		std::cout << "Rx Data: " << hex << (uint32_t)rcvdData[i]
			<< " Expected Data: " << hex << (uint32_t)expData[i] << endl;
#endif
	}

	delete[] expData;
	delete[] rxData;
}

#endif