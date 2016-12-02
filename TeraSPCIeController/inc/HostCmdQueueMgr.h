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
#ifndef HOST_CMD_QUEUE_MANAGER_H_
#define HOST_CMD_QUEUE_MANAGER_H_
#include "Common.h"
#include <map>
#include <vector>
#include <cmath>
#include "systemc.h"

namespace CrossbarTeraSLib {
	class HostCmdQueueMgr : public sc_core::sc_object
	{

	public:
		HostCmdQueueMgr(uint32_t size)
			: mSize(size)
			, mHCmdQSize(0)
		{
			//mHostCmdQueue.resize(mSize, new HCmdQueueData());
			for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
			{
				mHCmdQStatus.insert(std::pair<uint32_t, status>(qIndex, status::FREE));
				mHostCmdQueue.push_back(new HCmdQueueData());
			}

		}

		~HostCmdQueueMgr()
		{
			for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
			{
				delete mHostCmdQueue.at(qIndex);
			}
		}
		void push(HCmdQueueData* data, uint32_t& qAddr);
		void pop(uint32_t& qAddr);
		HCmdQueueData* acquire();
		void release(HCmdQueueData* ptr);
		uint32_t getSize();
		uint32_t getQSpaceLeft();
		bool isFull();
		bool isEmpty();
		bool getFreeQueueAddr(uint32_t& qAddr);
		void setQueueBusy(uint32_t& qAddr);
		uint64_t getHostAddress(uint32_t& qAddr);

		/** Sets the address in the queue free
		* it can be re-used
		* @param qAddr queue address
		* @return void
		**/
		void setQueueFree(uint32_t& qAddr);

	private:
		uint32_t mSize;
		uint32_t mHCmdQSize;
		std::vector<HCmdQueueData*> mHostCmdQueue;
		std::map<uint32_t, status> mHCmdQStatus;
	};
}
#endif