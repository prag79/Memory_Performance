/*******************************************************************
 * File : TeraSPCIeController.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Command Parameter Table used in
 * TeraSPCIeController architecture to store command parameters. It is indexed
 * by free iTags.
 * Related Specifications: 
 * PCIeControllerSpec.doc
 ********************************************************************/
#ifndef __TERAS_PCI_CONTROLLER_H
#define __TERAS_PCI_CONTROLLER_H

#include "TeraSPCIeControllerBase.h"
#include <queue>
#include <list>
#include <sysc/communication/sc_mutex.h>

namespace  CrossbarTeraSLib {
	static const char *filename = "TeraSPCIeController.cpp";

	typedef enum activeQueueType qType;

	class TeraSPCIeController : public sc_module, public TeraSPCIeControllerBase
	{
	public:


		//PCIe Interface
		tlm_utils::simple_target_socket<TeraSPCIeController, PCI_BUS_WIDTH, tlm::tlm_base_protocol_types> pcieRxInterface;
		tlm_utils::simple_initiator_socket<TeraSPCIeController, PCI_BUS_WIDTH, tlm::tlm_base_protocol_types> pcieTxInterface;

		/** b_transport registered with simple_target_socket
		* pcieRxInterface
		* This function implements Rx interface
		* Has following function
		* Controller register read/write,
		* Completion TLP from the host is received via this port
		* @return uint32_t Page size
		**/
		void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);


		//Memory Interface
		std::vector < tlm_utils::simple_initiator_socket_tagged<TeraSPCIeController, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>* > pOnfiBus;
		std::vector<sc_out<bool>* > pChipSelect;

		/** nb_transport_bw
		* @param int chanNum
		* @param tlm::tlm_generic_payload& generin payload
		* @param tlm::tlm_phase phase
		* @param sc_time time
		* @return void
		**/
		virtual tlm::tlm_sync_enum nb_transport_bw(int id, tlm::tlm_generic_payload& trans, \
			tlm::tlm_phase& phase, sc_time& delay);



		TeraSPCIeController(sc_module_name nm, uint32_t queueSize, uint32_t queueFactor, uint32_t blockSize,\
			uint32_t CQ1BaseAddr, uint32_t SQ1BaseAddr, uint32_t CQ1Size, uint32_t SQ1Size, uint32_t cwSize, uint8_t chanNum,
			uint32_t pageSize, const sc_time readTime, const sc_time programTime, \
			uint8_t credit, uint32_t onfiSpeed, 
			uint32_t bankNum, uint8_t numDie, uint32_t pageNum,
			 bool enMultiSim, bool mode, double cmdTransferTime, uint32_t ioSize, \
			 uint32_t pcieSpeed,std::fstream &LogFileHandler, uint32_t tbQueueDepth, uint32_t wrBuffSize, uint32_t rdBuffSize)
			:sc_module(nm)
			, TeraSPCIeControllerBase(CQ1BaseAddr, SQ1BaseAddr, CQ1Size, SQ1Size, cwSize, chanNum, pageSize, readTime, programTime, credit, onfiSpeed, bankNum, numDie,
			pageNum, enMultiSim, mode, cmdTransferTime, ioSize, pcieSpeed, tbQueueDepth)
			, mTimeOut(0)
			, mHostCmdQueue(queueSize)
			, mTagCounter(0)
			, mParamTable(queueSize)
			, mTagMgr(queueSize)
			, mWriteDataBuff(cwSize, wrBuffSize)
			, mReadDataBuff(cwSize, rdBuffSize)
			, mCmdQueue(chanNum, mCodeWordNum, blockSize, cwSize, bankNum, mCodeWordNum * chanNum * queueFactor, pageSize, pageNum)
			, mBankLinkList(mCodeWordNum, chanNum)
			, mRunningCntTable(queueSize, cwSize)
			, mTLPQueueMgr(queueSize, cwSize)
			, mPhaseTagCQ1(false)
			, mSizeOfCQ(CQ1Size)
			, mSizeOfSQ(SQ1Size)
			, mArbiterToken(0)
			, mNumCmd(0)
			, mNumCmdDispatched(0)
			, mNumPcmdCmd(0)
			, mSQ1TDBL(0)
			, isDoorBellSet(false)
			, mLogFileHandler(LogFileHandler)
			, mSQ1CmdLeft(0)
			, mIsWrBufferFull(false)
			, mIsRdBufferFull(false)
			, isSQCmdLeft(false)
			, mNumWrapAround(0)
		{
			SC_HAS_PROCESS(TeraSPCIeController);

#pragma region STATIC_THREADS_INSTANTIATION
			SC_THREAD(startCntrlr);
			sensitive << mCC_EnEvent;
			dont_initialize();

			SC_THREAD(arbiterThread);
			sensitive << mArbitrateEvent;
			dont_initialize();

			SC_THREAD(writeCmdDoneThread);
			sensitive << mWriteCmdDoneEvent;
			dont_initialize();

			SC_THREAD(pcmdQInsertThread);
			sensitive << mTrigPcmdQEvent;

			SC_THREAD(memReadReqCmdThread)
			sensitive << mTrigMemReadCmdEvent;
			dont_initialize();

			SC_THREAD(readDoneThread);
			sensitive << mReadDoneEvent;
			dont_initialize();

			
#pragma endregion 
			
			mMemReadTLP = new tMemoryReadReqTLP();
			pcieRxInterface.register_b_transport(this, &TeraSPCIeController::b_transport);

			if (mCwBankNum > 2)
			{
				mCwBankMaskBits = (uint32_t)(log2(double(mCwBankNum / mNumDie)));
			}
			else {
				mCwBankMaskBits = 1;
			}

			mWriteBuffAddr.resize(chanNum);
			initChannelSpecificParameters();

			createReportFiles();


		}

		~TeraSPCIeController();
	private:

#pragma region MEMBER_FUNCTIONS
		/** Decodes the type of TLP based on the type and format of the
		* TLP field
		* @param tDw0MemoryTLP* payload TLP payload to be decoded
		* @param uint16_t& length length of the payload
		* @param tlpType& tlpType: typr of the TLP returned
		* @return bool returns true if it is a valid TLP
		**/
		bool decodeTLPType(tDw0MemoryTLP* payload, uint16_t& length, tlpType& tlpType);
		void getCfgWrPayload(tConfigWriteReq* data, uint32_t& address, uint64_t& payload);
		void getTLPPayloadPtr(uint32_t* data, uint8_t* payload);
		void getCompletionTLPFields(tCompletionTLP* data, uint16_t& byteCnt, uint16_t& compID, uint16_t& reqID, \
			uint8_t& lwrAddr, uint8_t& tag);

		void parseSQFromCompletionTLP(tSubQueue* sqEntry, HCmdQueueData* hCmdEntry, uint32_t index, tCompletionTLP* completionTLP);

		/** Get Memory Page Size
		* Calculate the memory page size
		* @return uint32_t Page size
		**/
		uint32_t getMemPageSize();

		void PCIeAdminCntrlrMethod();
		void parseData(tCompletionTLP* completionTLP, sc_time& delay);

		/** The function is used to create bank link list
		** for each channel
		* @param qAddr Address of the command in PCMD queue
		* @return void
		**/
		void TeraSBankLLMethod(int32_t qAddr);

		/** Get  address
		* @param uint64_t page
		* @param uint8_t bank
		* @return uint64_t
		**/
		uint64_t getAddress(uint32_t& page, uint8_t& bank, bool& chipSelect);
		void createDMAQueueCmdEntry(ActiveCmdQueueData& activeQueue, uint8_t& qAddr, ActiveDMACmdQueueData& qData);
		/**  create generic payload
		* @param chipSelect chip select
		* @param ctype command type
		* @param cwBank code word Bank
		* @param page page
		* @param code ONFI command code
		* @param dataptr pointer to the payload
		* @return void
		**/
		void createPayload(bool& chipSelect, const cmdType ctype, uint8_t cwBank, \
			uint32_t page, const uint8_t& code, uint8_t* dataPtr, tlm::tlm_generic_payload* payloadPtr);

		void createCQ1Entry(uint16_t cid, cmdType cmd, bool phaseTag, uint16_t status, uint16_t sq1Head, uint16_t sqID, uint8_t* data);

		void setCmdTransferTime(const uint8_t& code, sc_core::sc_time& tDelay, sc_core::sc_time& delay, uint8_t chanNum);
		void end_of_simulation();

		void chipSelectMethod(uint8_t chanNum);

		/** Initialize all the channel related parameters
		* in the constructor
		* @return void
		**/
		void initChannelSpecificParameters();

		void createReportFiles();
#pragma endregion MEMBER_FUNCTIONS

#pragma region INTERFACE_METHODS_HELPER
	void writeCCReg(uint64_t payload);
	void writeAQAReg(uint64_t payload);
	void writeASQReg(uint64_t payload);
	void writeACQReg(uint64_t payload);
	void writeSQ0TDBLReg(uint64_t payload, sc_time delay);
	void writeCQ0HDBLReg(uint64_t payload, sc_time delay);
	void writeSQ1TDBLReg(uint64_t payload, sc_time delay);
	void writeCQ1HDBLReg(uint64_t payload, sc_time delay);
	void processCompletionTLPCmd(uint8_t* dataPtr, sc_time delay);
#pragma endregion

#pragma region CHAN_MANAGER_HELPER
		void dispatchShortCmd(uint8_t chanNum);
		void dispatchWriteCmd(uint8_t chanNum, ActiveDMACmdQueueData& queueData);
		void dispatchReadCmd(uint8_t chanNum, ActiveDMACmdQueueData& queueData);
#pragma endregion

#pragma region CMD_DISPATCHER_HELPER
		void pushLongQueueCmd(uint8_t chanNum, uint8_t cwBankIndex, ActiveCmdQueueData queueVal);
		void pushShortQueueCmd(uint8_t chanNum, uint8_t cwBankIndex, ActiveCmdQueueData queueVal);
#pragma endregion
#pragma region TLP_CREATION_METHODS
		void createMemReadTLP(uint64_t& address,uint32_t& length, const tagType tag, uint8_t& chanNum);

		/** Create Memory Write TLP format
		* to write to either host memory or Completion Queue
		* @param address host address
		* @param qType Queue Type:Completion queue or data queue
		* @param qAddr Read Data Buffer address from where data needs to be
		*        fetched
		* @return void
		**/
		void createMemWriteTLP(uint64_t& address, hostQueueType qType, uint32_t& qAddr, uint8_t* data);

#pragma endregion TLP_CREATION_METHODS

#pragma region STATIC_THREADS
		/** Arbiter Method to arbitrate on PCIe Bus
		* to write to either host memory or submission queue or Completion Queue
		* The arbiter is a round robin arbiter
		* @return void
		**/
		void arbiterThread();
		sc_event_queue mArbitrateEvent;

		/** Starts the controller
		* to write to either host memory or Completion Queue
		* @param address host address
		* @param qType Queue Type:Completion queue or data queue
		* @param qAddr Read Data Buffer address from where data needs to be
		*        fetched
		* @return void
		**/
		void startCntrlr();

		/** Static thread process used to fetch
		* host command from Host cmd Fifo and replicate it in available PCMDQ
		* Queue addresses. This thread is blocking and will wait till
		* there is space in the PCMD queue.
		* It is statically sensitive to mTrigPcmdQEvent
		* This event is notified when there is new command in
		* the Host Cmd Queue
		* @return void
		**/
		void pcmdQInsertThread();
		sc_event mTrigPcmdQEvent;

		void writeCmdDoneThread();

		void memReadReqCmdThread();
		sc_event mTrigMemReadCmdEvent;
		sc_event mHCmdQNotFullEvent;
		sc_event mCompletionTLPRxEvent;
		sc_event mMemRdReqEventDone;
		sc_event mDataTxDoneEvent;

#pragma endregion STATIC_THREADS

#pragma region STATIC_METHODS
	

		/** This Method is notified when a host WRITE command
		* completes execution
		* this thread releases controller resources and creates a completion queue TLP
		* and pushes it into Memory Write to Completion Queue TLP.
		* Statically sensitive to mWriteCmdDoneEvent event.
		* @return void
		**/
		//void writeCmdDoneMethod();
		sc_event mWriteCmdDoneEvent;

		/** Memory Read Request Command
		* This is a static thread which creates Memory read request TLP
		* on receiving new SQ1 Tail doorbell and when the
		* controller is ready(CSTS.RDY = 1)
		* Memory Read Request is sent only if there
		* is space in the Host Command Queue and Memory
		* read Request Cmd TLP
		* This thread is statically sensitive to mTrigMemReadCmdEvent
		* The thread is notified when host rings SQ1 tail doorbell register or
		* when host command has space and the controller can fetch some
		* more commands from the host SQ1
		* @return void
		**/
	//	void memReadReqCmdMethod();
		
		void readDoneThread();
#pragma endregion STATIC_METHODS

#pragma region ARBITRATION
		/** Arbiter protocol to arbitrate on PCIe Bus
		* The arbiter is a round robin arbiter
		* @return void
		**/
		TLPQueueType queueArbiter();

		/** QueueArbitration
		* responsible for arbitration of cmds between
		* short and DMA queue
		* @param chanNum channel Number
		* @return SHORT/DMA
		**/
		qType queueArbitration(const uint8_t& chanNum);

#pragma endregion ARBITRATION
		
#pragma region RRAM_IF_METHODS		
		/** RRAM Interface -Overloaded Method
		* creates payload and send data across ONFI
		* @param cmd Active Command Queue Entry
		* @param chanNum Channel Number
		* @param * data  Data Pointer
		* @param delay Command latency
		* @param code ONFI Command Code
		* @return void
		**/
		void RRAMInterfaceMethod(ActiveCmdQueueData& cmd, const uint8_t& chanNum, \
			uint8_t* data, sc_core::sc_time& delay, const uint8_t& code);

		/** RRAM Interface -Overloaded Method
		* creates payload and send data across ONFI
		* @param cmd Active DMA Command Queue Entry
		* @param chanNum Channel Number
		* @param * data  Data Pointer
		* @param delay Command latency
		* @param code ONFI Command Code
		* @return void
		**/
		void RRAMInterfaceMethod(ActiveDMACmdQueueData& cmd, const uint8_t& chanNum, uint8_t* data, sc_core::sc_time& delay,
			const uint8_t& code);

#pragma endregion RRAM_IF_METHODS

		/* Dynamic Threads and Methods*/
#pragma region DYNAMIC_THREADS
		/** Command Dispatcher thread
		* This thread checks for the status of each bank for a particular
		* channel and dispatches command if that particular bank is free else
		* hops over to other bank, this process is repeated. If all the banks
		* are busy then it comes out of arbitration and waits for banks to be free again
		* @param chanNum Channel Number
		* @return void
		**/
		void cmdDispatcherThread(uint8_t chanNum);

		/** Channel Manager thread
		* This thread arbitrates between short queue and DMA queue
		* on the ONFI channel, sends cmd/data to the RRAM Interface
		* @param chanNum Channel Number
		* @return void
		**/
		void chanManagerThread(uint8_t chanNum);
		
		/** Dynamic thread pops WRITE Command from ALCQ,
		* fetches the data from host and
		* pushes the command into ADCQ
		* @param chanNum Channel Number
		* @return void
		**/
		void longQueueThread(uint8_t chanNum);

		/** This Dynamic thread is notified when a READ sub command is sent to the ONFI channel.
		* READ sub-cmd is pushed into the pending queue and waits for readTime to elapse before
		* it is popped out of the pending queue before being pushed into the DMA queue to enable
		* DMA transfer.
		* @param chanNum Channel Number
		* @return void
		**/
		void pendingReadCmdThread(uint8_t chanNum);

		void dispatchReadCmdThread(uint8_t chanNum);

		void sendWriteCmdThread(uint8_t chanNum);

#pragma endregion DYNAMIC_THREADS

#pragma region DYNAMIC_METHODS
		/** This method is notified when a WRITE sub-command is done sending data 
		 * to the memory, WRITE command is pushed from the DMA queue into the
		 * pending queue and once pending queue time is elapsed it completes
		 * the write sub-command. If all the sub-commands of a WRITE command is 
		 * processed then it notifies Command done event.
		 * @param chanNum Channel Number
		 * @return void
		 **/
		void pendingWriteCmdMethod(uint8_t chanNum);

		/** This method is notified when a READ sub command is sent to the ONFI channel.
		 * READ sub-cmd is pushed into the pending queue and waits for readTime to elapse before
		 * it is popped out of the pending queue before being pushed into the DMA queue to enable
		 * DMA transfer.
		* @param chanNum Channel Number
		* @return void
		**/
		void pendingReadCmdMethod(uint8_t chanNum);

		/** checkRespProcess
		* Final Phase of transaction processing
		* @param chanNum channel number
		* @return void
		**/
		void checkRespMethod(uint8_t chanNum);

		void memReadCompletionThread(uint8_t chanNum);
#pragma endregion DYNAMIC_METHODS		
		
	
#pragma region MEMBER_VARIABLES
		enum buffType{
			READ,
			WRITE
		};

		uint16_t mTimeOut : 8;
		tMemoryReadReqTLP* mMemReadTLP;
		
		uint32_t mLength;

		/*Data Structure to keep track of write data buffer address*/
		std::vector<std::queue<uint8_t> > mWriteBuffAddr;
		std::queue<uint8_t> mWriteBuffAddrQueue;
		std::queue<uint8_t> mBuffAddrQueue;
		uint8_t mTagCounter;
		uint8_t mArbiterToken;
		bool mPhaseTagCQ1;
		std::fstream &mLogFileHandler;
		uint32_t mSizeOfSQ;
		uint32_t mSizeOfCQ;
		uint32_t mNumCmd;
		uint32_t mNumPcmdCmd;
		uint32_t mNumCmdDispatched;
		uint32_t mSQ1TDBL;
		bool isDoorBellSet;
		uint32_t mSQ1CmdLeft;

		/*Flag is set if write buffer
		is full*/
		bool mIsWrBufferFull;

		/*Flag is set if read buffer
		is full*/
		bool mIsRdBufferFull;

		bool isSQCmdLeft;

		uint32_t mNumWrapAround;
#pragma endregion MEMBER_VARIABLES


#pragma region MEMBER_OBJECTS
		HostCmdQueueMgr mHostCmdQueue;
		CmdParamTable mParamTable;
		iTagManager mTagMgr;
		DataBufferManager mWriteDataBuff;
		DataBufferManager mReadDataBuff;
		CommandQueueManager mCmdQueue;
		CwBankLinkListManager mBankLinkList;
		RunningCntTable mRunningCntTable;
		TLPManager mTLPQueueMgr;
		MemMgr mMemMgr;
		TransManager mTransMgr;

#pragma endregion MEMBER_OBJECTS
						
		
#pragma region EVENTS_VARIABLES
		std::vector<uint8_t> mOnfiCode;

		/*trigger for command dispatcher and Channel Manager*/
		std::vector<sc_event* > mCmdDispatchEvent;
		
		/* Active queue not full trigger event*/
		std::vector<sc_event* > mAlcqNotFullEvent;
		std::vector<sc_event* > mAdcqNotFullEvent;
		std::vector<sc_event* > mAscqNotFullEvent;

		/* Active queue not empty trigger event*/
		std::vector<sc_event* > mAlcqNotEmptyEvent;
		std::vector<sc_event* > mAdcqNotEmptyEvent;
		std::vector<sc_event* > mAscqNotEmptyEvent;

		std::vector<sc_event* > mTrigCmdDispEvent;
	
		/*4 phase TLM nb protocol events*/
		std::vector<sc_event* > mEndReqEvent;
		std::vector<sc_event* > mEndReadEvent;
		std::vector<sc_event* > mChipSelectEvent;
		std::vector<sc_event* > mChipSelectTxEvent;
		std::vector<sc_event* > mDispatchCmdEvent;
		std::vector<sc_event* > mDispatchWriteCmdEvent;
		std::vector<sc_event*> mReadDataRxEvent;
		std::vector<sc_event*> mWriteDataSentEvent;
		std::vector<sc_event* > mDispatchOnBuffFreeEvent;
		/*Write Buffer has space event, it is triggered for each 
		channel*/
		//std::queue<sc_event* > mWrBuffHasSpaceEvent;

		sc_event mWrBuffHasSpaceEvent;
		sc_event mRdBuffHasSpaceEvent;

		sc_event mCmdQNotFullEvent;
		/*Event triggered when Completion w/ data TLP Received from
		host*/
		std::vector<sc_event* > mCmplTLPDataRxEvent;

		/* Event to notify when read data has to be send to the host*/
		std::vector<sc_event_queue* > mReadDataSendEvent;
		
		/*Event to notify that there is space in the TLP queues*/
		sc_event mMemReadCmdQNotFullEvent;
		sc_event mMemWriteDataQNotFullEvent;
		sc_event mMemWriteCompQNotFullEvent;
		sc_event mMemReadDataQNotFullEvent;
		sc_event_queue mReadDoneEvent;
		sc_event mCQEmptyEvent;

		sc_event mCmdTxDoneEvent;
		sc_event mTrigReadsCmdCmpltEvent;
		sc_event mCmplTLPRxDataEvent;
		
		
		

#pragma endregion EVENT_VARIABLES

		
#pragma region SPAWN_OPTIONS
		std::vector<sc_spawn_options* > mCmdDispatcherProcOpt;
		std::vector<sc_spawn_options* >  mPendingCmdProcOpt;
		std::vector<sc_spawn_options* >  mPendingReadCmdProcOpt;
		std::vector<sc_spawn_options* > mCheckRespProcOpt;
		std::vector<sc_spawn_options* > mChanMgrProcOpt;
		std::vector<sc_spawn_options* > mLongQThreadOpt;
		std::vector<sc_spawn_options* > mReadCmpltMethodOpt;
		std::vector<sc_spawn_options* > mChipSelectProcOpt;
		std::vector<sc_spawn_options* > mDispatchReadCmdProcOpt;
		std::vector<sc_spawn_options* > mSendWriteCmdProcOpt;
#pragma endregion SPAWN_OPTIONS

#pragma region VECTOR_VARIABLES
		/* queue used by Cw Bank LL Manager*/
		std::vector<std::vector<int32_t> > queueHead;
		std::vector<std::vector<int32_t> > queueTail;
		std::vector<std::vector<cwBankStatus> > mCmdDispBankStatus;
		/*peq used by nb_transport call*/
		std::vector<tlm_utils::peq_with_get<tlm::tlm_generic_payload>* >  mRespQueue;

		std::vector<ActiveCommandQueue> mAlcq;
		std::vector<ActiveCommandQueue> mAscq;
		std::vector<ActiveDMACmdQueue> mAdcq;

		std::vector<std::unique_ptr<PendingCmdQueueManager> > mPendingCmdQueue;
		std::vector<std::unique_ptr<PendingCmdQueueManager> > mPendingReadCmdQueue;
		std::vector<std::vector<cwBankStatus> > mCwBankStatus;
	
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
		std::vector<uint8_t*> mReadData;
		std::vector<uint8_t*> mWriteData;
#pragma endregion VECTOR_VARIABLES

#pragma region QUEUE_VARIABLES
		/* Data structure to keep track of tags*/
		std::queue<uint16_t> mTagQueue;

		/*Data structure to keep track of number of subcommands to
		  be inserted in the command queue*/
		std::queue<uint32_t> mCmdSize;
		std::queue<HCmdQueueData> mHCmdQueue;
		std::queue<uint32_t> mCmpltTagQueue;
		
		/*This tag queue is only used to keep track of the
		tags while inserting sub-commands in the physical queue*/
		std::queue<uint32_t> mFreeTagQueue;

		std::queue<uint32_t> mTailAddrQueue;
		/*This queue is used to keep track of the command time
		 when it is dispatched from the physical cmd queue to the 
		 ONFI channel*/
		std::queue<sc_time> mDelayQueue;

		std::vector<std::queue<ActiveDMACmdQueueData> > mDMAQueueData;
		std::vector<std::queue<ActiveDMACmdQueueData> > mDMADoneQueue;

		std::queue<ActiveDMACmdQueueData> mDMAActiveCmdQueueData;

		std::list<ReadCmdDispatchData> mActiveDMACmdQueue;
		std::list<WriteCmdDispatchData> mActiveWriteCmdQueue;

		std::queue<bool> mChipSelectQueue;

		std::queue<uint8_t> mWrBuffAddr;

		std::queue<uint32_t> mCmdDoneTagQueue;

		std::queue<uint8_t> mRdBuffPtrQueue;
#pragma endregion QUEUE_VARIABLES		

		sc_mutex mChipSelectMutex;
		std::vector<sc_core::sc_mutex*> mOnfiBusMutex;
	};
}
#endif