/*******************************************************************
 * File : TestTeraSTop.h
 *
 * Copyright(C) HCL Technologies Ltd. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of TeraS memory Testbench
 * Related Specifications: 
 * TeraS_Device_Test Plan_ver1.1.doc
 ********************************************************************/

#ifndef TESTS_TERAS_TOP_H_
#define TESTS_TERAS_TOP_H_

#include "Common.h"
#include "TeraSTestCmdGenerator.h"
#include "TeraSTestTransQueueMgr.h"
#include "CmdExtManager.h"
#include "Report.h"
#include "reporting.h"
#include <fstream>
#include <time.h>
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/peq_with_get.h"

static const char *filename = "TestTeraSTop.log";

class TestTeraSTop : public sc_module
{
public:
	sc_out<bool> chipSelect;
	tlm_utils::simple_initiator_socket<TestTeraSTop, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types> onfiBus;
	
  struct CommandEntry
	{
		bool mCommandType;
	};

 	TestTeraSTop(sc_module_name nm,uint32_t numCommands, \
		uint32_t emulationCacheSize, uint32_t pageSize, uint32_t pageNum, \
		uint32_t bankNum, uint32_t onfiSpeed)
	:sc_module(nm)
	, CmdGenObj(emulationCacheSize, pageSize, bankNum)
	, mECS(emulationCacheSize)
	, mNumCmd(numCommands)
	, mPgSize(pageSize)
	, mBankNum(bankNum)
	, mPageNum(pageNum)
	, mCmdCount(0)
	, mAddress(0)
	, mTotalReads(0)
	, mTotalWrites(0)
	, mTotalTrans(0)
	, mWriteCmdCnt(0)
	, mOnfiSpeed(onfiSpeed)
	{
		srand((unsigned int)time(NULL));
		
		if (mNumCmd <= 0)
		{
			throw "Input Parameter Error: Number of Commands entered must be more than Zero";
		}

		try {
			mRespQueue = new tlm_utils::peq_with_get<tlm::tlm_generic_payload>("mRespQueue");
			
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice: PEQ bad allocation" << ba.what() << std::endl;
		}

		mTestDataFile.open("testDataFile.dat", std::fstream::in | std::fstream::out | std::fstream::trunc |std::fstream::binary);
		mTestDataFile.seekp(0, mTestDataFile.beg);
		mTestAddrFile.open("testAddrFile.dat", std::fstream::in | std::fstream::out |std::fstream::trunc | std::fstream::binary);
		mTestAddrFile.seekp(0, mTestAddrFile.beg);
		
		try {
			mWriteData = new uint8_t[mECS];
			mRxData = new uint8_t[mECS];
			memset(mWriteData, 0x0, mECS);
			mReadData = new uint8_t[mECS];
			memset(mReadData, 0x0, mECS);
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice: mWriteData/mReadData" << ba.what() << std::endl;
		}
		
		onfiBus.register_nb_transport_bw(this, &TestTeraSTop::nb_transport_bw);

		try {
			mTransReport = new Report("testTrans_report.log");
			mReadReport = new Report("testRead_report.log");
			mWriteReport = new Report("testWrite_report.log");
			openReportFile();
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice:Bad File allocation" << ba.what() << std::endl;
		}
		SC_HAS_PROCESS(TestTeraSTop);
		SC_THREAD(TestCase);

		SC_METHOD(checkRespMethod);
		sensitive << mRespQueue->get_event();
		dont_initialize();
	}

	~TestTeraSTop() {
		
		if (mWriteData)
	    delete[] mWriteData;

		if (mReadData)
		delete[] mReadData;
	
		if (mRxData)
		delete[] mRxData;
		closeReportFile();

		if (mRespQueue)
		delete mRespQueue;
	}

private:

	TestCommandGenerator CmdGenObj;
	
	typedef struct CommandEntry command;
	command mCommand;
	uint8_t *mReadData;
	uint8_t *mWriteData;
	uint8_t * mRxData;
	std::fstream mTestDataFile;
	std::fstream mTestAddrFile;

	CmdExtManager mCmdExtMgr;
	
	std::unique_ptr<CmdExtension> mReadCmdExt;
	uint32_t mECS;
	uint32_t mNumCmd;
	uint32_t mPgSize;
	uint32_t mCmdCount;
	uint64_t mWriteCmdCnt;
	uint64_t mAddress;
	uint8_t  mBankNum;
	uint32_t mPageNum;
	uint32_t mOnfiSpeed;

	uint64_t mTotalReads;
	uint64_t mTotalWrites;
	uint64_t mTotalTrans;

	Report*  mTransReport;
	Report*  mReadReport;
	Report*  mWriteReport;

	void TestCase();
	void checkRespMethod();
	inline void checkData(uint32_t cmdNum);
	inline void genCommand(bool cmdType);
	inline void genAddress(uint32_t cmdNum);
	inline void getAddress(uint32_t cmdNum, uint64_t& addr);
	inline void genData(uint32_t cmdNum);
	inline void getData(uint32_t& cmdNum, uint8_t* dataPtr);
	inline void createPayload(tlm::tlm_generic_payload& payload, uint32_t cmdQueueNum, const uint64_t& addr);
	void openReportFile();
	void closeReportFile();

	virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans, \
		tlm::tlm_phase& phase, sc_time& delay);

	sc_core::sc_event mNextWriteReqEvent;
	sc_core::sc_event mNextReadReqEvent;
	sc_core::sc_event mTriggerReadEvent;
	

	tlm::tlm_generic_payload mWritePayload;
	tlm::tlm_generic_payload mReadPayload;
	TransQueueManager mTransQueue;
	tlm_utils::peq_with_get<tlm::tlm_generic_payload>*  mRespQueue;
};

/** Create generic payload
* @param payload generic payload
* @param cmdQueueNum command queue no.
* @return void
**/

void TestTeraSTop::createPayload(tlm::tlm_generic_payload& payload, uint32_t cmdQueueNum, const uint64_t& addr)
{
	payload.set_address(addr);
	payload.set_streaming_width(1);
	CmdExtension* writeCmdExt = nullptr;
	if (mCommand.mCommandType == WRITE) {
		try {
			mCmdExtMgr.enqueue();
			writeCmdExt = mCmdExtMgr.dequeue();
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice:WRITE:Bad extension allocation" << ba.what() << std::endl;
		}
		payload.set_command(tlm::TLM_WRITE_COMMAND);
		writeCmdExt->queueNum = cmdQueueNum;
		payload.set_extension(writeCmdExt);
		payload.set_data_ptr(mWriteData);
	}
	else {
		CmdExtension* readCmdExt = nullptr;
		try {
			mCmdExtMgr.enqueue();
			readCmdExt = mCmdExtMgr.dequeue();
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice:READ:Bad extension allocation" << ba.what() << std::endl;
		}
		payload.set_command(tlm::TLM_READ_COMMAND);
		readCmdExt->queueNum = cmdQueueNum;
		payload.set_extension(readCmdExt);
		payload.set_data_ptr(mReadData);
	}
	payload.set_data_length(mECS);
	
}

/** Generate Random data
* @param uint32_t cmdNum
* @return void
**/

void TestTeraSTop::genData(uint32_t cmdNum)
{
	if (!mTestDataFile.is_open()){
		cout << "File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	try{
		uint8_t* data = new uint8_t[mECS];

		
		for (uint32_t j = 0; j < mECS; j++) {
			data[j] = rand() % 0xff;
		}
		std::memcpy(mWriteData, data, mECS);
		uint64_t fileIndex = cmdNum * mECS;
		mTestDataFile.seekp(fileIndex, mTestDataFile.beg);
		mTestDataFile.write((char*)data, mECS);
		delete[] data;
	}
	catch (std::bad_alloc& ba)
	{
		std::cerr << "TeraSMemoryDevice:READ:Bad data allocation" << ba.what() << std::endl;
	}
}

/** Get Random data
* @param uint32_t cmdNum
* @param uint8_t* dataPtr
* @return void
**/
void TestTeraSTop::getData(uint32_t& cmdNum, uint8_t* dataPtr)
{
	if (!mTestDataFile.is_open()){
		cout << "Test Data File: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint64_t fileIndex = mECS * cmdNum;
	mTestDataFile.seekg(fileIndex, mTestDataFile.beg);
	mTestDataFile.read((char*)dataPtr, mECS);
}

/** Generate Write/Read command
* @param cmdType command type
* @return void
**/

void TestTeraSTop::genCommand(bool cmdType)
{
	mCommand.mCommandType = cmdType;
}

/** Generate Address
* @param uint32_t cmdNum command Number
* @return void
**/
void TestTeraSTop::genAddress(uint32_t cmdNum)
{
	if (!mTestAddrFile.is_open()){
		cout << "File open error" << endl;
		std::exit(EXIT_FAILURE);
	}

	uint64_t fileIndex = cmdNum * sizeof(uint64_t);
	mTestAddrFile.seekp(fileIndex, mTestAddrFile.beg);
	uint64_t addr = CmdGenObj.generateAddress(cmdNum);
	mTestAddrFile.write((char*)&addr, sizeof(uint64_t));
}

/** Get generated address
* @param cmdNum command number
* @param uint64_t& addr 
* @return void
**/

void TestTeraSTop::getAddress(uint32_t cmdNum, uint64_t& addr)
{
	if (!mTestAddrFile.is_open()){
		cout << "TestAddress File:File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint64_t fileIndex = cmdNum * sizeof(uint64_t);
	mTestAddrFile.seekg(fileIndex, mTestAddrFile.beg);
	mTestAddrFile.read((char*)&addr, sizeof(uint64_t));
}

/** TestCase Thread to send and check
* command & data
* @return void
**/

void TestTeraSTop::TestCase()
{
	tlm::tlm_generic_payload *payloadPtr;
	struct CommandEntry cmd;
	cmd.mCommandType = WRITE;
	tlm::tlm_sync_enum returnStatus = tlm::TLM_COMPLETED;
	double readCmdTransSpeed = ceil(7000 / mOnfiSpeed);   //NS
	double writeCmdTransSpeed = ceil(((7 + mECS) * 1000) / mOnfiSpeed); //NS

	std::ostringstream msg;

	chipSelect.write(0);
	sc_time tDelay = SC_ZERO_TIME;
	uint64_t validCmd = mPgSize * mPageNum;
	validCmd *= (uint32_t)mBankNum;
	if (mNumCmd > ((double)(validCmd) / mECS))
	{
		msg.str("");
		msg << "TESTBENCH:Number of Commands are more than permissible addreses" << endl;
		std::cout << "TESTBENCH:WARNING: Number of Commands are more than permissible addreses" << endl;
		REPORT_WARNING(filename, __FUNCTION__, msg.str());
		mNumCmd = (uint32_t)( validCmd / mECS);
	}
		
	//WRITE LOOP
	for (uint32_t i = 0; i < mNumCmd; i++)
	{
		//if (mTransQueue.isEmpty())
		   mTransQueue.enqueue();
	
		if (!mTransQueue.isEmpty())
		{

			payloadPtr = mTransQueue.dequeue();
			payloadPtr->acquire();
			genCommand(WRITE);
			genData(i);
			uint64_t addr = 0;
			genAddress(i);
			getAddress(i, addr);
			createPayload(*payloadPtr, i, addr);
				
			//uint32_t time = (uint32_t)rand() % 0xfff;
			//tDelay += sc_time(time, SC_NS);
			tDelay = sc_time(writeCmdTransSpeed, SC_NS);

			msg.str("");
			msg << "TESTBENCH: "
				<< " SEND COMMAND NO.: " << dec << i << " :WRITE: @Time: " << dec << sc_time_stamp().to_double();
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			tlm::tlm_phase phase = tlm::BEGIN_REQ;
			returnStatus = onfiBus->nb_transport_fw(*payloadPtr, phase, tDelay);

			if (mWritePayload.get_response_status() == tlm::TLM_ADDRESS_ERROR_RESPONSE){
				msg.str("");
				msg << "*********************Address out of bound error******************************" << endl;
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				sc_stop();
			}
			
			switch (returnStatus)
			{
			case tlm::TLM_ACCEPTED:
			{
									  wait(mNextWriteReqEvent);				  //Wait for more phases
									  break;
			}
			case tlm::TLM_COMPLETED:
			{
									   wait(tDelay);
									   break;
			}
			default: //protocol error
				break;
			}//end switch

		}//end if
	}//end for
	tDelay = SC_ZERO_TIME;
	wait(mTriggerReadEvent);

	//Read Loop
	for (uint32_t i = 0; i < mNumCmd; i++)
	{
		//if (mTransQueue.isEmpty())
		 mTransQueue.enqueue();

		if (!mTransQueue.isEmpty())
		{
			payloadPtr = mTransQueue.dequeue();
			payloadPtr->acquire();
			genCommand(READ);
			uint64_t addr = 0;
			getAddress(i, addr);
			createPayload(*payloadPtr, i, addr);

			//uint32_t time = (uint32_t)rand() % 0xfff;
			
			//tDelay += sc_time(time, SC_NS);
			tDelay = sc_time(readCmdTransSpeed, SC_NS);

			msg.str("");
			msg << "TESTBENCH: "
				<< " SEND COMMAND NO.: " << dec << i << " : READ: @Time: " << dec << sc_time_stamp().to_double();
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			tlm::tlm_phase phase = tlm::BEGIN_REQ;
			returnStatus = onfiBus->nb_transport_fw(*payloadPtr, phase, tDelay);

			if (mReadPayload.get_response_status() == tlm::TLM_ADDRESS_ERROR_RESPONSE){

				msg.str("");
				msg << "*********************Address out of bound error******************************" << endl;
				std::cout << "*********************Address out of bound error******************************" << endl;
				REPORT_FATAL(filename, __FUNCTION__, msg.str());
				exit(EXIT_FAILURE);
			}
			
			switch (returnStatus)
			{
			case tlm::TLM_ACCEPTED:
			{
				wait(mNextReadReqEvent);						  //Wait for more phases
				break;
			}
			case tlm::TLM_COMPLETED:
			{
				wait(tDelay);
				break;
			}
			default: //protocol error
				break;
			}//end switch
		}//end if
	}//end for
}

/** nb_transport_bw
* non blocking interface
* @param tlm_generic payload transaction payload
* @param phase Phase of transaction
* @param sc_time latency of transaction
* @return tlm_sync_enum
**/

tlm::tlm_sync_enum TestTeraSTop::nb_transport_bw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
{
	
	tlm::tlm_sync_enum returnStatus = tlm::TLM_COMPLETED;
	std::ostringstream msg;
	
	switch (phase)
	{
	case(tlm::BEGIN_RESP) :
	{
		  mRespQueue->notify(trans, delay);
		  returnStatus = tlm::TLM_ACCEPTED;
		  break;
	}//end case
	case (tlm::END_REQ) :
	{
          if (trans.is_write())
		  {
			  mNextWriteReqEvent.notify(SC_ZERO_TIME);
			 /* mTotalWrites++;
			  mWriteReport->writeFile("WRITE", (double_t)(sc_time_stamp().to_double()), mTotalWrites);

			  mTotalTrans++;
			  mTransReport->writeFile("TRANS", (double_t)(sc_time_stamp().to_double()), mTotalTrans);*/
		  }
          else 
          {
			  mNextReadReqEvent.notify(SC_ZERO_TIME);
          }

		  msg.str("");
		  msg << "nb_transport_bw: "
		  	<< dec << "END_REQ Received " 
			<< ":@Time: " << dec << sc_time_stamp().to_double() << endl;
		  REPORT_INFO(filename, __FUNCTION__, msg.str());

		  returnStatus = tlm::TLM_ACCEPTED;
		  break;
	}
	}//end switch
	return returnStatus;
}

/** checkRespMethod
* Process BEGIN_RESP Phase
* Frees payload ptr and checks read data
* @return void
**/
void TestTeraSTop::checkRespMethod()
{
		tlm::tlm_generic_payload *payloadPtr;
		
		tlm::tlm_phase phase = tlm::END_RESP;
		tlm::tlm_sync_enum status = tlm::TLM_COMPLETED;
		std::ostringstream msg;
		sc_time tScheduled = SC_ZERO_TIME;
		
		while ((payloadPtr = mRespQueue->get_next_transaction()) != NULL) 
		{
			
			if (payloadPtr->is_write())
			{

				msg.str("");
				msg << "checkRespMethod: "
					<< "WRITE BEG_RESP Received "
					<< "@Time: " << dec << sc_time_stamp().to_double()
					<< " Address: " <<hex<< payloadPtr->get_address();
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				CmdExtension* rxCmdExt;
				onfiBus->nb_transport_fw(*payloadPtr, phase, tScheduled);
				payloadPtr->get_extension(rxCmdExt);
				uint32_t cmdQueueNum = rxCmdExt->queueNum;
				mCmdExtMgr.release(rxCmdExt);
				payloadPtr->release();
				payloadPtr->reset();
				mTransQueue.dequeue();
				//mTransQueue.free(payloadPtr);
				mWriteCmdCnt++;
				mTotalWrites++;
				mWriteReport->writeFile("WRITE", (double_t)(sc_time_stamp().to_double()), mTotalWrites);

				mTotalTrans++;
				mTransReport->writeFile("TRANS", (double_t)(sc_time_stamp().to_double()), mTotalTrans);
				if (mWriteCmdCnt == mNumCmd)
				{
					while (!mTransQueue.isEmpty())
					{
						tlm::tlm_generic_payload* pldPtr = mTransQueue.dequeue();
						mTransQueue.release(pldPtr);
					}
					mTriggerReadEvent.notify(SC_ZERO_TIME);
				}
			}
			else if (payloadPtr->is_read()){

				msg.str("");
				msg << "checkRespMethod: "
					<< "READ BEG_RESP Received "
					<< "@Time: " << dec << sc_time_stamp().to_double()
					<< " Address: " << payloadPtr->get_address();
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				CmdExtension * rxCmdExt;
				payloadPtr->get_extension(rxCmdExt);
				uint32_t cmdQueueNum = rxCmdExt->queueNum;
				assert(payloadPtr->get_data_length() == mECS);
				uint64_t expAddress = 0;
				getAddress(cmdQueueNum, expAddress);
				uint64_t rxAddress = payloadPtr->get_address();
				assert(rxAddress == expAddress);
				
				uint8_t *data = payloadPtr->get_data_ptr();

				mNextReadReqEvent.notify(SC_ZERO_TIME);
				memcpy(mRxData, data, mECS);
				
				tlm::tlm_sync_enum returnStatus = onfiBus->nb_transport_fw(*payloadPtr, phase, tScheduled);
				switch (returnStatus)
				{
				case tlm::TLM_ACCEPTED:
				{
				   break;
				}
				case tlm::TLM_COMPLETED:
				{
				   next_trigger(tScheduled);
				   break;
				}
				default: //protocol error
					break;
				}//end switch
		
				mCmdCount++;
				mCmdExtMgr.release(rxCmdExt);
				payloadPtr->release();
				payloadPtr->reset();
				mTransQueue.dequeue();
				//mTransQueue.free(payloadPtr);
				checkData(cmdQueueNum);
				mTotalReads++;
				mReadReport->writeFile("READ", (double_t)(sc_time_stamp().to_double()), mTotalReads);
				mTotalTrans++;
				mTransReport->writeFile("TRANS", (double_t)(sc_time_stamp().to_double()), mTotalTrans);
				if (mCmdCount == mNumCmd)
				{
					while (!mTransQueue.isEmpty())
					{
						tlm::tlm_generic_payload* pldPtr =mTransQueue.dequeue();
						mTransQueue.release(pldPtr);
					}
				}
			}//end else if
		}
}

/** Check data received
* @param cmdNum command number
* @return void
**/

void TestTeraSTop::checkData(uint32_t cmdNum)
{
	
	std::ostringstream msg;
	uint8_t* expData = new uint8_t[mECS];
	getData(cmdNum, expData);

	for (uint32_t j = 0; j < mECS; j++)
	{
    	assert(mRxData[j] == expData[j]);
	}//end for
	
	delete[] expData;
	
	if (mCmdCount == mNumCmd) {
		msg.str("");
		msg << "*********************SIMULATION PASS******************************" << endl;
		REPORT_INFO(filename, __FUNCTION__, msg.str());
		mTestAddrFile.close();
		mTestDataFile.close();
		sc_stop();
	}
}

/** openReportFile()
* opens report files used for
* performance measurement
* @return void
**/
void TestTeraSTop::openReportFile()
{
	std::ostringstream msg;
	if (!mReadReport->openFile())
	{
		msg.str("");
		msg << "READ_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filename, __FUNCTION__, msg.str());
		cout << "ERROR OPENING FILE" << std::endl;
		exit(EXIT_FAILURE);
		//TODO: Report Error
	}
	if (!mWriteReport->openFile())
	{
		msg.str("");
		msg << "WRITE_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filename, __FUNCTION__, msg.str());
		cout << "ERROR OPENING FILE" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!mTransReport->openFile())
	{
		msg.str("");
		msg << "TRANSACTION_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filename, __FUNCTION__, msg.str());
		cout << "ERROR OPENING FILE" << std::endl;
		exit(EXIT_FAILURE);
	}
}

/** closeReportFile()
* closes report files used for
* performance measurement
* @return void
**/
void TestTeraSTop::closeReportFile()
{
	mReadReport->closeFile();
	mWriteReport->closeFile();
	mTransReport->closeFile();
	delete mReadReport;
	delete mWriteReport;
	delete mTransReport;
}
#endif