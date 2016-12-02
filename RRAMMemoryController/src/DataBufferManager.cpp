#include "DataBufferManager.h"

/** write data in data buffer
* @param slotNum slot index
* @param dataPtr dataptr pointing to data
* @param size size of data
* @return void
**/
void DataBufferManager::writeBlockData(const uint16_t slotNum, const uint8_t* dataPtr, const uint32_t size)
{
	uint32_t address = slotNum << ((uint32_t)log2(double(mBlockSize)) - DATA_BUFFER_BIT_SHIFT);
	for (uint32_t dataIndex = 0; dataIndex < (size/DATA_BUFFER_WIDTH); dataIndex++)
	{
		memcpy(mDataBuffer.at(address + dataIndex), dataPtr + (dataIndex * DATA_BUFFER_WIDTH), DATA_BUFFER_WIDTH);
		//std::cout << "DATA BUFFER CONTENT" << std::hex << *mDataBuffer.at(address + dataIndex) << std::endl;
	}
	
}
/** read data in data buffer
* @param slotNum slot index
* @param dataPtr dataptr pointing to data
* @param size size of data
* @return void
**/
void DataBufferManager::readBlockData(const uint16_t slotNum, uint8_t* dataPtr, const uint32_t size)
{
	uint32_t address = slotNum << ((uint32_t)log2(double(mBlockSize)) - DATA_BUFFER_BIT_SHIFT);
	for (uint32_t dataIndex = 0; dataIndex < (size/DATA_BUFFER_WIDTH); dataIndex ++)
	{
		memcpy(dataPtr + (dataIndex * DATA_BUFFER_WIDTH), mDataBuffer.at(address + dataIndex), DATA_BUFFER_WIDTH);
		//std::cout << "DATA BUFFER CONTENT" << std::hex << *mDataBuffer.at(address + dataIndex) << std::endl;
	}
	
}
/** write CWdata in data buffer
* @param cmdQueueNum cmd queue index
* @param dataPtr dataptr pointing to data
* @return void
**/
void DataBufferManager::writeCWData(const uint32_t cmdQueueNum, uint8_t* dataPtr)
{
	uint32_t address = cmdQueueNum << ((uint32_t)log2(double(mCodeWordSize)) - DATA_BUFFER_BIT_SHIFT);
	for (uint32_t dataIndex = 0; dataIndex < (mCodeWordSize/DATA_BUFFER_WIDTH); dataIndex++)
	{
		memcpy(mDataBuffer.at(address + dataIndex), dataPtr + (dataIndex * DATA_BUFFER_WIDTH), DATA_BUFFER_WIDTH);
	}
	
}
/** read CWdata in data buffer
* @param cmdQueueNum cmd queue index
* @param dataPtr dataptr pointing to data
* @return void
**/
void DataBufferManager::readCWData(const uint32_t cmdQueueNum, uint8_t* dataPtr)
{
	uint32_t address = cmdQueueNum << ((uint32_t)log2(double(mCodeWordSize)) - DATA_BUFFER_BIT_SHIFT);
	for (uint32_t dataIndex = 0; dataIndex < (mCodeWordSize/DATA_BUFFER_WIDTH); dataIndex++)
	{
		memcpy(dataPtr + (dataIndex * DATA_BUFFER_WIDTH), mDataBuffer.at(address + dataIndex), DATA_BUFFER_WIDTH);
	}
	
}