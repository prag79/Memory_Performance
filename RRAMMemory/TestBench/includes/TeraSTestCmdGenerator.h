#ifndef TESTS_TERAS_COMMAND_GEN_H_
#define TESTS_TERAS_COMMAND_GEN_H_

#include "tlm.h"
#include <queue>
#include <vector>
#include<cstdint>
#include<cmath>
#include "common.h"

class TestCommandGenerator 
{ 

	public:

		TestCommandGenerator( uint32_t cwSize, uint32_t pageSize, uint32_t bankNum)
			:mPageSize(pageSize)
			, mECZ(cwSize)
			, mBankNum(bankNum)
			, mBankMaskBits((uint32_t)log2(double(bankNum)))
			, mPageMaskBits((uint32_t)log2(double(pageSize)))
		{}
		
		uint64_t generateAddress(uint32_t cmdNum);
	private:
		uint32_t  mPageSize, mECZ, mBankNum;
		uint32_t mPageMaskBits;
		uint32_t mBankMaskBits;
		inline uint8_t getBankAddress(uint32_t cmdNum);
		inline uint32_t getPageAddress(uint32_t cmdNum);
		inline uint64_t getAddress(const uint64_t& page, const uint8_t bank);
		inline uint32_t getBankMask();
};

/** Generate device address
* @param cmdNum command number
* @return uint64_t
**/

uint64_t TestCommandGenerator::generateAddress(uint32_t cmdNum){
	uint64_t page = 0;
	uint8_t bank = 0;
	uint64_t address = 0;
	
	try {
		bank = getBankAddress(cmdNum);
	}
	catch (std::overflow_error e)
	{ 
		std::cerr <<"getBankAddress: "<< e.what() << std::endl;
	}
	
	try {
		page = getPageAddress(cmdNum);
	}
	catch (std::overflow_error e)
	{
		std::cerr << "getPageAddress: " << e.what() << std::endl;
	}

	return getAddress(page, bank);
}

/** Get Bank address
* @param cmdNum command number
* @return uint8_t
**/
uint8_t TestCommandGenerator::getBankAddress(uint32_t cmdNum)
{
	if (mECZ < (mBankNum * mPageSize)){

		if (mECZ == 0)
			throw std::overflow_error("Divide by zero exception");

		return (uint8_t)(cmdNum % ((mBankNum * mPageSize) / mECZ));
	}
	else{
		return 0;
	}
}

/** Get Page address
* @param cmdNum command number
* @return uint32_t
**/
uint32_t TestCommandGenerator::getPageAddress(uint32_t cmdNum)
{
	if (mPageSize == 0)
		throw std::overflow_error("Divide by zero exception");
	return (uint32_t)((cmdNum * mECZ) / (mPageSize * mBankNum));
}

/** Get  address
* @param uint64_t page 
* @param uint8_t bank
* @return uint64_t
**/
uint64_t TestCommandGenerator::getAddress(const uint64_t& page, const uint8_t bank)
{
	 return ((uint64_t)((((uint8_t)((bank << (uint32_t)(log2(mECZ / mPageSize))) & getBankMask())) | \
		((uint64_t)((page << ((uint32_t)(log2(mPageSize)) + mBankMaskBits)) & PAGE_MASK_BITS)))\
		& MAX_ADDRESS_RANGE));
}

/** Get Bank Mask
* Generate Masks bits based on number of bits
* @return uint32_t
**/
uint32_t TestCommandGenerator::getBankMask()
{
	uint32_t bankMask = 0;

	for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
		bankMask |= (0x1 << bitIndex);
	return bankMask;

}
#endif

