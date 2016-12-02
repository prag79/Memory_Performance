/*******************************************************************
 * File : CompletionQueueManager_mcore.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Completion Queue for multi core system
 * used in TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef COMPLETION_QUEUE_MANAGER_MCORE_H_
#define COMPLETION_QUEUE_MANAGER_MCORE_H_
#include "Common.h"


class CompletionQueueManagerMCore
{

public:
	CompletionQueueManagerMCore(uint32_t mSlotNum)
	{
		/*for (uint32_t slotIndex = 0; slotIndex < mSlotNum; slotIndex++)
		{
		mCmdQueue.push_back(0);
		}*/
		mCmdQueue.resize(mSlotNum, false);
	}

	void setStatus(uint32_t slotNum);
	void resetStatus(uint32_t slotNum);
	bool getStatus(uint32_t slotNum);
	//uint16_t getCount();
	//uint8_t getSlotNum();
	//void popQueue(const uint8_t& slotNum);
private:
	//struct CmdQueueField
	//{
	//	uint16_t runningCnt : 9;
	//	//uint8_t slotNum : 8;
	//};

	//typedef struct CmdQueueField cmdField;

	//void getSlotAndCwCnt(const uint32_t cmd, uint8_t& slotNum, uint16_t& cwCount, const uint32_t& lbaMaskBits);


	std::vector<bool> mCmdQueue;
	//std::vector<uint16_t>::iterator it;
	//uint16_t mRunningCnt;
	uint16_t mSlotNum;
};

#endif