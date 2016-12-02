/*******************************************************************
 * File : CommandGenerator.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Command Generation logic used in 
 * the TB
 ********************************************************************/
 
#ifndef __COMMAND_GENERATOR_H__
#define __COMMAND_GENERATOR_H__
 
#include <iostream>
#include <fstream>
#include <cstdint>
#include <time.h>
#include "TestCommon.h"
 
using namespace std;

class CommandGenerator{
 

  public:
	  // constructor
	  CommandGenerator(uint32_t blockSize, uint32_t cwSize, \
		  uint32_t pageSize, uint32_t bankNum, uint8_t numDie, \
		  uint32_t pageNum, uint8_t chanNum)
		  : mBlockSize(blockSize)
		  , mCwSize(cwSize)
		  , mPageSize(pageSize)
		  , mBankNum(bankNum)
		  , mNumDie(numDie)
		  , mPageNum(pageNum)
		  , mChanNum(chanNum)
		  , mBankMaskBits((uint32_t)log2(double(bankNum)))
		  {
		  mCmdFile.open("CmdFile.dat", std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
		  mLBAIndex = 0;
 		  srand((unsigned int)time(NULL));
		  mBankMask = getBankMask();
	  }
	 
	  // generate valid LBA's and write to file mLBAFile
	  void generateLBA();
	  void generateLBA(uint64_t& lba, uint64_t& lbaIndex);
	 
	  // return valid LBA
	  uint64_t getLBA(const uint64_t lbaIndex);

	  // generate random read/write random cmd type
	  void generateCmdType(const uint32_t& numCmd);
 
	  // return random r/w cmd type
	  testCmdType getCmdType(const uint64_t& cmdIndex);

	  void generateFixChannelLBA(const uint32_t& numCmd);

	  uint64_t getFixChannelLBA(const uint32_t& cmdIndex);

	  uint16_t getBankAddress(uint32_t cmdNum);
 
	  uint32_t getPageAddress(uint32_t cmdNum);

	  uint32_t getBankMask();

	  uint32_t mBankMask;

	  ~CommandGenerator()
	  {
		  mCmdFile.close();
	  }
 private:
	 uint64_t mStartLBA;
	 uint32_t mBlockSize, mCwSize;
	 uint32_t mLBAIndex;
	 uint32_t mPageSize;
	 uint32_t mBankMaskBits;
	 uint16_t  mBankNum;
	 uint8_t  mNumDie;
	 uint32_t mPageNum;
	 uint8_t  mChanNum;

	 std::fstream mLBAFile;
	 std::fstream mCmdFile;
};


/** Generate LBA begins from 
* startLBA coming from top constructor
* and write it into the file
* @param numCmd No. of commands
* @return void
**/
void CommandGenerator:: generateLBA(){
	uint64_t lba = 0;
	
	  
	uint64_t memSize = (uint64_t)((uint64_t)mBankNum * (uint64_t)mNumDie \
		* (uint64_t)(mPageSize)* (uint64_t)(mPageNum)* (uint64_t)(mChanNum));

   for (uint64_t lbaIndex = 0; lbaIndex <  (uint64_t) memSize / mCwSize; lbaIndex++){
	   
	   lba += mCwSize;
	 
   }
}
/** Generate LBA begins from
* startLBA coming from top constructor
* and write it into the file
* @param lba lba
* @param lbaIndex lba index to be read
* @return void
**/
void CommandGenerator::generateLBA(uint64_t& lba, uint64_t& lbaIndex)
{
	uint64_t memSize = (uint64_t)((uint64_t)mBankNum * (uint64_t)mNumDie \
		* (uint64_t)(mPageSize)* (uint64_t)(mPageNum)* (uint64_t)(mChanNum));

	
}

/** Generate LBA with fix channel begins from
* startLBA coming from top constructor
* and write it into the file
* @param numcmd num of commands
* @return void
**/
void CommandGenerator::generateFixChannelLBA(const uint32_t& numCmd){
	uint64_t lba = 0;
	uint8_t bank = 0;
	uint64_t page = 0;
	lba = mStartLBA;
	if (!mLBAFile.is_open()){
		cout << "FileManager:writeData: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	

	for (uint32_t cmdIndex = 1; cmdIndex < numCmd; cmdIndex++){
		try {
			bank = getBankAddress(cmdIndex);
		}
		catch (std::overflow_error e)
		{
			std::cerr << "getBankAddress: " << e.what() << std::endl;
		}

		try {
			page = getPageAddress(cmdIndex);
		}
		catch (std::overflow_error e)
		{
			std::cerr << "getPageAddress: " << e.what() << std::endl;
		}

		lba = (((bank & mBankMask) << 4) | ((page & PAGE_MASK_BITS) << (mBankMaskBits + 5)));
		
	}

}

/** get LBA
* @param lbaIndex lba index to be read
* @return uint64_t
**/
uint64_t CommandGenerator::getLBA(const uint64_t lbaIndex)
{
	
	uint64_t lba = 0;
	
	return lba;
}
/** get LBA with fix channel
* @param cmdIndex cmd index to be read
* @return uint64_t
**/
uint64_t CommandGenerator::getFixChannelLBA(const uint32_t& cmdIndex){
	uint64_t lba =0;
	return lba;
}

/** Generate random command type and
* write to the file
* @param numCmd No. of commands
* @return void
**/
void CommandGenerator:: generateCmdType(const uint32_t& numCmd){
	uint8_t cmdType;

	for(uint64_t cmdIndex = 0; cmdIndex < (uint64_t)numCmd; cmdIndex++){
    cmdType = rand() % 2;
    // write command to File
	if (!mCmdFile.is_open()){
		cout << "FileManager:writeCmd: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}

	mCmdFile.seekp(cmdIndex, mCmdFile.beg);
	mCmdFile.write((char*)&cmdType, sizeof(cmdType));// to be edited later
  }
}

/** Read cmd type from file and return
* for each command
* @param cmdIndex command no.
* @return uint8_t
**/
testCmdType CommandGenerator::getCmdType(const uint64_t& cmdIndex){
	//cmdType cmd;
	// Read from file and return
	if (!mCmdFile.is_open()){
		cout << "FileManager:readCmd: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint8_t cmdVal;
	mCmdFile.seekg(cmdIndex, mCmdFile.beg);
	mCmdFile.read((char*)&cmdVal, sizeof(cmdVal));// to be edited later
	if (cmdVal)
		return (testCmdType)1;
	else
		return (testCmdType)0;
}

/** get bank Address
* @param cmdNum number of cmds
* @return uint8_t
**/
uint16_t CommandGenerator::getBankAddress(uint32_t cmdNum)
{
	if (mCwSize < (mBankNum * mPageSize)){

		if (mCwSize == 0)
			throw std::overflow_error("Divide by zero exception");

		return (uint16_t)(cmdNum % ((mBankNum * mPageSize) / mCwSize));
	}
	else{
		return 0;
	}
}
 
/** get Page Address
* @param cmdNum number of cmds
* @return uint32_t
**/
uint32_t CommandGenerator::getPageAddress(uint32_t cmdNum)
{
	if (mPageSize == 0)
		throw std::overflow_error("Divide by zero exception");
	return (uint32_t)((cmdNum * mCwSize) / (mPageSize * mBankNum));
}

/** get bank mask
* @return uint32_t
**/
uint32_t CommandGenerator::getBankMask()
{
	uint32_t bankMask = 0;

	for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
		bankMask |= (0x1 << bitIndex);
	return bankMask;

}

#endif