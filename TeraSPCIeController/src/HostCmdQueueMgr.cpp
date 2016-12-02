#include "HostCmdQueueMgr.h"

namespace CrossbarTeraSLib {
	void HostCmdQueueMgr::pop(uint32_t& qAddr)
	{
		//HCmdQueueData* data = mHostCmdQueue.at(qAddr);
		//data = nullptr;
		mHostCmdQueue.at(qAddr)->reset();
		mHCmdQSize--;
		//release(data);
	}

	void HostCmdQueueMgr::push(HCmdQueueData* data, uint32_t& qAddr)
	{
		memcpy(mHostCmdQueue.at(qAddr), data, sizeof(HCmdQueueData)) ;
		mHCmdQSize++;
	}

	uint32_t HostCmdQueueMgr::getSize()
	{
		return mHCmdQSize;
	}

	uint32_t HostCmdQueueMgr::getQSpaceLeft()
	{
		return (mHostCmdQueue.size() - mHCmdQSize);
	}

	bool HostCmdQueueMgr::isFull()
	{
		if (mHostCmdQueue.size() == mHCmdQSize)
			return true;
		else
			return false;
	}

	bool HostCmdQueueMgr::isEmpty()
	{
		if (mHCmdQSize)
			return false;
		else
			return true;
	}

	HCmdQueueData* HostCmdQueueMgr::acquire()
	{
		HCmdQueueData* hEntry = new HCmdQueueData();
		return hEntry;
	}

	void HostCmdQueueMgr::release(HCmdQueueData* ptr)
	{
		delete ptr;
	}

	bool HostCmdQueueMgr::getFreeQueueAddr(uint32_t& qAddr)
	{
		for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
		{
			if (mHCmdQStatus.at(qIndex) == status::FREE)
			{
				qAddr = qIndex;
				return true;
			}
			
		}
		return false;
	}

	void HostCmdQueueMgr::setQueueBusy(uint32_t& qAddr)
	{
		mHCmdQStatus.at(qAddr) = status::BUSY;
	}

	void HostCmdQueueMgr::setQueueFree(uint32_t& qAddr)
	{
		mHCmdQStatus.at(qAddr) = status::FREE;
	}

	uint64_t HostCmdQueueMgr::getHostAddress(uint32_t& qAddr)
	{
		return mHostCmdQueue.at(qAddr)->hAddr0;
	}
}