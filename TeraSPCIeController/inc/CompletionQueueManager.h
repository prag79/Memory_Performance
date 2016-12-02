/*******************************************************************
 * File : CompletionQueueManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Completion Command Queue used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef COMPLETION_QUEUE_MANAGER_H_
#define COMPLETION_QUEUE_MANAGER_H_
#include "Common.h"
#include <queue>

class CompletionQueueManager
{
public:
	CompletionQueueManager()
		:mQueueCount(0)
	{

	}

	uint32_t getQueueCount();
	void resetQueueCount();
	void pushCommand(const uint32_t& value);
	void popCommand(uint8_t* value);
	void getSlotNum(uint16_t& slotNum);
	void getCmdType(cmdType& cmd);
	bool isEmpty();

private:
	uint32_t mQueueCount;
	std::queue<uint32_t> mCompletionQueue;
};
#endif