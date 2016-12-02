#include "PCmdQLLManager.h"
namespace CrossbarTeraSLib {
	bool PCmdQueueLLManager::getFreeQueueAddr(uint32_t& qAddr)
	{
		for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
		{
			if (pCmdQStatus.at(qIndex) == status::FREE)
			{
				qAddr = qIndex;
				return true;
			}
			
		}
		return false;
	}

	void PCmdQueueLLManager::setQueueBusy(uint32_t& qAddr)
	{
		pCmdQStatus.at(qAddr) = status::BUSY;
	}

	void PCmdQueueLLManager::resetQueueStatus(uint32_t& qAddr)
	{
		pCmdQStatus.at(qAddr) = status::FREE;
	}
}