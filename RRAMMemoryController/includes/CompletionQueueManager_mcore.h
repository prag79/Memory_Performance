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
#include "dimm_common.h"


class CompletionQueueManagerMCore
{

public:
	CompletionQueueManagerMCore(uint32_t mSlotNum)
	{
		
		mCmdQueue.resize(mSlotNum, false);
	}

	void setStatus(uint32_t slotNum);
	void resetStatus(uint32_t slotNum);
	bool getStatus(uint32_t slotNum);
	
private:
	
	std::vector<bool> mCmdQueue;
	
	uint16_t mSlotNum;
};

#endif