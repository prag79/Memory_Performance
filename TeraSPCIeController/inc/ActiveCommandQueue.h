/*******************************************************************
 * File : ActiveCommandQueue.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Active Command Queue used in 
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef __ACTIVE_COMMAND_QUEUE_H__
#define __ACTIVE_COMMAND_QUEUE_H__

#include<vector>
#include<cstdint>
#include "pcie_common.h"

namespace CrossbarTeraSLib {
	class ActiveCommandQueue
	{
	public:
		ActiveCommandQueue(uint32_t queueSize)
		{
			mQueueLength = queueSize;
		};

		ActiveCommandQueue()
		{};

		void pushQueue(ActiveCmdQueueData cmdData);
		ActiveCmdQueueData popQueue();
		int64_t peekQueue();
		void freeQueue();

		bool isEmpty();
		bool isFull();
		size_t getSize();


	private:
		uint32_t mQueueLength;
		std::vector<ActiveCmdQueueData> mQueue;
		std::vector<ActiveCmdQueueData>::iterator it;
	};

	class ActiveDMACmdQueue
	{
	public:
		ActiveDMACmdQueue(uint32_t queueSize)
			:mQueueSize(queueSize)
		{

		}

		void pushQueue(ActiveDMACmdQueueData cmdData);
		ActiveDMACmdQueueData popQueue();
		int64_t peekQueue();
		void freeQueue();

		bool isEmpty();
		bool isFull();
		size_t getSize();
	private:
		uint32_t mQueueSize;
		std::vector<ActiveDMACmdQueueData> mQueue;
		std::vector<ActiveDMACmdQueueData>::iterator it;
	};
}
#endif