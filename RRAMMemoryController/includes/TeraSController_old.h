/*******************************************************************
 * File : TeraSController.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of TeraS Controller model and
 * its architecture.
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
#include "Common.h"
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
	enum bankStatus
	{
		FREE = 0,
		BUSY = 1
	};
	typedef bankStatus cwBankStatus;
	enum queueType
	{
		SHORT = 0,
		LONG = 1,
		NONE = -1
	};
	typedef enum queueType qType;

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
			mLBAMaskBits = mPageMaskBits + mCwBankMaskBits + mChanMaskBits + 1;
			mActiveLBAMaskBits = mPageMaskBits + mCwBankMaskBits + 1;
			mPendingLBAMaskBits = mCwBankMaskBits + 1;

			mBankMask = getBankMask();
			mCwBankMask = getCwBankMask();
			mActiveLBAMask = getActiveLBAMask();
			mActivePageMask = getActivePageMask();
			mChanMask = getChanMask();
			mLBAMask = getLBAMask();
			mPendingLBAMask = getPendingLBAMask();

			string ctrlLogFile = "./Logs/TeraSMemoryController";
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
		
			mReqInProgressPtr = nullptr;
			ddrInterface.register_b_transport(this, &TeraSController::b_transport);
		
			for (uint8_t chanIndex = 0; chanIndex < chanNum; chanIndex++)
			{
				mCmdDispBankStatus.push_back(std::vector<cwBankStatus>(mCodeWordNum,cwBankStatus::FREE));
				mOnfiCode.push_back(0x00);
				mLongQueueSize.push_back(0x0);
				mShortQueueSize.push_back(0x0);

				mOnfiChanTime.push_back(0);
				mOnfiTotalTime.push_back(0);

				mOnfiStartActivity.push_back(0);
				mOnfiEndActivity.push_back(0);
				mOnfiChanActivityStart.push_back(false);

				mAlcq.push_back(ActiveCommandQueue(mCodeWordNum));
				mAscq.push_back(ActiveCommandQueue(mCodeWordNum));
				mAvailableCredit.push_back(credit);
				mCwBankStatus.push_back(std::vector<cwBankStatus>(mCodeWordNum, cwBankStatus::FREE));
			
				pOnfiBus.push_back(new tlm_utils::simple_initiator_socket_tagged<TeraSController, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>(sc_gen_unique_name("pOnfiBus")));
				pOnfiBus.at(chanIndex)->register_nb_transport_bw(this, &TeraSController::nb_transport_bw, chanIndex);
				
			
				mCmdDispatchEvent.push_back(new sc_event(sc_gen_unique_name("mCmdDispatchEvent")));
				mChanMgrEvent.push_back(new sc_event(sc_gen_unique_name("mChanMgrEvent")));
				mAlcqNotFullEvent.push_back(new sc_event(sc_gen_unique_name("mAlcqNotFullEvent")));
				mAscqNotFullEvent.push_back(new sc_event(sc_gen_unique_name("mAscqNotFullEvent")));
				mAlcqNotEmptyEvent.push_back(new sc_event(sc_gen_unique_name("mAlcqNotEmptyEvent")));
				mAscqNotEmptyEvent.push_back(new sc_event(sc_gen_unique_name("mAscqNotEmptyEvent")));
				mEndReqEvent.push_back(new sc_event(sc_gen_unique_name("mEndReqEvent")));
				mBegRespEvent.push_back(new sc_event(sc_gen_unique_name("mBegRespEvent")));
				mTrigCmdDispEvent.push_back(new sc_event(sc_gen_unique_name("mTrigCmdDispEvent")));
				mEndReadEvent.push_back(new sc_event(sc_gen_unique_name("mEndReadEvent")));
				mCreditPositiveEvent.push_back(new sc_event(sc_gen_unique_name("mEndReadEvent")));

				mRespQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload>(sc_gen_unique_name("respQueueEvent")));
				
				pChipSelect.push_back(new sc_core::sc_out<bool>);
				mPendingCmdQueue.push_back(new PendingCmdQueueManager(sc_gen_unique_name("mPendingQueue")));
				mPendingReadCmdQueue.push_back(new PendingCmdQueueManager(sc_gen_unique_name("mPendingReadQueue")));
				
				mReadData.push_back(new uint8_t[mCodeWordSize]);
				mWriteData.push_back(new uint8_t[mCodeWordSize]);
				queueHead.push_back(std::vector<int32_t>(mCodeWordNum, -1));
				queueTail.push_back(std::vector<int32_t>(mCodeWordNum, -1));
			
				mCmdDispatcherProcOpt.push_back(new sc_spawn_options());

				mCmdDispatcherProcOpt.at(chanIndex)->set_sensitivity(mCmdDispatchEvent.at(chanIndex));
				mCmdDispatcherProcOpt.at(chanIndex)->dont_initialize();
				sc_spawn(sc_bind(&TeraSController::cmdDispatcherProcess, \
					this, chanIndex), sc_gen_unique_name("cmdDispatcherProcess"), mCmdDispatcherProcOpt.at(chanIndex));

				mChanMgrProcOpt.push_back(new sc_spawn_options());
				
				sc_spawn(sc_bind(&TeraSController::chanManagerProcess, \
					this, chanIndex), sc_gen_unique_name("chanManagerProcess"), mChanMgrProcOpt.at(chanIndex));

				mPendingCmdProcOpt.push_back(new sc_spawn_options());
				mPendingCmdProcOpt.at(chanIndex)->spawn_method();
				mPendingCmdProcOpt.at(chanIndex)->set_sensitivity(&mPendingCmdQueue.at(chanIndex)->get_event());
				mPendingCmdProcOpt.at(chanIndex)->dont_initialize();
				sc_spawn(sc_bind(&TeraSController::pendingCmdProcess, \
					this, chanIndex), sc_gen_unique_name("pendingCmdProcess"), mPendingCmdProcOpt.at(chanIndex));
				
				mPendingReadCmdProcOpt.push_back(new sc_spawn_options());
				mPendingReadCmdProcOpt.at(chanIndex)->spawn_method();
				mPendingReadCmdProcOpt.at(chanIndex)->set_sensitivity(&mPendingReadCmdQueue.at(chanIndex)->get_event());
				mPendingReadCmdProcOpt.at(chanIndex)->dont_initialize();
				sc_spawn(sc_bind(&TeraSController::pendingCmdReadProcess, \
					this, chanIndex), sc_gen_unique_name("pendingCmdReadProcess"), mPendingReadCmdProcOpt.at(chanIndex));

				mCheckRespProcOpt.push_back(new sc_spawn_options());
				mCheckRespProcOpt.at(chanIndex)->spawn_method();
				mCheckRespProcOpt.at(chanIndex)->set_sensitivity(&mRespQueue.at(chanIndex)->get_event());
				mCheckRespProcOpt.at(chanIndex)->dont_initialize();
				sc_spawn(sc_bind(&TeraSController::checkRespProcess, \
					this, chanIndex), sc_gen_unique_name("checkRespProcess"), mCheckRespProcOpt.at(chanIndex));
				
				try {
					if (mEnMultiSim){
						string filename = "./Reports/longQueue_report";
						mLongQueueReport = new Report(appendParameters(filename, ".log"));
						filename = "./Reports/shortQueue_report";
						mShortQueueReport = new Report(appendParameters(filename, ".log"));
						filename = "./Reports/cntrl_latency_report";
						mLatencyReport = new Report(appendParameters(filename, ".log"));
						filename = "./Reports/onfi_chan_util_report";
						mOnfiChanUtilReport = new Report(appendParameters(filename, ".log"));
						filename = "./Reports/onfi_chan_activity_report";
						mOnfiChanActivityReport = new Report(appendParameters(filename, ".log"));
						filename = "./Reports/longQueue_report";
						mLongQueueReportCsv = new Report(appendParameters(filename, ".csv"));
						mShortQueueReportCsv = new Report(appendParameters("./Reports/shortQueue_report",".csv"));
						mLatencyReportCsv = new Report(appendParameters("./Reports/cntrl_latency_report",".csv"));
						mOnfiChanUtilReportCsv = new Report(appendParameters("./Reports/onfi_chan_util_report",".csv"));
						mOnfiChanActivityReportCsv = new Report(appendParameters("./Reports/onfi_chan_activity_report",".csv"));
					}
					else{
						mLongQueueReport = new Report("./Reports/longQueue_report.log");
						mShortQueueReport = new Report("./Reports/shortQueue_report.log");
						mLatencyReport = new Report("./Reports/cntrl_latency_report.log");
						mOnfiChanUtilReport = new Report("./Reports/onfi_chan_util_report.log");
						mOnfiChanActivityReport = new Report("./Reports/onfi_chan_activity_report.log");

						mLongQueueReportCsv = new Report("./Reports/longQueue_report.csv");
						mShortQueueReportCsv = new Report("./Reports/shortQueue_report.csv");
						mLatencyReportCsv = new Report("./Reports/cntrl_latency_report.csv");
						mOnfiChanUtilReportCsv = new Report("./Reports/onfi_chan_util_report.csv");
						mOnfiChanActivityReportCsv = new Report("./Reports/onfi_chan_activity_report.csv");
					}
				}
				catch (std::bad_alloc& ba)
				{
					std::cerr << "TeraSMemoryDevice:Bad File allocation" << ba.what() << std::endl;
				}

				if (!mOnfiChanActivityReport->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "ONFI_CHAN_ACTIVITY_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}
				if (!mLongQueueReport->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "LONGQUEUE_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}

				if (!mShortQueueReport->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "SHORTQUEUE_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}
				if (!mLatencyReport->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "LATENCY_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}

				if (!mOnfiChanUtilReport->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "ONFI_CHANNEL_UTILIZATION_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}

				if (!mOnfiChanActivityReportCsv->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "ONFI_CHAN_ACTIVITY_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}
				if (!mLongQueueReportCsv->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "LONGQUEUE_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}

				if (!mShortQueueReportCsv->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "SHORTQUEUE_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}
				if (!mLatencyReportCsv->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "LATENCY_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}

				if (!mOnfiChanUtilReportCsv->openFile())
				{
					std::ostringstream msg;
					msg.str("");
					msg << "ONFI_CHANNEL_UTILIZATION_REPORT LOG: "
						<< " ERROR OPENING FILE" << std::endl;
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
					exit(EXIT_FAILURE);
				}
			}

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
				delete mPendingCmdQueue.at(chanIndex);
				delete mPendingReadCmdQueue.at(chanIndex);
				delete mChanMgrProcOpt.at(chanIndex);
				delete pOnfiBus.at(chanIndex);
				delete mEndReadEvent.at(chanIndex);
				delete mCmdDispatchEvent.at(chanIndex);
				delete mChanMgrEvent.at(chanIndex);
				delete mAlcqNotFullEvent.at(chanIndex);
				delete mAscqNotFullEvent.at(chanIndex);
				delete mAlcqNotEmptyEvent.at(chanIndex);
				delete mAscqNotEmptyEvent.at(chanIndex);
				delete mTrigCmdDispEvent.at(chanIndex);
				delete mEndReqEvent.at(chanIndex);
				delete mBegRespEvent.at(chanIndex);
				delete mCreditPositiveEvent.at(chanIndex);
			}
			mLongQueueReport->closeFile();
			delete mLongQueueReport;
			mShortQueueReport->closeFile();
			delete mShortQueueReport;
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
		uint8_t mBankNum;
		uint8_t mNumDie;
		bool   mEnMultiSim;
		bool mMode;
		cmdType mCmdType;
		std::vector<uint8_t> mOnfiCode;
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

		std::vector<PendingCmdQueueManager* > mPendingCmdQueue;
		std::vector<PendingCmdQueueManager* > mPendingReadCmdQueue;
		
		std::vector<slotTable> mSlotTable;

		CwBankLinkListManager mBankLinkList;
		std::vector<sc_event* > mCmdDispatchEvent;
		std::vector<sc_event* > mChanMgrEvent;
		std::vector<sc_event* > mAlcqNotFullEvent;
		std::vector<sc_event* > mAscqNotFullEvent;
		std::vector<sc_event* > mAlcqNotEmptyEvent;
		std::vector<sc_event* > mAscqNotEmptyEvent;
		std::vector<sc_event* > mTrigCmdDispEvent;
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

		std::vector<uint16_t> mLongQueueSize;
		std::vector<uint16_t> mShortQueueSize;

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

		std::vector<std::vector<cwBankStatus> > mCwBankStatus;
		TransManager mTransMgr;
		tlm::tlm_generic_payload* mReqInProgressPtr;

		Report*  mLongQueueReport;
		Report*  mShortQueueReport;
		Report*  mLatencyReport;
		Report*  mOnfiChanUtilReport;
		Report*  mOnfiChanActivityReport;

		Report*  mLongQueueReportCsv;
		Report*  mShortQueueReportCsv;
		Report*  mLatencyReportCsv;
		Report*  mOnfiChanUtilReportCsv;
		Report*  mOnfiChanActivityReportCsv;

		string appendParameters(string name, string format);

		char* renameLog(char *reportName);
		void decodeAddress(const uint64_t& address, QueueType& queueType, uint16_t& slotNum);
		void TeraSBankLLMethod();
		qType queueArbitration(const uint8_t& chanNum);
		void RRAMInterfaceMethod(uint64_t& cmd, const uint8_t& chanNum, \
			uint8_t* data, sc_core::sc_time& delay, const uint8_t& code);
		void decodeNextAddress(const uint64_t& cmd, int32_t& nextAddr);
		void getActiveCmdType(const uint64_t& cmd, cmdType& CmdType);
		void decodeLBA(uint64_t cmd, uint8_t& cwBank, bool& chipSelect, uint32_t& page);
		void decodeCommand(const uint64_t cmd, cmdType& ctype, uint8_t& cwBank, bool& chipSelect, uint32_t& page);
		void decodeActiveCommand(const uint64_t cmd, uint16_t& slotNum, cmdType& ctype, uint64_t& lba, uint8_t& queueNum, uint16_t& cwCnt);
		void createActiveCmdEntry(const uint64_t cmd, const int32_t queueAddr, uint64_t& queueVal);
		void createActiveCmdEntry(const uint16_t& slotNum, const uint8_t& queueAddr, \
			const cmdType& ctype, const uint8_t& lba, const uint16_t& cwCnt, const uint32_t& pageNum, uint64_t& cmdVal);
		void createPendingCmdEntry(const uint64_t cmd, uint32_t& cmdVal);
		void decodePendingCmdEntry(const uint32_t cmd, cmdType& ctype, \
			uint8_t& lba, uint16_t& cwCnt, uint16_t& slotNum, uint8_t& queueAddr);
		void createCompletionEntry(const cmdType cmd, const uint16_t slotNum, const uint16_t cwCnt, uint32_t& value);

		void getQueueAddress(const uint64_t& cmd, uint32_t& addr);
		void createPayload(bool& chipSelect, const cmdType ctype, uint8_t cwBank, \
			uint32_t page, const uint8_t& code, uint8_t* dataPtr, tlm::tlm_generic_payload* payloadPtr);
		void createPayload(uint8_t* data, uint8_t& count, const uint32_t& length, uint8_t* dataPtr);
		void createPayload(uint8_t data, const uint32_t& length, uint8_t* dataPtr);

		void cmdDispatcherProcess(uint8_t chanNum);
		void chanManagerProcess(uint8_t chanNum);
		void pendingCmdProcess(uint8_t chanNum);
		void pendingCmdReadProcess(uint8_t chanNum);
		

		uint64_t getAddress(uint32_t& page, uint8_t& bank, bool& chipSelect);
		uint32_t getBankMask();
		uint64_t getPageMask();
		uint64_t getChanMask();
		uint64_t getLBAMask();
		uint32_t getCwBankMask();
		uint64_t getActivePageMask();
		uint64_t getActiveLBAMask();
		uint64_t getPendingLBAMask();

		void end_of_simulation();
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
		std::fstream mLogFileHandler;
	};

}

#endif