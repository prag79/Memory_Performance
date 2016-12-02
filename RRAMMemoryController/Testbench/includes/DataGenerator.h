/*******************************************************************
 * File : DataGenerator.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of DATA Generation logic used in TB.
   ********************************************************************/
 
#ifndef __DATA_GENERATOR_H__
#define __DATA_GENERATOR_H__
 
#include <iostream>
#include <fstream>
#include <cstdint>
#include <time.h>

class DataGenerator{

public:
	DataGenerator(uint32_t cwSize, uint32_t blockSize)
		: mCwSize(cwSize)
		, mBlockSize(blockSize)
	{
		mDataFile.open("mDataFile.dat", std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
		mDataFile.seekp(0, mDataFile.beg);
		mDataPtr = new uint8_t[blockSize];
	}

	~DataGenerator()
	{
		delete [] mDataPtr;
	}
	// generate random data
	void generateData(const uint64_t lba, uint8_t *dataPtr, const uint32_t& size);
 
	// get data
	void getData(uint8_t* dataPtr, const uint64_t& lbaIndex, const uint32_t& size);
	

private:
	std::fstream mDataFile;
	uint32_t mBlockSize;
	uint32_t mCwSize;
	uint8_t* mDataPtr;
};

/** getData will read data from file having 
* index as starting LBA for complete rwCommand size
* @param uint8_t* dataPtr 
* @param lba starting LBA
* @param rwCommandSize command size
* @return void
**/
void DataGenerator::getData(uint8_t *dataPtr, const uint64_t& lbaIndex, const uint32_t& size)
{
	if (!mDataFile.is_open()){
		cout << "FileManager:readData: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint64_t fileIndex = lbaIndex * size;
	mDataFile.seekg(fileIndex, mDataFile.beg);
	
	mDataFile.read((char*)dataPtr, size);
	
}

/** Generate Data and write it into the file having
* index as (starting LBA + dataIndex) for complete rwCommand size
* dataInex - corresponds to CW
* @param lba starting LBA
* @param cwCount count correponding to each R/W command size
* @return void
**/
void DataGenerator::generateData(const uint64_t lba, uint8_t *dataPtr, const uint32_t& size)
{
	for (unsigned int i = 0; i < size; i++){
		dataPtr[i] = rand() % 0xff;
	}
	if (!mDataFile.is_open()){
		cout << "FileManager:writeData: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
		uint64_t startFileIndex = (lba  * size);
		mDataFile.seekp(startFileIndex, mDataFile.beg);
		mDataFile.write((char*)dataPtr, size);
}

#endif