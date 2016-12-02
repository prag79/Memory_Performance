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
	void CommandQueueManager::insert(const uint32_t queueNum, const uint8_t cmd, \
		const uint64_t plba, const uint32_t cmdOffset, const uint32_t iTag, \
		const int32_t nextAddr, const sc_core::sc_time delay)
	{
		std::ostringstream msg;
		if (cmd == 1)
		mCmdQueue.at(queueNum).cmd = cmdType::WRITE;
		else if (cmd == 2){
			mCmdQueue.at(queueNum).cmd = cmdType::READ;
		}
		//mCmdQueue.at(queueNum).cmd = cmd;
		mCmdQueue.at(queueNum).plba = plba;
		mCmdQueue.at(queueNum).iTag = iTag;
		mCmdQueue.at(queueNum).cmdOffset = cmdOffset;
		mCmdQueue.at(queueNum).nextAddr = nextAddr;
		mCmdQueue.at(queueNum).time = delay;
		mCmdQSize++;
		/*msg << "CommandQueueManager: "
			<< " @Time: " << dec << delay
			<< " Cmd Type :" << hex << (uint32_t)cmd
			<< " queue Addr: " << dec << (uint32_t)queueNum
			<< " iTag: " << dec << iTag
			<< " plba: " << dec << plba;
		REPORT_INFO(qmgr, __FUNCTION__, msg.str());*/

	}

	/** fetch command in  CommandQueueManager
	* @param address address
	* @param value cmd
	* @param delay time
	* @return void
	**/
	void CommandQueueManager::fetchCommand(const uint32_t queueNum, cmdType& cmd, \
		uint64_t& plba, uint32_t& cmdOffset, uint32_t& iTag, \
		int32_t& nextAddr, sc_core::sc_time& delay)
	{
		cmd = mCmdQueue.at(queueNum).cmd;
		plba = mCmdQueue.at(queueNum).plba;
		cmdOffset = mCmdQueue.at(queueNum).cmdOffset;
		iTag = mCmdQueue.at(queueNum).iTag;
		nextAddr = mCmdQueue.at(queueNum).nextAddr;
		delay = mCmdQueue.at(queueNum).time;
	}

	/** update next address in  CommandQueueManager
	* @param address address
	* @param value value
	* @return void
	**/
	void CommandQueueManager::updateNextAddress(const uint32_t& queueNum, int32_t& value)
	{
		mCmdQueue.at(queueNum).nextAddr = value;
	}

	/** update next address in  CommandQueueManager
	* @param address address
	* @param value value
	* @return void
	**/
	void CommandQueueManager::fetchNextAddress(const uint32_t& queueNum, int32_t& value)
	{
		value = mCmdQueue.at(queueNum).nextAddr;
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
	//void CommandQueueManager::replicateCommand(const uint16_t& slotNum, const uint16_t& cwCount)
	//{
	//	uint16_t cwMaxCnt = cwCount;
	//	uint32_t address = slotNum << mAddrShifter;
	//	
	//	uint64_t value;
	//    value = mCmdQueue.at(address).cmd;
	//	sc_core::sc_time delay = mCmdQueue.at(address).time;
	//	
	//	uint64_t logicalBlockAddr = (value >> 9) & getLBAMask();
	//	for (uint32_t lbaIndex = 9; lbaIndex < (mLBAMaskBits + 9); lbaIndex++)
	//	{
	//		value &= ~(1<<lbaIndex) ;
	//	}
	//	
	//	for (uint16_t cwIndex = 0; cwIndex < (cwCount - 1); cwIndex++)
	//	{  
	//		logicalBlockAddr++; 
	//		address++;
	//		
	//		mCmdQueue.at(address).cmd = value | ((logicalBlockAddr & getLBAMask()) << 9);/*(value ^ (((logicalBlockAddr & getLBAMask()) << 9 ) | cwCount ))*/;
	//		mCmdQueue.at(address).time = delay;
	//		mCmdQueue.at(address).nextAddr = -1;
	//		
	//	}

	//}
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
	void CommandQueueManager::getLBAField(const uint32_t& queueNum, \
		uint8_t& chanNum, uint8_t& cwBank, uint8_t& chipSelect, uint32_t& pageNum)
	{


		uint64_t lba = mCmdQueue.at(queueNum).plba;// (value >> 9) & getLBAMask();
		chanNum = (uint8_t)(lba & getChanMask());
		cwBank = (uint8_t)((lba >> mChanMaskBits) & mBankMask);
			
		
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
	cmdType  CommandQueueManager::getCmdType(const uint32_t& queueNum)
	{
		cmdType cmd = mCmdQueue.at(queueNum).cmd;
		return cmd;
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

		for (uint8_t bitIndex = 0; bitIndex < mCwBankMaskBits; bitIndex++)
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

	bool CommandQueueManager::getFreeQueueAddr(uint32_t& qAddr)
	{
		for (uint32_t qIndex = 0; qIndex < mQueueSize; qIndex++)
		{
			if (mCmdQStatus.at(qIndex) == status::FREE)
			{
				qAddr = qIndex;
				return true;
			}
			
		}
		return false;
	}

	void CommandQueueManager::setQueueBusy(uint32_t& qAddr)
	{
		mCmdQStatus.at(qAddr) = status::BUSY;
	}

	void CommandQueueManager::setQueueFree(uint32_t& qAddr)
	{
		mCmdQStatus.at(qAddr) = status::FREE;
		mCmdQueue.at(qAddr).reset();
		mCmdQSize--;
	}

	void CommandQueueManager::resetQueueStatus(uint32_t& qAddr)
	{
		mCmdQStatus.at(qAddr) = status::FREE;
	}

	void CommandQueueManager::getiTagNumber(uint32_t& qAddr, uint32_t& iTag)
	{
		iTag = mCmdQueue.at(qAddr).iTag;
	}

	void CommandQueueManager::getCmdOffset(uint32_t& qAddr, uint32_t& offset)
	{
		offset = mCmdQueue.at(qAddr).cmdOffset;
	}

	bool CommandQueueManager::isFull()
	{
		if (mCmdQueue.size() == mCmdQSize)
			return true;
		else return false;
	}

	bool CommandQueueManager::isEmpty()
	{
		if (mCmdQSize == 0)
			return true;
		else return false;
	}
}