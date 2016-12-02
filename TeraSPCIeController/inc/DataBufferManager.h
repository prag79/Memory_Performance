/*******************************************************************
 * File : DataBufferManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Command Parameter Table used in
 * TeraSPCIeController architecture to store command parameters. It is indexed
 * by free iTags.
 * Related Specifications: 
 * PCIeControllerSpec.doc
 ********************************************************************/

#ifndef __DATA_BUFFER_MANAGER_H
#define __DATA_BUFFER_MANAGER_H
#include <cstdint>
#include <vector>
#include <map>
#include "common.h"

namespace CrossbarTeraSLib {
	class DataBufferManager
	{
	public:
		DataBufferManager(uint32_t cwSize, uint32_t size)
			:mCwSize(cwSize)
			, mSize(size)
		{
			for (uint32_t qIndex = 0; qIndex < size; qIndex++)
			{
				//mDataBuffPtr.push_back(new uint8_t[cwSize]);
				mDataBuffStatus.insert(std::pair<uint32_t, BufferStatus>(qIndex, BufferStatus()));
				mDataBuffPtr.push_back(new uint8_t[cwSize]);
				memset(mDataBuffPtr.at(qIndex), 0x00, cwSize);
			}
			
		}

		~DataBufferManager()
		{
			for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
			{
				delete[] mDataBuffPtr.at(qIndex);
			}
		}
		void readData(uint8_t& qAddr, uint8_t* data);
		void writeData(uint8_t& qAddr, uint8_t* data);
		bool getFreeBuffPtr(uint8_t& qAddr);
		bool setBuffBusy(uint8_t& qAddr);
		bool resetBufferStatus(uint8_t& qAddr);
		void updateNextAddr(uint8_t& qAddr, uint32_t& nextAddr);
		void getNextAddr(uint8_t& qAddr, uint32_t& nextAddr);
		uint8_t* acquire();
		void release(uint8_t* data);
				
	private:
		std::vector<uint8_t*> mDataBuffPtr;
		std::map<uint32_t, BufferStatus> mDataBuffStatus;
		uint32_t mCwSize;
		uint32_t mSize;
	};

}
#endif