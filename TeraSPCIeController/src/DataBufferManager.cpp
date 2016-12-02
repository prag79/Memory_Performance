#include "DataBufferManager.h"
namespace CrossbarTeraSLib {
	void DataBufferManager::readData(uint8_t& qAddr, uint8_t* data)
	{
		memcpy(data, mDataBuffPtr.at(qAddr), mCwSize);
	}

	void DataBufferManager::writeData(uint8_t& qAddr, uint8_t* data)
	{
		memcpy(mDataBuffPtr.at(qAddr), data, mCwSize);
	}

	bool DataBufferManager::getFreeBuffPtr(uint8_t& qAddr)
	{
		for (uint32_t qIndex = 0; qIndex < mSize; qIndex++)
		{
			if (mDataBuffStatus.at(qIndex).buffStatus == FREE)
			{
				qAddr = qIndex;
				return true;
			}
		}
		return false;
	}

	bool DataBufferManager::setBuffBusy(uint8_t& qAddr)
	{
		/*if (mDataBuffStatus.size() < mSize)
		{*/
			mDataBuffStatus.at(qAddr).buffStatus = BUSY;
			return true;
		//}
		return false;
	}

	bool DataBufferManager::resetBufferStatus(uint8_t& qAddr)
	{
		/*if (mDataBuffStatus.size() < mSize)
		{*/
			mDataBuffStatus.at(qAddr).buffStatus = FREE;
			return true;
		//}
		return false;
	}

	void DataBufferManager::getNextAddr(uint8_t& qAddr, uint32_t& nextAddr)
	{
		nextAddr = mDataBuffStatus.at(qAddr).nextAddr;
	}

	void DataBufferManager::updateNextAddr(uint8_t& qAddr, uint32_t& nextAddr)
	{
		mDataBuffStatus.at(qAddr).nextAddr = nextAddr;
	}

	uint8_t* DataBufferManager::acquire()
	{
		uint8_t* data = new uint8_t[mCwSize];
		return data;
	}

	void DataBufferManager::release(uint8_t* data)
	{
		delete data;
	}

	
}