/*******************************************************************
 * File : TeraSController.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Author: Pragnajit Datta Roy
 * Description: This file contains detail of TeraS Controller model and
 * its architecture.
 * Supports both single core and multi core mode
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef MODELS_TERAS_MEMORY_CONTROLLER_H_
#define MODELS_TERAS_MEMORY_CONTROLLER_H_

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"
#include "dimm_common.h"
#include "Report.h"
#include "CommandQueueManager.h"
#include "DataBufferManager.h"
#include "CompletionQueueManager.h"
#include "CompletionQueueManager_mcore.h"
#include "ActiveCommandQueue.h"
#include "PendingCmdQueueManager.h"
#include "CwBankLinkListManager.h"
#include "IntermediateCmdQueueManager.h"
#include "TransManager.h"


namespace CrossbarTeraSLib {
	static const char *filename = "TeraSMemoryController.cpp";
	//static const char *ctrlLogFile = "./Logs/TeraSMemoryController.log";
	
	typedef enum activeQueueType qType;

	class TeraSController : public sc_module
	{
	public:
		std::vector < tlm_utils::simple_initiator_socket_tagged<TeraSController, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>* > pOnfiBus;
		std::vector<sc_out<bool>* > pChipSelect;

		tlm_utils::simple_target_socket<TeraSController, DDR_BUS_WIDTH, tlm::tlm_base_protocol_types> ddrInterface;

		TeraSController(sc_module_name nm, uint32_t chanNum, uint32_t blockSize, \
			uint32_t codeWordSize, uint32_t pageSize, \
			uint32_t queueSize, const sc_time readTime, const sc_time programTime, \
			uint8_t credit, uint32_t onfiSpeed, uint32_t DDRSpeed,
		    uint32_t bankNum, uint8_t numDie, uint32_t pageNum,
			uint32_t numSlot, uint32_t queueDepth,bool enMultiSim, bool mode, double cmdTransferTime, uint32_t ioSize)
			:sc_module(nm)
			, mChanNum(chanNum)
			, mCodeWordNum((uint32_t)((bankNum * pageSize * numDie) / codeWordSize))
			, mCmdQueue(chanNum, mCodeWordNum, blockSize, codeWordSize, bankNum, queueSize, pageSize, pageNum)
			, mDataBuffer_1(blockSize, codeWordSize, queueSize)
			, mDataBuffer_2(blockSize, codeWordSize, queueSize)
			, mBankLinkList(mCodeWordNum, chanNum)
			, mCwCount(blockSize / codeWordSize)
			, mReadTime(readTime)
			, mProgramTime(programTime)
			, mCodeWordSize(codeWordSize)
			, mPageSize(pageSize)
			, mBankMaskBits((uint32_t)log2(double(bankNum)))
			, mPageMaskBits((uint32_t)log2(double(pageNum)))
			, mChanMaskBits((uint32_t)log2(double(chanNum)))
			, mPageNum(pageNum)
			, mCredit(credit)
			, mOnfiSpeed(onfiSpeed)
			, mBlockSize(blockSize)
			, mDDRSpeed(DDRSpeed)
			, mAddrShifter((uint16_t)log2(blockSize / codeWordSize))
			, mBankNum(bankNum)
			, mNumDie(numDie)
			, mCwBankNum((uint16_t)((bankNum * pageSize * numDie) / codeWordSize))
			, mTotalReads(0)
			, mTotalWrites(0)
			, mDelay(SC_ZERO_TIME)
			, mCmdTransferTime(cmdTransferTime)
			, mECCDelay(1000)
			, mNumCmdWrites(0)
			, mNumCmdReads(0)
			, mNextReadCount(0)
			, mEnMultiSim(enMultiSim)
			, mQueueDepth(queueDepth)
			, mIntermediateQueue(numSlot)
			, mCompletionQueue_mcore(numSlot)
			, mMode(mode)
			, mCmdType(cmdType::READ)
			, mIOSize(ioSize)
		{
			SC_HAS_PROCESS(TeraSController);
			if (mCwBankNum > 2)
			{
				mCwBankMaskBits = (uint32_t)(log2(double(mCwBankNum/mNumDie))) ;
			}
			else {
				mCwBankMaskBits = 1;
			}

			SC_METHOD(completionMethod);
			sensitive << mCmdCompleteEvent;
			dont_initialize();

			initMasks();
			
			std::string ctrlLogFile = "./Logs/TeraSMemoryController";
			if (enMultiSim)
			{
				ctrlLogFile = appendParameters(ctrlLogFile, ".log");
			}
			
			mLogFileHandler.open(ctrlLogFile, std::fstream::trunc | std::fstream::out);

			for (uint32_t slotIndex = 0; slotIndex < numSlot; slotIndex++)
			{
				mLatency.push_back(CmdLatencyData());
				mWaitEndTime.push_back(0);
				mWaitStartTime.push_back(0);
				mWaitTime.push_back(0);
				
			}
			if (codeWordSize < pageSize)
			{
				throw "Input Parameter Error: Code Word Size is less than Page Size";
			}
			mSlotTable.resize((uint32_t)queueSize/(blockSize/codeWordSize));
		
			ddrInterface.register_b_transport(this, &TeraSController::b_transport);
		
			initChannelSpecificParameters();

			createReportFiles();

			
		}

		~TeraSController()
		{
			for (uint32_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
			{
				delete[] mReadData.at(chanIndex);
				delete[] mWriteData.at(chanIndex);
				delete mRespQueue.at(chanIndex);
				delete pChipSelect.at(chanIndex);
				
				delete mPendingCmdProcOpt.at(chanIndex);
				delete mPendingReadCmdProcOpt.at(chanIndex);
				delete mCheckRespProcOpt.at(chanIndex);
				delete mCmdDispatcherProcOpt.at(chanIndex);
				
				delete mChanMgrProcOpt.at(chanIndex);
				delete pOnfiBus.at(chanIndex);
				delete mEndReadEvent.at(chanIndex);

				delete mTrigCmdDispEvent.at(chanIndex);
				delete mCmdDispatchEvent.at(chanIndex);
				delete mChanMgrEvent.at(chanIndex);
				delete mAlcqNotFullEvent.at(chanIndex);
				delete mAscqNotFullEvent.at(chanIndex);
				delete mAlcqNotEmptyEvent.at(chanIndex);
				delete mAscqNotEmptyEvent.at(chanIndex);
				delete mEndReqEvent.at(chanIndex);
				delete mBegRespEvent.at(chanIndex);
				delete mCreditPositiveEvent.at(chanIndex);
			}
		
			mLatencyReport->closeFile();
			delete mLatencyReport;
			mOnfiChanUtilReport->closeFile();
			delete mOnfiChanUtilReport;
			mOnfiChanActivityReport->closeFile();
			delete mOnfiChanActivityReport;
			mLogFileHandler.close();
		}
		virtual tlm::tlm_sync_enum nb_transport_bw(int id, tlm::tlm_generic_payload& trans, \
			tlm::tlm_phase& phase, sc_time& delay);

		void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
	private:
		uint32_t mChanNum;
		uint16_t mCwBankNum;
		uint8_t  mCredit;
		uint32_t mOnfiSpeed;
		uint32_t mBlockSize;
		uint32_t mIOSize;
		uint32_t mDDRSpeed;
		uint16_t mAddrShifter;
		uint32_t mCodeWordNum;
		uint32_t mPageNum;
		uint16_t mBankNum;
		uint8_t mNumDie;
		bool   mEnMultiSim;
		bool mMode;
		cmdType mCmdType;
		
		enum queueType {
			PCMDQueue,
			DataBuffer_1,
			DataBuffer_2,
			CompletionQueue
		};

		typedef enum queueType QueueType;

		struct slotTableEntry
		{
			cmdType cmd : 2;
			uint16_t cnt : 16;
			slotTableEntry()
			{
				reset();
			}
			inline void reset()
			{
				cnt = 0;
			}
		};
		typedef struct slotTableEntry slotTable;

		CommandQueueManager mCmdQueue;
		DataBufferManager mDataBuffer_1;
		DataBufferManager mDataBuffer_2;
		CompletionQueueManager mCompletionQueue;
		CompletionQueueManagerMCore mCompletionQueue_mcore;
		IntermediateCmdQueueManager mIntermediateQueue;

		std::vector<ActiveCommandQueue> mAlcq;
		std::vector<ActiveCommandQueue> mAscq;
		std::vector<uint8_t*> mReadData;
		std::vector<uint8_t*> mWriteData;

		std::vector<std::unique_ptr<PendingCmdQueueManager> > mPendingCmdQueue;
		std::vector<std::unique_ptr<PendingCmdQueueManager> > mPendingReadCmdQueue;
		
		std::vector<slotTable> mSlotTable;

		CwBankLinkListManager mBankLinkList;
		std::vector<sc_event* > mCmdDispatchEvent;
		std::vector<sc_event* > mTrigCmdDispEvent;
		std::vector<sc_event* > mChanMgrEvent;
		std::vector<sc_event* > mAlcqNotFullEvent;
		std::vector<sc_event* > mAscqNotFullEvent;
		std::vector<sc_event* > mAlcqNotEmptyEvent;
		std::vector<sc_event_queue* > mAlcqEmptyEvent;

		std::vector<sc_event* > mAscqNotEmptyEvent;
		std::vector<sc_event_queue* > mAscqEmptyEvent;

		
		std::vector<sc_event* > mEndReqEvent;
		std::vector<sc_event* > mEndReadEvent;
		std::vector<sc_event* > mBegRespEvent;
		std::vector<sc_event* > mCreditPositiveEvent;

		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mRespQueue;
		uint16_t mCwCount;
		uint32_t mQueueDepth;
		uint32_t mCodeWordSize;
		uint32_t mPageSize;
		uint32_t mBankMaskBits;
		uint32_t mPageMaskBits;
		uint32_t mChanMaskBits;
		uint32_t mCwBankMaskBits;
		uint32_t mLBAMaskBits;
		uint32_t mActiveLBAMaskBits;
		uint32_t mPendingLBAMaskBits;

		uint64_t mTotalReads;
		uint64_t mTotalWrites;
		uint64_t mNumCmdWrites;
		uint64_t mNumCmdReads;
		uint32_t mNextReadCount;

		/*std::vector<uint16_t> mLongQueueSize;
		std::vector<uint16_t> mShortQueueSize;*/

		std::vector<uint8_t> mAvailableCredit;
		std::vector<CmdLatencyData> mLatency;
		std::vector<double_t> mOnfiChanTime;
		std::vector<double_t> mOnfiTotalTime;
		std::vector<double_t> mWaitStartTime;
		std::vector<double_t> mWaitEndTime;
		std::vector<double_t> mWaitTime;
		std::vector<double_t> mOnfiStartActivity;
		std::vector<double_t> mOnfiEndActivity;
		std::vector<bool> mOnfiChanActivityStart;
	
		sc_time mReadTime;
		sc_time mProgramTime;
		sc_time mDelay;

		double mCmdTransferTime;
		double mDataTransferTime;
		double mECCDelay;

		/* CW and Data Latch1, data latch2 status */
		std::vector<std::vector<cwBankStatus> > mCwBankStatus;
		std::vector<std::vector<cwBankStatus> > mPhyDL1Status;
		std::vector<std::vector<cwBankStatus> > mPhyDL2Status;
		TransManager mTransMgr;
	

		/*std::unique_ptr<Report>  mLongQueueReport;
		Report*  mShortQueueReport;*/
		Report*  mLatencyReport;
		Report*  mOnfiChanUtilReport;
		Report*  mOnfiChanActivityReport;

		/*Report*  mLongQueueReportCsv;
		Report*  mShortQueueReportCsv;*/
		Report*  mLatencyReportCsv;
		Report*  mOnfiChanUtilReportCsv;
		Report*  mOnfiChanActivityReportCsv;

		std::string appendParameters(std::string name, std::string format);

		char* renameLog(char *reportName);
		void decodeAddress(const uint64_t& address, QueueType& queueType, uint16_t& slotNum);
		void TeraSBankLLMethod();
		qType queueArbitration(const uint8_t& chanNum);
		void RRAMInterfaceMethod(uint64_t& cmd, const uint8_t& chanNum, \
			uint8_t* data, sc_core::sc_time& delay, const uint8_t& code);
		void decodeNextAddress(const uint64_t& cmd, int32_t& nextAddr);
		void getActiveCmdType(const uint64_t& cmd, cmdType& CmdType);
		void decodeLBA(uint64_t cmd, uint16_t& cwBank, bool& chipSelect, uint32_t& page);
		void decodeCommand(const uint64_t cmd, cmdType& ctype, uint16_t& cwBank, bool& chipSelect, uint32_t& page);
		void decodeActiveCommand(const uint64_t cmd, uint16_t& slotNum, cmdType& ctype, uint64_t& lba, uint8_t& queueNum, uint16_t& cwCnt);
		void createActiveCmdEntry(const uint64_t cmd, const int32_t queueAddr, uint64_t& queueVal);
		void createActiveCmdEntry(const uint16_t& slotNum, const uint8_t& queueAddr, \
			const cmdType& ctype, const uint16_t& lba, const uint16_t& cwCnt, const uint32_t& pageNum, uint64_t& cmdVal);
		void createPendingCmdEntry(const uint64_t cmd, uint64_t& cmdVal);
		void decodePendingCmdEntry(const uint64_t cmd, cmdType& ctype, \
			uint16_t& lba, uint16_t& cwCnt, uint16_t& slotNum, uint8_t& queueAddr);
		void createCompletionEntry(const cmdType cmd, const uint16_t slotNum, const uint16_t cwCnt, uint16_t& value);

		void getQueueAddress(const uint64_t& cmd, uint32_t& addr);
		void createPayload(bool& chipSelect, const cmdType ctype, uint16_t cwBank, \
			uint32_t page, const uint8_t& code, uint8_t* dataPtr, tlm::tlm_generic_payload* payloadPtr);
		void createPayload(uint8_t* data, uint8_t& count, const uint32_t& length, uint8_t* dataPtr);
		void createPayload(uint8_t data, const uint32_t& length, uint8_t* dataPtr);
		
		void processShortQueueCmd(uint8_t chanNum);
		void processLongQueueCmd(uint8_t chanNum);

		void DispatchLongQueue(uint8_t chanNum, uint64_t queueVal, sc_core::sc_time timeVal, uint16_t cwBankIndex);
		void DispatchShortQueue(uint8_t chanNum, uint64_t queueVal, sc_core::sc_time timeVal, uint16_t cwBankIndex);
		void cmdDispatcherProcess(uint8_t chanNum);
		void checkCmdDispatcherBankStatus(uint8_t chanNum);
		void pollCmdDispatcherBankStatus(uint8_t chanNum);
		
		void chanManagerProcess(uint8_t chanNum);
		void pendingWriteCmdProcess(uint8_t chanNum);
		void pendingReadCmdProcess(uint8_t chanNum);
		void completionMethod();
		
		void initChannelSpecificParameters();
		void createReportFiles();
		void initMasks();

		/*Helper Functions*/
		void processPCMDQueue(uint8_t* dataPtr, sc_time& delay, uint16_t& slotNum);
		void processData(tlm::tlm_generic_payload& trans, uint8_t* dataPtr, sc_time& delay, uint16_t& slotNum);
		void readCompletionQueue_mode0(uint32_t& length, uint8_t* dataPtr);
		void readCompletionQueue_mode1(uint32_t& length, uint8_t* dataPtr, uint16_t& slotNum);
		void processBankLinkList(uint16_t cwBankIndex, uint64_t& cmd, sc_core::sc_time& timeVal, uint8_t chanNum);
		void setCmdTransferTime(const uint8_t& code, sc_core::sc_time& tDelay, sc_core::sc_time& delay, uint8_t chanNum);

		uint64_t getAddress(uint32_t& page, uint16_t& bank, bool& chipSelect);
		uint32_t getBankMask();
		uint64_t getPageMask();
		uint64_t getChanMask();
		uint64_t getLBAMask();
		uint32_t getCwBankMask();
		uint64_t getActivePageMask();
		uint64_t getActiveLBAMask();
		uint64_t getPendingLBAMask();
		cmdType  getPhysicalCmdType(const uint64_t& cmd);
		uint16_t getCwBankIndex(const uint16_t& lba);
		void     end_of_simulation();
		uint32_t mBankMask;
		uint64_t mPageMask;
		uint64_t mChanMask;
		uint64_t mLBAMask;
		uint32_t mCwBankMask;
		uint64_t mActivePageMask;
		uint64_t mActiveLBAMask;
		uint64_t mPendingLBAMask;

		std::vector<sc_spawn_options* > mCmdDispatcherProcOpt;
		std::vector<sc_spawn_options* >  mPendingCmdProcOpt;
		std::vector<sc_spawn_options* >  mPendingReadCmdProcOpt;
		std::vector<sc_spawn_options* > mCheckRespProcOpt;
		std::vector<sc_spawn_options* > mChanMgrProcOpt;

		void checkRespProcess(uint8_t chanNum);

		std::vector<std::vector<int32_t> > queueHead;
		std::vector<std::vector<int32_t> > queueTail;
		std::vector<std::vector<cwBankStatus> > mCmdDispBankStatus;
		std::queue<uint16_t> mSlotNumQueue;
		sc_event_queue mCmdCompleteEvent;
		std::fstream mLogFileHandler;
	};

}

#endif