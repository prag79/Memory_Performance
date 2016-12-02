/*******************************************************************
 * File : TestBench.h
 *
 * Copyright(C) crossbar-inc. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains stureucture for Test Bench for PCIe
 * Related Specifications: 
 *
 ********************************************************************/

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "pciTbCommon.h"
#include "NvmeQueue.h"
#include "Config.h"
#include "Reporting.h"
#include "Report.h"
#include "common.h"
#include <vector>
#include "WorkLoadManager.h"
#include "trafficGenerator.h"
//#include "DataGen.h"

#ifndef _PCI_TESTBENCH_
#define _PCI_TESTBENCH_

//using namespace DataGen;
namespace CrossbarTeraSLib{
	static const char *filePciTestBench = "PciTestBench.h";
	class PciTestBench : public sc_module
	{
	public:
		
		//Testbench interface
		tlm_utils::simple_initiator_socket<PciTestBench, BUS_WIDTH, tlm::tlm_base_protocol_types>	tbSocket;

		SC_HAS_PROCESS(PciTestBench);
			PciTestBench(sc_module_name _name, uint32_t numCmds, uint32_t blockSize, uint32_t cwSize, uint32_t SizeOfSQ,\
				uint32_t SizeOfCQ, uint8_t CmdType, uint32_t PollTime, bool enMultiSim, uint32_t ioSize, uint32_t tbQueueDepth,\
				bool enableWorkLoad, std::string wrkloadFiles, uint16_t seqLBAPct, uint16_t cmdPct);
			~PciTestBench(){
			delete[] 	mDataPtr;

			mLogFileHandler.close();
			mLBAReport->closeFile();
			mIOPSReport->closeFile();
			mIOPSReportCsv->closeFile();
			mLatencyRep->closeFile();
			mLatencyRepCsv->closeFile();
			if (mEnableWorkLoad == true){
				mTotalNumOfCmdsReport->closeFile();
				mTotalNumOfCmdsReportCsv->closeFile();
			}
		}
	private:

#pragma region STATIC_THREAD
		void testThread();
		void AComQueThread();  //to process Admin Comppletion Queue
		void NComQueThread();  //To process NVME Completion Queue
		void testCaseFromWorkLoad();  // for workload
#pragma endregion
#pragma region MEMBER_FUNC
		string appendParameters(string name, string format);
		void logReg(const uint64_t &value, const uint64_t &addr);
		void logSQ(const uint64_t &value, const uint64_t &addr);
		void logCQ(const uint64_t &addr);
		bool initController(void);
		bool executeTest(uint32_t testNo);
		bool executeTestWriteRead();
		void checkData();
		void sendPayload(const tlm::tlm_command cmd, const uint64_t &address, const uint32_t &dataLen, uint8_t* data, sc_time& delay);
#pragma endregion
#pragma region MEM_OBJECTS
		CircularQueue	*mASubQue;
		CircularQueue	*mAComQue;
		CircularQueue	*mNSubQue;
		CircularQueue	*mNComQue;

		WorkLoadManager		mWrkLoad;
		trafficGenerator	mTrafficGen;
		//DataGenerator    mDataGen;
#pragma endregion
#pragma region MEM_VARIABLES
		uint64_t	mPSubQueContent[PENDING_QUEUE_DEPTH];
		uint32_t	mPSubQueHead, mPSubQueTail;
		uint32_t	mACmd, mNCmd;
		bool		mAComQuePhaseBit;
		bool		mNComQuePhaseBit;
		//uint32_t	mQueueDepth;
		uint32_t	mBlockSize;
		uint32_t	mCwSize;
		uint8_t*	mDataPtr;
		uint32_t	mSizeOfSQ;
		uint32_t	mSizeOfCQ;
		uint8_t		mCmdType;
		uint32_t	mPollTime;
		uint32_t	mTotalNumCmds;
		bool		mEnMultiSim;
		bool		mEnableWorkLoad;
		uint32_t	mIOSize;
		uint32_t	mTbQueueDepth;
		uint16_t	mSeqLBAPct;
		uint16_t	mCmdPct;

		std::vector<uint32_t>	mCidHandler;
		std::fstream			mLogFileHandler;

		std::vector<CmdLatencyData> mLatency;

		Report*  mLBAReport;
		Report*  mIOPSReport;
		Report*  mIOPSReportCsv;
		Report*  mLatencyRepCsv;
		Report*  mLatencyRep;
		Report*  mTotalNumOfCmdsReport; //only in case of workload enabled
		Report*  mTotalNumOfCmdsReportCsv; //only in case of workload enabled

		sc_time firstCmdTime, lastCmdTime;
		bool	logIOPS;
#pragma endregion
#pragma region EVENT_VARIABLES
		sc_event	mAComQueEvent;
		sc_event	mASubQueAvailEvent;
		sc_event	mNComQueEvent;
		sc_event	mNSubQueAvailEvent;
#pragma endregion
	};
}
#endif



