#ifndef TLP_MANAGER_H_
#define TLP_MANAGER_H_
#include "common.h"
#include <vector>

namespace CrossbarTeraSLib {
	class TLPManager : public sc_core::sc_object
	{
	public:
		TLPManager(uint32_t size, uint32_t cwSize)
			: mSize(size)
			, mCwSize(cwSize)
			, mMemRdCmdQIndex(0)
			, mMemRdDataQIndex(0)
			, mMemWrDataQIndex(0)
			, mMemWrCmpQIndex(0)
			, mMemRdCmdQHead(-1)
			, mMemRdCmdQTail(-1)
			, mMemRdDataQHead(-1)
			, mMemRdDataQTail(-1)
			, mMemWrCmpQHead(-1)
			, mMemWrCmpQTail(-1)
			, mMemWrDataQHead(-1)
			, mMemWrDataQTail(-1)
		{
			for (uint32_t queueIndex = 0; queueIndex < mSize; queueIndex++)
			{
				mMemRdCmdQueue.push_back(new uint8_t[MEM_RD_CMD_WIDTH]);
				mMemRdDataQueue.push_back(new uint8_t[MEM_RD_DATA_WIDTH]);
				mMemWriteDataQueue.push_back(new uint8_t[MEM_WR_DATA_WIDTH]);
				mMemWriteCompletionQueue.push_back(new uint8_t[MEM_WR_COMPLETION_WIDTH]);
			}
		}
		bool pushTLP(TLPQueueType qtype, uint8_t* data);
		bool popTLP(TLPQueueType qtype, uint8_t* data);

		bool isEmpty(TLPQueueType qType);

		bool isFull(TLPQueueType qType);
		uint16_t getSize(TLPQueueType qType);
	private:

		std::vector<uint8_t*> mMemRdCmdQueue;
		std::vector<uint8_t*> mMemRdDataQueue;
		std::vector<uint8_t*> mMemWriteDataQueue;
		std::vector<uint8_t*> mMemWriteCompletionQueue;
		uint32_t mMemRdCmdQIndex;
		uint32_t mMemRdDataQIndex;
		uint32_t mMemWrDataQIndex;
		uint32_t mMemWrCmpQIndex;
		int32_t mMemRdCmdQHead;
		int32_t mMemRdDataQHead;
		int32_t mMemWrDataQHead;
		int32_t mMemWrCmpQHead;
		int32_t mMemRdCmdQTail;
		int32_t mMemRdDataQTail;
		int32_t mMemWrDataQTail;
		int32_t mMemWrCmpQTail;
		uint32_t mSize;
		uint32_t mCwSize;

	};
}
#endif