/*******************************************************************
 * File : DataBufferManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Data Buffer used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef DATA_BUFFER_MANAGER_H_
#define DATA_BUFFER_MANAGER_H_

#include "dimm_common.h"
#include <vector>
#include <cmath>

class DataBufferManager
{
public:
	DataBufferManager(uint32_t blockSize, uint32_t codeWordSize, uint32_t cmdQueueSize)
		:mCodeWordSize(codeWordSize)
		, mBlockSize(blockSize)
	{
		mDataBufferSize = (uint32_t)((codeWordSize * cmdQueueSize)/ DATA_BUFFER_WIDTH);
		for (uint64_t bufferIndex = 0; bufferIndex < mDataBufferSize; bufferIndex++)
		{
			mDataBuffer.push_back(new uint8_t[DATA_BUFFER_WIDTH]);
		}
		
	}

	~DataBufferManager()
	{
		for (uint32_t bufferIndex = 0; bufferIndex < mDataBufferSize; bufferIndex++)
		{
			delete mDataBuffer.at(bufferIndex);
		}
	}
	void writeBlockData(const uint16_t slotNum, const uint8_t* dataPtr, const uint32_t size);
	void readBlockData(const uint16_t slotNum, uint8_t* dataPtr, const uint32_t size);
	void writeCWData(const uint32_t cmdQueueNum, uint8_t* dataPtr);
	void readCWData(const uint32_t cmdQueueNum, uint8_t* dataPtr);
private:
	std::vector<uint8_t*> mDataBuffer;
	uint32_t mDataBufferSize;
	uint32_t mCodeWordSize;
	uint32_t mBlockSize;
};

#endif