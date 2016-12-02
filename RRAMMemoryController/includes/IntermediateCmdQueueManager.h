/*******************************************************************
 * File : IntermediateCmdQueueManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Intermediate Queue used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/

#ifndef INTERMEDIATE_QUEUE_MANAGER_H_
#define INTERMEDIATE_QUEUE_MANAGER_H_

#include "dimm_common.h"


class IntermediateCmdQueueManager
{

public:
	IntermediateCmdQueueManager(uint32_t mSlotNum)
	{
		mCmdQueue.resize(mSlotNum, 0);
	}

	uint16_t update(const uint64_t& cmd, const uint32_t& lbaMaskBits);
	//uint16_t getCount();
	uint8_t getSlotNum();
	
private:
	struct CmdQueueField
	{
		uint16_t runningCnt : 9;
		
	};

	typedef struct CmdQueueField cmdField;

	void getSlotAndCwCnt(const uint64_t cmd, uint16_t& slotNum, uint16_t& cwCount, const uint32_t& lbaMaskBits);

	std::vector<uint16_t> mCmdQueue;
	std::vector<uint16_t>::iterator it;
	//uint16_t mRunningCnt;
	uint16_t mSlotNum;
};

#endif