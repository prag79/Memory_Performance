#include "CompletionQueueManager.h"

/**get queue count of completion queue
* @return uint32_t
**/
uint32_t CompletionQueueManager::getQueueCount()
{
	mQueueCount = mCompletionQueue.size();
	
	return mQueueCount;
}

/**reset of completion queue
* @return void
**/
void CompletionQueueManager::resetQueueCount()
{
	mQueueCount = 0;
}

/**push a command in  completion queue
* @param value value to be inserted
* @return void
**/
void CompletionQueueManager::pushCommand(const uint16_t& value)
{
	mCompletionQueue.push(value);
}

/**pop a command in  completion queue
* @param value value to be popped
* @return void
**/
void CompletionQueueManager::popCommand(uint8_t* value)
{
	if (!mCompletionQueue.empty())
	{
		uint16_t tempVal = mCompletionQueue.front();

		memcpy(value, (uint8_t*)&tempVal, 2);
		mCompletionQueue.pop();
	}
}
/**get a slot index in  completion queue
* @param slotNum slot index
* @return void
**/
void CompletionQueueManager::getSlotNum(uint16_t& slotNum)
{
	if (!mCompletionQueue.empty())
	{
		uint16_t tempVal = mCompletionQueue.front();
		slotNum = (uint16_t)(tempVal >> 7) & 0xff;
		
	}
}

/**get command type in  completion queue
* @param cmd calulated cmd type 
* @return void
**/
void CompletionQueueManager::getCmdType(cmdType& cmd)
{
	if (!mCompletionQueue.empty())
	{
		uint16_t tempVal = mCompletionQueue.front();
		uint8_t cmdtype = (tempVal >> 15) & 0x1;
		if (cmdtype == 0)
		{
			cmd = cmdType::READ;
		}
		else {
			cmd = cmdType::WRITE;
		}
	}
}

/**check if completion queue is empty
* @return bool
**/
bool CompletionQueueManager::isEmpty()
{
	if (mCompletionQueue.empty())
		return true;
	else
		return false;
}