#include "IntermediateCmdQueueManager.h"

/** Update IntermediateCmdQueueManager
* @param cmd command
* @param lbaMaskBits Lba mask bits
* @return void
**/
void IntermediateCmdQueueManager::update(const uint32_t& cmd, const uint32_t& lbaMaskBits)
{
	uint16_t slotNum;
	uint16_t cwCount;
	getSlotAndCwCnt(cmd, slotNum, cwCount, lbaMaskBits);

	if (mCmdQueue[slotNum] == 0)
	{
		mCmdQueue[slotNum] = cwCount - 1;
		
	}
	else
	{
		mCmdQueue[slotNum]--;
		
	}
	mRunningCnt = mCmdQueue[slotNum];
	/*for (it = mCmdQueue.begin(); it != mCmdQueue.end(); it++)
	{
		if (it->slotNum == slotNum)
		{
			mSlotNum = slotNum;
			break;
		}
	}
	if (it != mCmdQueue.end())
	{
		if (it->runningCnt)
		{
			it->runningCnt--;
			mRunningCnt = it->runningCnt;
		}
		else
			mRunningCnt = 0;
	}
	else {
		mCmdQueue.push_back({cwCount -1, slotNum });
		mRunningCnt = cwCount - 1;
		mSlotNum = slotNum;
	}*/
}
/** count in  IntermediateCmdQueueManager
* @return uint16_t
**/
uint16_t IntermediateCmdQueueManager::getCount()
{
	return mRunningCnt;
}
/** slot number in  IntermediateCmdQueueManager
* @return uint8_t
**/
uint8_t IntermediateCmdQueueManager::getSlotNum()
{
	return mSlotNum;
}

/** get slot and CW cnt from command 
* @param cmd command
* @param slotNum slot index
* @param cwCount Code word Count
* @param lbaMaskbits lba mask bits
* @return void
**/
void IntermediateCmdQueueManager::getSlotAndCwCnt(const uint32_t cmd, uint16_t& slotNum, uint16_t& cwCount, const uint32_t& lbaMaskBits)
{
	cwCount = cmd & 0x1ff;
	slotNum = (cmd >> (lbaMaskBits + 19)) & 0xff;
}

/** pop command in  IntermediateCmdQueueManager
* @param slotNum slot index
* @return void
**/
//void IntermediateCmdQueueManager::popQueue(const uint8_t& slotNum)
//{
//	for (it = mCmdQueue.begin(); it != mCmdQueue.end(); it++)
//	{
//		if (it->slotNum == slotNum)
//		{
//			mCmdQueue.erase(it);
//			break;
//		}
//	}
//}