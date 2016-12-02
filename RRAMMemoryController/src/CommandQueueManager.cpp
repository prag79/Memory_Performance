#include "CommandQueueManager.h"
#include "reporting.h"
#include <bitset>

namespace CrossbarTeraSLib {
	static const char *qmgr = "queueMgr.log";

	/** insert command in  CommandQueueManager
	* @param slotNum slot index
	* @param value cmd to be inserted
	* @param delay time
	* @return void
	**/
	void CommandQueueManager::insertCommand(const uint16_t& slotNum, const uint8_t* value, const sc_core::sc_time& delay)
	{
		std::ostringstream msg;
		uint32_t address = slotNum << mAddrShifter;
		memcpy(&(mCmdQueue.at(address).cmd), (cmdType*)value, 8);
		uint16_t slot;
		cmdType ctype;
		uint64_t lba;
		uint16_t cwCnt;
		decodeCommand(mCmdQueue.at(address).cmd, slot, ctype, lba, cwCnt);
		msg << "CommandQueueManager: "
			<< " @Time: " << dec << delay
			<< " Cmd Type :" << hex << (uint32_t)ctype
			<< " Slot Num: " << dec << (uint32_t)slot
			<< " Cw Count: " << dec << cwCnt;

		REPORT_INFO(qmgr, __FUNCTION__, msg.str());
		mCmdQueue.at(address).time = delay;
		mCmdQueue.at(address).nextAddr = -1;
		mSlotNum = (uint16_t)slotNum;
		mPcmdQueueAddress = address >> mAddrShifter;
	}

	/** fetch command in  CommandQueueManager
	* @param address address
	* @param value cmd
	* @param delay time
	* @return void
	**/
	void CommandQueueManager::fetchCommand(const int32_t& address, uint64_t& value, sc_core::sc_time& delay)
	{
		value = mCmdQueue.at(address).cmd;
		delay = mCmdQueue.at(address).time;
	}

	/** update next address in  CommandQueueManager
	* @param address address
	* @param value value
	* @return void
	**/
	void CommandQueueManager::updateNextAddress(const int32_t& address, int32_t& value)
	{

		mCmdQueue.at(address).nextAddr = value;

	}

	/** update next address in  CommandQueueManager
	* @param address address
	* @param value value
	* @return void
	**/
	void CommandQueueManager::fetchNextAddress(const int32_t address, int32_t& value)
	{

		value = mCmdQueue.at(address).nextAddr;
	}

	/** get CW count in  CommandQueueManager
	* @param cwCount code word Count
	* @param address address
	* @return void
	**/
	void CommandQueueManager::getCwCount(const uint16_t& address, uint16_t& cwCount)
	{
		uint32_t addr = address << mAddrShifter;
		uint64_t value = mCmdQueue.at(addr).cmd;
		cwCount = value & 0x1ff;
	}
	/** replicate command in  CommandQueueManage
	* @param slotNum slot index
	* @param cwCount code word Count
	* @return void
	**/
	void CommandQueueManager::replicateCommand(const uint16_t& slotNum, const uint16_t& cwCount)
	{
		uint16_t cwMaxCnt = cwCount;
		uint32_t address = slotNum << mAddrShifter;

		uint64_t value;
		value = mCmdQueue.at(address).cmd;
		sc_core::sc_time delay = mCmdQueue.at(address).time;

		uint64_t logicalBlockAddr = (value >> 9) & getLBAMask();
		for (uint32_t lbaIndex = 9; lbaIndex < (mLBAMaskBits + 9); lbaIndex++)
		{
			value &= ~(1 << lbaIndex);
		}

		for (uint16_t cwIndex = 0; cwIndex < (cwCount - 1); cwIndex++)
		{
			logicalBlockAddr++;
			address++;

			mCmdQueue.at(address).cmd = value | ((logicalBlockAddr & getLBAMask()) << 9);/*(value ^ (((logicalBlockAddr & getLBAMask()) << 9 ) | cwCount ))*/;
			mCmdQueue.at(address).time = delay;
			mCmdQueue.at(address).nextAddr = -1;

		}

	}
	/** get slot Number in  CommandQueueManage
	* @param address value
	* @return void
	**/
	void CommandQueueManager::getSlotNum(uint16_t& address)
	{
		address = mSlotNum;
	}

	/** get LBA field in  CommandQueueManage
	* @param address address
	* @param chanNum channel Number
	* @param cwbank cwbank
	* @param chipselect chip select
	* @param pageNum page number
	* @return void
	**/
	void CommandQueueManager::getLBAField(const int32_t& address, \
		uint8_t& chanNum, uint16_t& cwBank, uint8_t& chipSelect, uint32_t& pageNum)
	{

		uint64_t value = mCmdQueue.at(address).cmd;
		uint64_t lba = (value >> 9) & getLBAMask();
		chanNum = (uint8_t)(lba & getChanMask());
		cwBank = (uint16_t)((lba >> mChanMaskBits) & mBankMask);
		chipSelect = (lba >> (mChanMaskBits + mCwBankMaskBits)) & 0x1;
		pageNum = (uint32_t)((lba >> (mChanMaskBits + 1 + mCwBankMaskBits)) & getPageMask());
	}

	/** get PCMDQ address in  CommandQueueManage
	* @param address address
	* @return void
	**/
	void CommandQueueManager::getPCMDQAddress(int32_t& address)
	{
		address = mSlotNum << mAddrShifter;

	}

	/** get cmd type in  CommandQueueManage
	* @param slotNum slot index
	* @return cmdtype
	**/
	cmdType  CommandQueueManager::getCmdType(const uint16_t& slotNum)
	{
		uint32_t addr = slotNum << mAddrShifter;
		uint64_t cmd = mCmdQueue.at(addr).cmd;
		uint8_t ctype = (cmd >> (mLBAMaskBits + 9)) & 0x3;
		return (cmdType)ctype;
	}

	/** decode command in  CommandQueueManage
	* @param cmd command
	* @param slotNum slot index
	* @param ctype command type
	* @param lba lba
	* @param cwCnt cW count
	* @return void
	**/
	void CommandQueueManager::decodeCommand(const uint64_t& cmd, uint16_t& slotNum, \
		cmdType& ctype, uint64_t& lba, uint16_t& cwCnt)
	{
		slotNum = (uint16_t)((cmd >> ((mLBAMaskBits + 11) & 0xff)));
		ctype = (cmdType)((cmd >> (mLBAMaskBits + 9)) & 0x3);
		lba = (uint64_t)((cmd >> 9) & getLBAMask());
		cwCnt = (uint16_t)(cmd & 0x1ff);
	}

	/** Get bank  Mask
	* Generate bank Masks bits based on cwbank mask
	* @return uint32_t
	**/
	uint32_t CommandQueueManager::getBankMask()
	{
		uint32_t bankMask = 0;

		for (uint16_t bitIndex = 0; bitIndex < mCwBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** Get LBA Mask
	* Generate LBA Masks bits
	* @return uint64_t
	**/
	uint64_t CommandQueueManager::getLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;
	}

	/** Get channel Mask
	* Generate channel Masks bits
	* @return uint32_t
	**/
	uint32_t CommandQueueManager::getChanMask()
	{
		uint32_t chanMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mChanMaskBits; bitIndex++)
			chanMask |= ((uint64_t)0x1 << bitIndex);
		return chanMask;
	}

	/** Get page Mask
	* Generate page Masks bits
	* @return uint64_t
	**/
	uint64_t CommandQueueManager::getPageMask()
	{
		uint64_t pageMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mPageMaskBits; bitIndex++)
			pageMask |= ((uint64_t)0x1 << bitIndex);
		return pageMask;
	}

	/** Get Queue address
	* @param slotNum slot index
	* @param addr address
	* @return void
	**/
	void CommandQueueManager::getQueueAddress(const uint16_t& slotNum, uint32_t& addr)
	{
		addr = slotNum << mAddrShifter;
	}
}