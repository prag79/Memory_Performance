#ifndef __TERAS_PCI_CONTROLLER_BASE_H
#define __TERAS_PCI_CONTROLLER_BASE_H
#include <cstdint>
#include <vector>
#include <map>
#include "pcie_common.h"
#include "MemMgr.h"
#include "NvmeQueue.h"
#include "Report.h"
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "reporting.h"
#include "NvmeReg.h"
//#include "circularQueue.h"
#include "HostCmdQueueMgr.h"
#include "CmdParamTable.h"
#include "iTagManager.h"
#include "DataBufferManager.h"
#include "CommandQueueManager.h"
#include "tlp.h"
#include "TLPManager.h"
#include "CwBankLinkListManager.h"
#include "ActiveCommandQueue.h"
#include "TransManager.h"
#include "PendingCmdQueueManager.h"
#include "RunningCntTable.h"
#include <cstdlib>

namespace  CrossbarTeraSLib {
	
	
	class TeraSPCIeControllerBase
	{
	public:
		TeraSPCIeControllerBase(uint32_t cq1BaseAddr, uint32_t sq1BaseAddr, uint32_t cq1Size, uint32_t sq1Size, uint32_t cwSize, uint8_t chanNum,
			uint32_t pageSize, const sc_time readTime, const sc_time programTime, \
			uint8_t credit, uint32_t onfiSpeed,
			uint32_t bankNum, uint8_t numDie, uint32_t pageNum,
			 bool enMultiSim, bool mode, double cmdTransferTime, uint32_t ioSize, uint32_t pcieSpeed, uint32_t tbQueueDepth)
			:regCAP(0)
			, regVS(0)
			, regINTMS(0)
			, regINTMC(0)
			, regCC(0)
			, regRES(0)
			, regCSTS(0)
			, regNSSR(0)
			, regACQ(0)
			, regAQA(0)
			, regASQ(0)
			, regSQ0TDBL(0)
			, regCQ0HDBL(0)
			, regSQ1TDBL(0)
			, regCQ1HDBL(0)
			, mCQ1Queue(false, cq1BaseAddr, cq1Size)
			, mSQ1Queue(true, sq1BaseAddr, sq1Size)
			, mChanNum(chanNum)
			, mReadTime(readTime)
			, mProgramTime(programTime)
			, mCwSize(cwSize)
			, mPageSize(pageSize)
			, mBankMaskBits((uint32_t)log2(double(bankNum)))
			, mPageMaskBits((uint32_t)log2(double(pageNum)))
			, mChanMaskBits((uint32_t)log2(double(chanNum)))
			, mPageNum(pageNum)
			, mCredit(credit)
			, mOnfiSpeed(onfiSpeed)
			//, mDDRSpeed(DDRSpeed)
			, mPcieSpeed(pcieSpeed)
			, mBankNum(bankNum)
			, mNumDie(numDie)
			, mCwBankNum((uint16_t)((bankNum * pageSize * numDie) / cwSize))
			, mCodeWordNum((uint32_t)((bankNum * pageSize * numDie) / cwSize))
			, mTotalReads(0)
			, mTotalWrites(0)
			, mDelay(SC_ZERO_TIME)
			, mCmdTransferTime(cmdTransferTime)
			, mECCDelay(1000)
			, mNumCmdWrites(0)
			, mNumCmdReads(0)
			, mNextReadCount(0)
			, mEnMultiSim(enMultiSim)
			, mQueueDepth(tbQueueDepth)
			, mMode(mode)
			, mCmdType(cmdType::READ)
			, mIOSize(ioSize)
			, mBlockSize(ioSize)
		{
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

			/*std::string ctrlLogFile = "./Logs/TeraSPCIeController";
			if (enMultiSim)
			{
				ctrlLogFile = appendParameters(ctrlLogFile, ".log");
			}

			mLogFileHandler.open(ctrlLogFile, std::fstream::trunc | std::fstream::out);
*/
		}
	protected:

		uint32_t getBankMask();
		uint64_t getPageMask();
		uint64_t getChanMask();
		uint64_t getLBAMask();
		uint32_t getCwBankMask();
		uint64_t getActivePageMask();
		uint64_t getActiveLBAMask();
		uint64_t getPendingLBAMask();

		void decodeCommand(const ActiveCmdQueueData cmd, cmdType& ctype, uint8_t& cwBank, bool& chipSelect, uint32_t& page);
		void decodeCommand(const ActiveDMACmdQueueData queueData, cmdType& ctype, \
			uint8_t& cwBank, bool& chipSelect, uint32_t& page);
		//void decodeActiveCommand(const ActiveCmdQueueData cmd, uint16_t& slotNum, cmdType& ctype, uint64_t& lba, uint8_t& queueNum, uint16_t& cwCnt);
		void createActiveCmdEntry(const cmdType cmd, const uint64_t plba, \
			const uint32_t cmdOffset, const uint32_t iTag, const uint32_t queueNum, const sc_time delay, ActiveCmdQueueData& queueVal);
		void createActiveCmdEntry(const uint16_t& slotNum, const uint8_t& queueAddr, \
			const cmdType& ctype, const uint8_t& lba, const uint16_t& cwCnt, const uint32_t& pageNum, uint64_t& cmdVal);
		/*void createPendingCmdEntry(const uint64_t cmd, uint32_t& cmdVal);
		*/
		void createPendingCmdQEntry(const ActiveDMACmdQueueData cmd, ActiveCmdQueueData& cmdVal);

		/** decode pending cmd entry
		* @param cmd pending cmd
		* @param cmdType cmd type
		* @param lba lba
		* @param chipSelect chipSelect
		* @param queueAddr queue address
		* @return void
		**/
		void decodePendingCmdEntry(const ActiveCmdQueueData cmd, cmdType& ctype, \
			uint8_t& lba, uint32_t& iTag, bool& chipSelect, uint8_t& queueAddr);

		/** decode LBA
		* @param cmd  cmd
		* @param cwbank cw bank
		* @param chipselect chip select
		* @param page page
		* @return void
		**/
		void decodeLBA(const uint64_t plba, uint8_t& cwBank, bool& chipSelect, uint32_t& page);

		std::string appendParameters(std::string name, std::string format);

		uint64_t regCAP;
		uint32_t regVS;
		uint32_t regINTMS;
		uint32_t regINTMC;
		uint32_t regCC;
		uint32_t regRES;
		uint32_t regCSTS;
		uint32_t regNSSR;
		uint32_t regAQA;
		uint64_t regASQ;
		uint64_t regACQ;
		uint32_t regSQ0TDBL;
		uint32_t regCQ0HDBL;
		uint32_t regSQ1TDBL;
		uint32_t regCQ1HDBL;

		/*Host parameter used here to tag log file*/
		uint32_t mQueueDepth;
		/* ONFI Memory parameters*/

		uint8_t mChanNum;
		uint32_t mCwSize;
		uint16_t mCwBankNum;
		uint8_t  mCredit;
		uint32_t mOnfiSpeed;
		uint32_t mBlockSize;
		uint32_t mIOSize;
		//uint32_t mDDRSpeed;
		uint32_t mPcieSpeed;
		uint16_t mAddrShifter;
		uint32_t mCodeWordNum;
		uint32_t mPageNum;
		uint8_t mBankNum;
		uint8_t mNumDie;
		bool   mEnMultiSim;
		bool mMode;

		uint32_t mPageSize;
		uint32_t mBankMaskBits;
		uint32_t mPageMaskBits;
		uint32_t mChanMaskBits;
		uint32_t mCwBankMaskBits;
		uint32_t mLBAMaskBits;
		uint32_t mActiveLBAMaskBits;
		uint32_t mPendingLBAMaskBits;

		uint32_t mBankMask;
		uint64_t mPageMask;
		uint64_t mChanMask;
		uint64_t mLBAMask;
		uint32_t mCwBankMask;
		uint64_t mActivePageMask;
		uint64_t mActiveLBAMask;
		uint64_t mPendingLBAMask;
		uint64_t mTotalReads;
		uint64_t mTotalWrites;
		uint64_t mNumCmdWrites;
		uint64_t mNumCmdReads;
		uint32_t mNextReadCount;

		sc_time mReadTime;
		sc_time mProgramTime;
		sc_time mDelay;

		double mCmdTransferTime;
		double mDataTransferTime;
		double mECCDelay;

		cmdType mCmdType;

		sc_core::sc_event mCC_EnEvent;
		sc_core::sc_event mSQ1TdblEvent;
		sc_core::sc_event mCQ1HdblEvent;
		sc_core::sc_event mSQ0TdblEvent;
		sc_core::sc_event mCQ0HdblEvent;

		/*keep track of head and tail of host queue*/
		CircularQueue mCQ1Queue;
		CircularQueue mSQ1Queue;

		//std::unique_ptr<Report>  mLongQueueReport;
		//std::unique_ptr<Report>  mDMAQueueReport;
		//Report*  mShortQueueReport;
		Report*  mLatencyReport;
		Report*  mOnfiChanUtilReport;
		Report*  mOnfiChanActivityReport;

		Report*  mLongQueueReportCsv;
		Report*  mDMAQueueReportCsv;
		Report*  mShortQueueReportCsv;
		Report*  mLatencyReportCsv;
		Report*  mOnfiChanUtilReportCsv;
		Report*  mOnfiChanActivityReportCsv;

		//std::fstream mLogFileHandler;

	private:
		/*Disabling copy constructor*/
		TeraSPCIeControllerBase(const TeraSPCIeControllerBase &);
	};
}
#endif
