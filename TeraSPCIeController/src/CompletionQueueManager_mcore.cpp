#include "CompletionQueueManager_mcore.h"

/** Update IntermediateCmdQueueManager
* @param cmd command
* @param lbaMaskBits Lba mask bits
* @return void
**/
void CompletionQueueManagerMCore::setStatus(uint32_t slotNum)
{
	mCmdQueue.at(slotNum) = true;
}

void CompletionQueueManagerMCore::resetStatus(uint32_t slotNum)
{
	mCmdQueue.at(slotNum) = false;
}
/** count in  IntermediateCmdQueueManager
* @return uint16_t
**/
bool CompletionQueueManagerMCore::getStatus(uint32_t slotNum)
{
	return mCmdQueue.at(slotNum);
}


