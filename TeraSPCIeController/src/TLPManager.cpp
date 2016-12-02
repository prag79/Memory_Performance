#include "TLPManager.h"
namespace CrossbarTeraSLib {
	bool TLPManager::pushTLP(TLPQueueType qType, uint8_t* data)
	{
		switch (qType)
		{
		case MEM_READ_CMD_Q:
		{
							   uint8_t* qData = new uint8_t[MEM_RD_CMD_WIDTH];
							   memcpy(qData, data, MEM_RD_CMD_WIDTH);
							   mMemRdCmdQueue.push(std::make_unique<uint8_t*>(qData));
							   break;
		}

		case MEM_READ_DATA_Q:
		{
								uint8_t* qData = new uint8_t[MEM_RD_DATA_WIDTH];
								memcpy(qData, data, MEM_RD_DATA_WIDTH);
								mMemRdDataQueue.push(std::make_unique<uint8_t*>(qData));
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								 uint8_t* qData = new uint8_t[MEM_WR_DATA_WIDTH];
								 memcpy(qData, data, MEM_WR_DATA_WIDTH);
								 /*uint8_t* qData = new uint8_t[MEM_WR_DATA_WIDTH + mCwSize];
								 memcpy(qData, data, MEM_WR_DATA_WIDTH + mCwSize);*/
								 mMemWriteDataQueue.push(std::make_unique<uint8_t*>(qData));
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 uint8_t* qData = new uint8_t[MEM_WR_COMPLETION_WIDTH];
								 memcpy(qData, data, MEM_WR_COMPLETION_WIDTH);
								 mMemWriteCompletionQueue.push(std::make_unique<uint8_t*>(qData));
								 break;
		}
		default:
			return false;
		}
		return true;
	}

	bool TLPManager::popTLP(TLPQueueType qType, uint8_t* data)
	{
		switch (qType)
		{
		case MEM_READ_CMD_Q:
		{
							   memcpy(data, *mMemRdCmdQueue.front().get(), MEM_RD_CMD_WIDTH);
							   mMemRdCmdQueue.pop();
							  // delete[] mMemRdCmdQueue.front();
							   break;
		}
		case MEM_READ_DATA_Q:
		{
								memcpy(data, *mMemRdDataQueue.front().get(), MEM_RD_DATA_WIDTH);
								mMemRdDataQueue.pop();
								//delete[] mMemRdDataQueue.front();
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								 memcpy(data, *mMemWriteDataQueue.front().get(), MEM_WR_DATA_WIDTH);
								 //memcpy(data, mMemWriteDataQueue.front(), MEM_WR_DATA_WIDTH + mCwSize);
								 mMemWriteDataQueue.pop();
								 //delete[] mMemWriteDataQueue.front();
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 memcpy(data, *mMemWriteCompletionQueue.front().get(), MEM_WR_COMPLETION_WIDTH);
								 mMemWriteCompletionQueue.pop();
								 //delete[] mMemWriteCompletionQueue.front();
								 break;
		}
		}
		return true;
	}

	bool TLPManager::isEmpty(TLPQueueType qType)
	{
		bool isEmpty;
		switch (qType)
		{
		case MEM_READ_CMD_Q:
		{
							   if (mMemRdCmdQueue.size())
								   isEmpty = false;
							   else
								   isEmpty = true;
							   break;
		}
		case MEM_READ_DATA_Q:
		{
								if (mMemRdDataQueue.size())
									isEmpty = false;
								else
									isEmpty = true;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								  if (mMemWriteDataQueue.size())
									  isEmpty = false;
								  else
									  isEmpty = true;
								  break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 if (mMemWriteCompletionQueue.size())
									 isEmpty = false;
								 else
									 isEmpty = true;
								 break;
		}
		}
		return isEmpty;

	}

	bool TLPManager::isFull(TLPQueueType qType)
	{
		bool isFull;
		switch (qType)
		{

		case MEM_READ_CMD_Q:
		{
							   if (mMemRdCmdQueue.size() == mSize)
								   isFull = true;
							   else
								   isFull = false;
							   break;
		}
		case MEM_READ_DATA_Q:
		{
								if (mMemRdDataQueue.size() == mSize)
									isFull = true;
								else
									isFull = false;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								 if (mMemWriteDataQueue.size() == mSize)
									 isFull = true;
								 else
									 isFull = false;
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 if (mMemWriteCompletionQueue.size() == mSize)
									 isFull = true;
								 else
									 isFull = false;
								 break;
		}
		}
		return isFull;
	}

	uint16_t TLPManager::getSize(TLPQueueType qType)
	{
		uint16_t size;
		switch (qType)
		{
		case MEM_READ_CMD_Q:
			size = mMemRdCmdQueue.size();
			break;
		case MEM_READ_DATA_Q:
			size = mMemRdDataQueue.size();
			break;
		case MEM_WRITE_DATA_Q:
			size = mMemWriteDataQueue.size();
			break;
		case MEM_WRITE_COMP_Q:
			size = mMemWriteCompletionQueue.size();
			break;
		}
		return size;
	}
}