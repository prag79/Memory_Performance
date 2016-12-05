/*******************************************************************
 * File : FileManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of File IO task used to read and write 
 * memory data from/to the file.
 * Related Specifications: 
 * TeraS_Device_Specification_ver1.1.doc
 ********************************************************************/
#ifndef MODELS_CACHE_MANAGER_H_
#define MODELS_CACHE_MANAGER_H_
#include "dimm_common.h"
#include <fstream>
#include <string>

class FileManager
{
public:
	FileManager(uint32_t size, uint32_t pageSize, uint8_t bankNum,\
		uint8_t chanNum, uint8_t numDie, uint32_t pageNum, uint32_t blockSize)
		:mSize(size)
		, mPageSize(pageSize)
		, mBankNum(bankNum)
		, mChanNum(chanNum)
		, mNumDie(numDie)
		, mPageNum(pageNum)
		, mBlockSize(blockSize)
	{
		
		mMemFile.open("MemoryFile.dat", std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
	}

	void readMemory(const uint8_t& bankNum, const uint32_t& pageAddress, \
		uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum);
	void writeMemory(const uint8_t& bankNum, const uint32_t& pageAddress, \
		uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum);
	void initFile();
	void closeFile();
	
	~FileManager()
	{
		
	}

private:
	uint32_t mSize;
	uint32_t mPageSize;
	uint8_t mBankNum;
	uint8_t mChanNum;
	uint8_t mNumDie;
	uint32_t mPageNum;
	uint32_t mBlockSize;
	std::fstream mMemFile;
	
};
#endif