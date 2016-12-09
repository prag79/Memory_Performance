/*******************************************************************
 * File : TeraSMemoryChip.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of TeraS Memory model and
 * its architecture.
 * Related Specifications: 
 * TeraS_Device_Specification_ver1.1.doc
 ********************************************************************/
#ifndef MODELS_TERAS_MEMORY_DEVICE_H_
#define MODELS_TERAS_MEMORY_DEVICE_H_
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include <cmath>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"
#include <vector>
#include <queue>
#include <fstream>
#include "Report.h"
#include "dimm_common.h"
#include "FileManager.h"

namespace CrossbarTeraSLib {
	

	class TeraSMemoryDevice : public sc_module {
	public:
		std::vector<sc_in<bool>* > pChipSelect;
		std::vector<tlm_utils::simple_target_socket_tagged<TeraSMemoryDevice, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>* > pOnfiBus;

		TeraSMemoryDevice(sc_module_name name, uint32_t codeWordSize, \
			uint8_t numDie, \
			uint8_t bankNum, uint32_t pageNum, uint32_t pageSize, \
			const sc_time readTime, const sc_time programTime, uint32_t onfiSpeed,
			uint8_t chanNum, uint32_t blockSize,uint32_t queueDepth, bool enMultiSim, uint32_t ioSize, bool isPCIe)
			: sc_module(name)
			, mCodeWordSize(codeWordSize)
			, mNumDie(numDie)
			, mBankNum(bankNum)
			, mPageNum(pageNum)
			, mPageSize(pageSize)
			, mReadTime(readTime)
			, mProgramTime(programTime)
			, mCodeWordBankNum(codeWordSize / pageSize)
			, mBankMaskBits((uint32_t)log2(double(bankNum)))
			, mPageMaskBits((uint32_t)log2(double(pageSize)))
			, mTransactionCount(0)
			, mLocalTime(sc_time(0, SC_US))
			, mTotalReads(0)
			, mTotalWrites(0)
			, mAccessDelay(SC_ZERO_TIME)
			, mLastSimTime(SC_ZERO_TIME)
			, mCurrentSimTime(SC_ZERO_TIME)
			, mNumBytes(0)
			, mOnfiSpeed(onfiSpeed)
			, mChanNum(chanNum)
			, mCodeWordBank((uint32_t) (bankNum * numDie) /mCodeWordBankNum)
			, mEnMultiSim(enMultiSim)
			, mBlockSize(blockSize)
			, mQueueDepth(queueDepth)
			, mIOSize(ioSize)
			, mIsPCIe(isPCIe)
		{
			SC_HAS_PROCESS(TeraSMemoryDevice);

			if (codeWordSize < pageSize)
			{
				throw "Input Parameter Error: Code Word Size is less than Page Size";
			}

			mReadDataTransSpeed = (double)((double)(mCodeWordSize * 1000) / mOnfiSpeed); //NS
			
			initPhyDLParameters();

			initChannelSpecificParameters();
			
			string memLogFile;
			if (isPCIe)
			{
				memLogFile = "./Logs_PCIe/TeraSMemoryChip";
			}
			else
			{
				memLogFile = "./Logs/TeraSMemoryChip";
			}
			if (mEnMultiSim)
			{
				memLogFile = appendParameters(memLogFile, ".log");
				
			}
			mLogFileHandler.open(memLogFile, std::fstream::trunc | std::fstream::out);
			mCacheMemory = std::make_unique<FileManager>((uint32_t)(bankNum * pageNum * pageSize * chanNum * numDie), pageSize, bankNum, chanNum, numDie, pageNum, blockSize);
			
			createReportFiles();
			

		}
		/*Garbage Collector*/
		~TeraSMemoryDevice();
	private:
		uint32_t mBlockSize;
		uint32_t mIOSize;
		uint32_t mQueueDepth;
		uint32_t mCodeWordSize;
		uint8_t  mNumDie;
		uint8_t  mBankNum;
		uint8_t  mCodeWordBankNum;
		uint32_t mPageNum;
		uint32_t mPageSize;
		uint32_t mPageMaskBits;
		uint32_t mBankMaskBits;
		bool     mEnMultiSim;
		bool     mIsPCIe;
		sc_time  mReadTime;
		sc_time  mProgramTime;

		std::vector<std::vector<uint64_t> > mTransactionCount;
		uint64_t mTotalWrites;
		uint64_t mTotalReads;
		uint64_t mNumBytes;
		sc_time  mLocalTime;
		sc_time  mLastSimTime;
		sc_time mCurrentSimTime;
		uint32_t mTimeResolution;
		uint32_t mOnfiSpeed;
		uint8_t mChanNum;
		uint32_t mCodeWordBank;

		std::vector<uint8_t* >  mPhysicalDataCache;
		std::vector<uint8_t* >  mPhysicalDataLatch1;
		std::vector<uint8_t* >  mPhysicalDataLatch2;

		/*This data structure stores the status of the two latches
		 whether free or busy*/
		std::vector<PhysicalDLStatus* > mDataLatch1Status;
		std::vector<PhysicalDLStatus* > mDataLatch2Status;

		/*Emulation Cache data structure*/
		std::vector<uint8_t* >  mEmulationDataCache;
		//std::vector<std::vector<uint64_t> > mValidAddress;
		
		std::vector<uint64_t>::iterator addrIterator;
		std::unique_ptr<FileManager> mCacheMemory;
		std::vector<std::queue<bool> > mReadChipSelect;
		std::vector<std::queue<bool> > mReadDataChipSelect;
		std::vector<std::queue<bool> > mWriteChipSelect;

		Report*  mChannelReport;
		Report*  mChannelReportCsv;

		inline void decodeAddress(const sc_dt::uint64 memAddr, \
			uint8_t& codeWordBankNum, uint32_t& pageNum, uint8_t& code);
		inline bool checkAddress(tlm::tlm_generic_payload& memAddr);
		inline bool checkDataSize(tlm::tlm_generic_payload &payload);

		virtual tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload& trans, \
			tlm::tlm_phase& phase, sc_time& delay);

		//Implementing 4 phase AT call, will reduce it to 2 phase if speed is an issue
		void sendBackWriteACKMethod(int chanNum);
		void sendBackReadACKMethod(int chanNum);
		void memReadMethod(int chanNum);
		void memAccessWriteMethod(int chanNum);
		void memAccessReadMethod(int chanNum);

		/*Static method*/
		void setPhyDL1StatusFreeMethod(int chanNum);

		/** SC_METHOD sensitive to mDL2WriteEvent
		* Checks the status of data latch2 before 
		* writing to data latch
		* If data latch is busy 
		* waits for the data latch free event
		* @return void
		**/
		
		void phyDl2WriteMethod(uint32_t dl2Index);

		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mBegRespQueue;
		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mBegRespReadQueue;
		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mEndReqQueue;
		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mEndReadReqQueue;
		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mMemReadQueue;

		/*sc_spawn options*/
		std::vector<sc_spawn_options* > mSendBackAckProcOpt;
		std::vector<sc_spawn_options* > mSendBackReadAckProcOpt;
		std::vector<sc_spawn_options* > mMemReadProcOpt;
		std::vector<sc_spawn_options* > mMemAccessProcOpt;
		std::vector<sc_spawn_options* > mMemAccReadProcOpt;
		std::vector<sc_spawn_options* > mPhyDL2WriteProcOpt;
		std::vector<sc_spawn_options* > mPhyDL1FreeStatusProcOpt;

		inline void readMemory(uint8_t& bankNum, uint32_t& pageNum, \
			const uint8_t& numDie, const uint8_t& chanNum);
		inline void writeMemory(uint8_t& bankNum, \
			uint32_t& pageNum, const uint8_t& numDie, const uint8_t& chanNum);
		uint32_t getBankMask();

		inline void moveDataFromEmulation(uint8_t* dataPtr, const uint8_t& chanNum);
		inline void moveDataToEmulation(uint8_t* dataPtr, const uint8_t& chanNum);

		inline void copyDataToDataLatch1(uint8_t* dataPtr, \
			const uint8_t& bankIndex, const uint8_t& numDie, const uint8_t& chanNum);

		inline void copyDataToEmulationCache(const uint8_t& bankIndex, \
			const uint8_t& cwBankIndex, const uint8_t& numDie, const uint8_t& chanNum);
		inline void copyDataFromPhysicalCache(const uint8_t& bankIndex, \
			uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum);
		inline void copyDataFromEmulationCache(const uint8_t& cwBankIndex, \
			const uint8_t& bankIndex, const uint8_t& numDie, const uint8_t& chanNum);

		inline void copyDataFromPhyDataLatch2(const uint8_t& cwBankIndex, const uint8_t& bankIndex, \
			const uint8_t& numDie, const uint8_t& chanNum);

		inline void copyDataFromPhyDataLatch1(const uint8_t& bankIndex, \
			uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum);

		string appendParameters(string name, string format);

		/*Initialize parameters in the constructor*/
		void initPhyDLParameters();
		void initChannelSpecificParameters();
		void initDynamicProcess(uint8_t chanIndex);
		void createReportFiles();

		void start_of_simulation();
		void end_of_simulation();

		/*Vector of sc_event*/
		std::vector<sc_event* > mEndRespRxEvent;
		std::vector<sc_event* > mPhyDL2WriteEvent;
		//std::vector<sc_event* > mPhyDL1WriteEvent;
		std::vector<sc_event* > mPhyDL1FreeEvent;
		std::vector<sc_event* > mTrigDL1FreeStatusMethodEvent;
		/*Queue to push bankIndex value by SendWriteAccessAckMethod and pop by 
		  memAccessWriteMethod*/
		std::queue<uint8_t> mBankIndexQueue;

		double mWriteCmdTransSpeed;
		double mReadCmdTransSpeed;
		double mReadDataTransSpeed;

		sc_time mAccessDelay;
		std::fstream mLogFileHandler;
	};
}
#endif