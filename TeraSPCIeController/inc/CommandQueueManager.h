/*******************************************************************
 * File : CommandQueueManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of the Command Queue used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef CMD_QUEUE_MANAGER_H_
#define CMD_QUEUE_MANAGER_H_
#include "Common.h"
#include <map>
#include <cmath>
#include "systemc.h"

namespace CrossbarTeraSLib {
	class CommandQueueManager : public sc_core::sc_object
	{

	public:
		CommandQueueManager(uint32_t chanNum, uint32_t codeWordNum, \
			uint32_t blockSize, uint32_t codeWordSize, \
			uint8_t bankNum, uint32_t queueSize, uint32_t pageSize, uint32_t pageNum)
			: mChanNum(chanNum)
			, mCwNum(codeWordNum)
			, mQueueSize(queueSize)
			, mCwCount(blockSize / codeWordSize)
			, mSlotNum(0)
			, mAddrShifter((uint16_t)log2(blockSize / codeWordSize))
			, mBankMaskBits((uint32_t)log2(double(codeWordNum / 2)))
			, mCwBankNum((uint16_t)((bankNum * pageSize) / codeWordSize))
			, mPageMaskBits((uint32_t)log2(double(pageNum)))
			, mChanMaskBits((uint32_t)log2(double(chanNum)))
			, mCmdQSize(0)

		{
			if (mCwBankNum > 2){
				mCwBankMaskBits = (uint32_t)log2(double(mCwBankNum));
			}
			else
			{
				mCwBankMaskBits = 1;
			}
			mLBAMaskBits = mPageMaskBits + mCwBankMaskBits + mChanMaskBits + 1;

			mBankMask = getBankMask();
			mPageMask = getPageMask();
			mLBAMask = getLBAMask();
			mChanMask = getChanMask();

			//mCmdQueue.resize(mQueueSize, CmdQueueData());
			for (uint32_t qIndex = 0; qIndex < mQueueSize; qIndex++)
			{
				mCmdQStatus.insert(std::pair<uint32_t, status>(qIndex, status::FREE));
				mCmdQueue.push_back(CmdQueueData());
			}
		}

		void insert(const uint32_t queueNum, const uint8_t cmd, \
			const uint64_t plba, const uint32_t cmdOffset, const uint32_t iTag, \
			const int32_t nextAddr, const sc_core::sc_time delay);
		void fetchCommand(const uint32_t queueNum, cmdType& cmd, \
			uint64_t& plba, uint32_t& cmdOffset, uint32_t& iTag, \
			int32_t& nextAddr, sc_core::sc_time& delay);
		void updateNextAddress(const uint32_t& queueNum, int32_t& value);
		void fetchNextAddress(const uint32_t& queueNum, int32_t& value);
		void getCwCount(const uint16_t& address, uint16_t& cwCount);
		cmdType getCmdType(const uint32_t& queueNum);
		//void replicateCommand(const uint16_t& queueNum, const uint16_t& cwCount);
		void getSlotNum(uint16_t& address);
		void getPCMDQAddress(int32_t& address);
		void getQueueAddress(const uint16_t& slotNum, uint32_t& addr);
		void getLBAField(const uint32_t& queueNum, \
			uint8_t& chanNum, uint8_t& cwBank, uint8_t& chipSelect, uint32_t& pageNum);


		void decodeCommand(const uint64_t& cmd, uint16_t& slotNum, \
			cmdType& ctype, uint64_t& lba, uint16_t& cwCnt);


		bool getFreeQueueAddr(uint32_t& qAddr);
		void setQueueBusy(uint32_t& qAddr);


		/** Sets the address in the queue free
		* it can be re-used
		* @param qAddr queue address
		* @return void
		**/
		void setQueueFree(uint32_t& qAddr);
		void resetQueueStatus(uint32_t& qAddr);
		void getiTagNumber(uint32_t& qAddr, uint32_t& iTag);
		void getCmdOffset(uint32_t& qAddr, uint32_t& offset);
		bool isFull();
		bool isEmpty();
	private:
		uint32_t mChanNum;
		uint32_t mCwNum;
		uint32_t mQueueFactor;
		uint32_t mQueueSize;
		uint32_t mCwCount;
		uint16_t mNumSlot;
		uint16_t mSlotNum;
		uint16_t mAddrShifter;
		uint32_t mPcmdQueueAddress;
		uint32_t mBankMaskBits;
		uint16_t mCwBankNum;
		uint32_t mCwBankMaskBits;
		uint32_t mPageMaskBits;
		uint32_t mChanMaskBits;
		uint32_t mLBAMaskBits;

		std::vector<CmdQueueData> mCmdQueue;
		std::map<uint32_t, status> mCmdQStatus;

		uint32_t getBankMask();
		uint64_t getLBAMask();
		uint32_t getChanMask();
		uint64_t getPageMask();

		uint32_t mBankMask;
		uint64_t mLBAMask;
		uint32_t mChanMask;
		uint64_t mPageMask;
		uint32_t mCmdQSize;
	};
}
#endif