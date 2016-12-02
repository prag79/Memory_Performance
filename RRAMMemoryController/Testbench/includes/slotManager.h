/*******************************************************************
 * File : slotManager.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Slot Management used in the 
 * Test Bench.
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/
#ifndef __SLOT_MANAGER_H
#define __SLOT_MANAGER_H

#include <iostream>
#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include "common.h"
#include "TestCommon.h"

class SlotManager{
	
public:
	SlotManager(uint32_t slotNum)
		:mSlotNum(slotNum)
		, mNumSlotReserved(0)
		, mAvailBlockSize(0)
		, mReqBlockSize(0)
	{
		/*if (mReqBlockSize / mAvailBlockSize > 1){
			mNumSlot = mReqBlockSize / mAvailBlockSize;
		}
		else{
			mNumSlot = 1;
		}*/
		for (uint16_t slotStatusIndex = 0; slotStatusIndex < slotNum; slotStatusIndex++)
		{
			slotStatus.push_back(mSlotStatusTable());
			slotTags.insert(std::pair<uint16_t, bool>(slotStatusIndex, false));
			
		}
		queueSlots.resize(slotNum);
		firstSlotTags.resize(slotNum, 0);
	}
	// check for the continuous availability of numSlot, return first slot, make status to busy
	bool getAvailableSlot(uint16_t &availSlot);
	bool getAvailableSlot(uint16_t& availSlot, uint16_t maxSlotNum);

	// insert command into map for corresponding slot index
	void insertCmd(const uint64_t& cmdPayload, const uint16_t& slotNum);
	// return cmd for the given slot
	uint64_t getCmd(const uint16_t slotNo);
	// delete available slot and make status free
	void freeSlot(uint16_t slotNo, uint8_t& numSlotReserved);
	void freeSlot(uint16_t slotNo);

	uint8_t getNumSlotReserved();
	uint8_t getNumSlotRequired(uint32_t& configuredBlockSize, uint32_t& requestedBlockSize, uint32_t& cwSize, uint8_t& remSlot);
	bool addSlotToList(uint16_t& tags, uint16_t& slotNum);
	bool findSlotInList(uint16_t& tags, uint16_t& slotNum, std::list<uint16_t>::iterator& pos);
	bool removeSlotFromList(uint16_t& tags, uint16_t& slotNum);
	uint16_t getListSize(uint16_t& tags);
	//void allocateTags(uint8_t& slotIndex, uint8_t& tags);
	//bool assignTags(uint8_t& slotIndex, uint8_t& tags);
	bool getFreeTags(uint16_t& tags);
	void resetTags(uint16_t tags);
	void addFirstSlotNum(uint16_t& tags, uint16_t& slotNum);
	uint16_t getFirstSlotNum(uint16_t& tags);
	bool getAvailableSlot(uint16_t& availSlot, uint8_t slotCount);

	uint16_t getTag(uint16_t& slotIndex);
private:
	std::vector < mSlotStatusTable> slotStatus;//struct with slotnum,
	std::vector<mSlotStatusTable >::iterator it;
	uint32_t mSlotNum;
	uint32_t mReqBlockSize;
	uint32_t mAvailBlockSize;
	uint32_t mNumSlot;
	uint8_t mNumSlotReserved;

	
	std::vector<std::list<uint16_t> > queueSlots;
	std::map<uint16_t, bool> slotTags;
	std::vector<uint16_t> firstSlotTags;
};


bool SlotManager::getFreeTags(uint16_t& tags)
{
	uint16_t index = 0;
	for (; index < mSlotNum; index++)
	{
		if (slotTags.at(index) == false)
		{
			tags = index;
			slotTags.at(index) = true;
			return true;
		}
	}
	return false;
}

void SlotManager::resetTags(uint16_t tags)
{
	slotTags.at(tags) = false;
}

void SlotManager::addFirstSlotNum(uint16_t& tags, uint16_t& slotNum)
{
	firstSlotTags.at(tags) = slotNum;
}

uint16_t SlotManager::getFirstSlotNum(uint16_t& tags)
{
	return firstSlotTags.at(tags);
}


bool SlotManager::addSlotToList(uint16_t& tags, uint16_t& slotNum)
{
	queueSlots[tags].push_back(slotNum);
	return true;
}


bool SlotManager::findSlotInList(uint16_t& tags, uint16_t& slotNum, std::list<uint16_t>::iterator& pos)
{
	for (pos = queueSlots[tags].begin(); pos != queueSlots[tags].end(); pos++)
	{
		if (*pos == slotNum)
		{
			return true;
		}
	}
	return false;
	
}

bool SlotManager::removeSlotFromList(uint16_t& tags, uint16_t& slotNum)
{
	std::list<uint16_t>::iterator pos;
	for (uint16_t tagIndex = 0; tagIndex < mSlotNum; tagIndex++)
	{

		if (findSlotInList(tagIndex, slotNum, pos))
		{
			queueSlots[tagIndex].erase(pos);
			tags = tagIndex;
			return true;
		}
		
	}
}

uint16_t SlotManager::getTag(uint16_t& slotIndex)
{
	std::list<uint16_t>::iterator pos;
	uint16_t tag;
	for (uint16_t tagIndex = 0; tagIndex < mSlotNum; tagIndex++)
	{

		if (findSlotInList(tagIndex, slotIndex, pos))
		{
			//queueSlots[tagIndex].erase(pos);
			tag = tagIndex;
			return tag;
		}
	}

}
uint16_t SlotManager::getListSize(uint16_t& tags)
{
	return queueSlots[tags].size();
}

/** check for numslot, returns the first slot 
* and return the first slot(in case of one or more slots)
* will make return slots busy
* @param numSlot no. of slots required
* @param availSlot first slot will be updated
* @return bool
**/
bool SlotManager::getAvailableSlot(uint16_t& availSlot)
{
	uint16_t slotIndex =0;
	bool ifslotdone = RESET;
		
	while (slotIndex < mSlotNum)
	{
		if ((slotStatus.at(slotIndex).status == status::FREE))
		{
			availSlot = slotIndex;
			slotStatus.at(slotIndex).status = status::BUSY;
			//slotIndex++;
			
			return true;
		}
		slotIndex++;
	}
	return false;
		
}

/** check for numslot, returns the first slot
* and return the first slot(in case of one or more slots)
* will make return slots busy
* @param numSlot no. of slots required
* @param availSlot first slot will be updated
* @return bool
**/
bool SlotManager::getAvailableSlot(uint16_t& availSlot, uint8_t slotCount)
{
	uint16_t slotIndex = 0;
	bool ifslotdone = RESET;

	while (slotIndex < mSlotNum)
	{
		if ((slotStatus.at(slotIndex).status == status::FREE))
		{
			availSlot = slotIndex;
			slotStatus.at(slotIndex).status = status::BUSY;
			slotStatus.at(slotIndex).setNumSlotReserved(slotCount);
			//slotIndex++;

			return true;
		}
		slotIndex++;
	}
	return false;

}

bool SlotManager::getAvailableSlot(uint16_t& availSlot, uint16_t maxSlotNum)
{
	uint16_t slotIndex = 0;
	bool ifslotdone = RESET;

	while (slotIndex < maxSlotNum)
	{
		if ((slotStatus.at(slotIndex).status == status::FREE))
		{
			availSlot = slotIndex;
			slotStatus.at(slotIndex).status = status::BUSY;
			slotIndex++;

			return true;
		}
		slotIndex++;
	}
	return false;

}

/** Insert command to slot table at slot index
* @param uint8_t* command payload tobe inserted in table
* @return void
**/
void SlotManager::insertCmd(const uint64_t& cmdPayload, const uint16_t& slotNum){
	
	memcpy(&(slotStatus.at(slotNum).payload),&cmdPayload,8*sizeof(uint8_t));
	
}

/** getCmd return command corresponding to the slot index
* @param slotNo slot index
* @return uint8_t*
**/
uint64_t SlotManager::getCmd(const uint16_t slotNo){
	return slotStatus.at(slotNo).payload;
}

/** free slot will add the free slots into slot table
* @param slotNo free slot no.
* @return void
**/
void SlotManager::freeSlot( uint16_t slotNo, uint8_t& numSlotReserved)
{
	numSlotReserved = slotStatus.at(slotNo).getNumSlotReserved();
	for (int i = 0; i < numSlotReserved; i++){
		slotStatus.at(slotNo).reset();
		slotNo++;
	}
}
void SlotManager::freeSlot(uint16_t slotNo)
{
		slotStatus.at(slotNo).reset();
	
}

uint8_t SlotManager::getNumSlotReserved()
{
	return mNumSlotReserved;
}

uint8_t SlotManager::getNumSlotRequired(uint32_t& configuredBlockSize, uint32_t& requestedBlockSize, uint32_t& cwSize, uint8_t& remSlots)
{
	uint8_t numSlotRequested;
	double quot;
	double rem = remainder((double)requestedBlockSize, (double)configuredBlockSize);

	if ((double)(requestedBlockSize / (double)configuredBlockSize) > 1)
	{

		quot = (double)requestedBlockSize / (double)configuredBlockSize;
		numSlotRequested = (uint8_t)floor(quot);
		if ((uint32_t)abs((int)rem) > configuredBlockSize)
			remSlots = abs((int)rem) / configuredBlockSize;
		else
		{
			if (rem == 0)
				remSlots = 0;
			else
				remSlots = 1;
		}
			
	}
	else
	{
		numSlotRequested = 1;
		remSlots = 0;
	}
	//if ((requestedBlockSize / configuredBlockSize) > 1)
	//{
	//	numSlotRequested = requestedBlockSize / configuredBlockSize;
	//	double rem = remainder((double)requestedBlockSize, (double)configuredBlockSize);
	//	
	//	//numSlotRequested += remSlots;
	//}
	//else
	//{
	//	numSlotRequested = 1;
	//}
	return numSlotRequested;
}
#endif