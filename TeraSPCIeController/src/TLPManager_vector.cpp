#include "TLPManager.h"
namespace CrossbarTeraSLib {
	bool TLPManager::pushTLP(TLPQueueType qType, uint8_t* data)
	{
		switch (qType)
		{
		case MEM_READ_CMD_Q:
		{
							   //uint8_t* qData = new uint8_t[MEM_RD_CMD_WIDTH];
							   if (mMemRdCmdQTail >= (mMemRdCmdQueue.size() - 1))
								   mMemRdCmdQTail = -1;
							   mMemRdCmdQTail++;
							  
							   memcpy(mMemRdCmdQueue.at(mMemRdCmdQTail), data, MEM_RD_CMD_WIDTH);
							   mMemRdCmdQIndex++;
							   //mMemRdCmdQueue.push(qData);
							   break;
		}

		case MEM_READ_DATA_Q:
		{
								if (mMemRdDataQTail >= (mMemRdCmdQueue.size() - 1))
									mMemRdDataQTail = -1;
								//uint8_t* qData = new uint8_t[MEM_RD_DATA_WIDTH];
								mMemRdDataQTail++;
								memcpy(mMemRdDataQueue.at(mMemRdDataQTail), data, MEM_RD_DATA_WIDTH);
								//mMemRdDataQueue.push(qData);
								mMemRdDataQIndex++;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								 if (mMemWrDataQTail >= (mMemRdCmdQueue.size() - 1))
									 mMemWrDataQTail = -1;
								 //uint8_t* qData = new uint8_t[MEM_WR_DATA_WIDTH];
								 mMemWrDataQTail++;
								 memcpy(mMemWriteDataQueue.at(mMemWrDataQTail), data, MEM_WR_DATA_WIDTH);
								 /*uint8_t* qData = new uint8_t[MEM_WR_DATA_WIDTH + mCwSize];
								 memcpy(qData, data, MEM_WR_DATA_WIDTH + mCwSize);*/
								 //mMemWriteDataQueue.push(qData);
								 mMemWrDataQIndex++;
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 if (mMemWrCmpQTail >= (mMemRdCmdQueue.size() - 1))
									 mMemWrCmpQTail = -1;
								 //uint8_t* qData = new uint8_t[MEM_WR_COMPLETION_WIDTH];
								 mMemWrCmpQTail++;
								 memcpy(mMemWriteCompletionQueue.at(mMemRdCmdQTail), data, MEM_WR_COMPLETION_WIDTH);
								 //mMemWriteCompletionQueue.push(qData);
								 mMemWrCmpQIndex++;
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
							   if (mMemRdCmdQHead >= (mMemRdCmdQueue.size() - 1))
								   mMemRdCmdQHead = -1;
							   mMemRdCmdQHead++;
							   memcpy(data, mMemRdCmdQueue.at(mMemRdCmdQHead), MEM_RD_CMD_WIDTH);
							  // delete [] mMemRdCmdQueue.front();
							   //mMemRdCmdQueue.pop();
							   mMemRdCmdQIndex--;
							   break;
		}
		case MEM_READ_DATA_Q:
		{
								if (mMemRdDataQHead >= (mMemRdCmdQueue.size() - 1))
									mMemRdDataQHead = -1;
								mMemRdDataQHead++;
								memcpy(data, mMemRdDataQueue.at(mMemRdDataQHead), MEM_RD_DATA_WIDTH);
								//delete [] mMemRdDataQueue.front();
								//mMemRdDataQueue.pop();
								mMemRdDataQIndex--;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								 if (mMemWrDataQHead >= (mMemRdCmdQueue.size() - 1))
									 mMemWrDataQHead = -1;
								 mMemWrDataQHead++;
								 memcpy(data, mMemWriteDataQueue.at(mMemWrDataQHead), MEM_WR_DATA_WIDTH);
								 //delete [] mMemWriteDataQueue.front();
								 //mMemWriteDataQueue.pop();
								 mMemWrDataQIndex--;
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 if (mMemWrCmpQHead >= (mMemRdCmdQueue.size() - 1))
									 mMemWrCmpQHead = -1;
								 mMemWrCmpQHead++;
								 memcpy(data, mMemWriteCompletionQueue.at(mMemWrCmpQHead), MEM_WR_COMPLETION_WIDTH);
								 //uint8_t *ptr = mMemWriteCompletionQueue.front();
								 //delete[] ptr;
								 //ptr = nullptr;
								 //mMemWriteCompletionQueue.pop();
								 mMemWrCmpQIndex--;
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
							   if (mMemRdCmdQIndex)
								   isEmpty = false;
							   else
							   {
								  
								   isEmpty = true;
							   }
							   break;
		}
		case MEM_READ_DATA_Q:
		{
								if (mMemRdDataQIndex)
									isEmpty = false;
								else
								    isEmpty = true;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
								  if (mMemWrDataQIndex)
									  isEmpty = false;
								  else
									  isEmpty = true;
								  break;
		}
		case MEM_WRITE_COMP_Q:
		{
								 if (mMemWrCmpQIndex)
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
						  
							   if (mMemRdCmdQueue.size() == (mMemRdCmdQIndex + 1))
							   {
								   mMemRdCmdQHead = -1;
								   mMemRdCmdQTail = -1;
								   isFull = true;
							   }
							   else
								   isFull = false;
							   break;
		}
		case MEM_READ_DATA_Q:
		{
		
								if (mMemRdDataQueue.size() == (mMemRdDataQIndex + 1))
								{
									mMemRdDataQHead = -1;
									mMemRdDataQTail = -1;
									isFull = true;
								}
								else
									isFull = false;
								break;
		}
		case MEM_WRITE_DATA_Q:
		{
					
								 if (mMemWriteDataQueue.size() == (mMemWrDataQIndex + 1))
								 {
									 mMemWrDataQHead = -1;
									 mMemWrDataQTail = -1;
									 isFull = true;
								 }
								 else
									 isFull = false;
								 break;
		}
		case MEM_WRITE_COMP_Q:
		{
		            			 if (mMemWriteCompletionQueue.size() == mMemWrCmpQIndex)
								 {
									 mMemWrCmpQHead = -1;
									 mMemWrCmpQTail = -1;
									 isFull = true;
								 }
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
			size = mMemRdCmdQIndex;
			break;
		case MEM_READ_DATA_Q:
			size = mMemRdDataQIndex;
			break;
		case MEM_WRITE_DATA_Q:
			size = mMemWrDataQIndex;
			break;
		case MEM_WRITE_COMP_Q:
			size = mMemWrCmpQIndex;
			break;
		}
		return size;
	}
}