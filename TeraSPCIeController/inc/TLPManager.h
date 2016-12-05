#ifndef TLP_MANAGER_H_
#define TLP_MANAGER_H_
#include "pcie_common.h"
#include <queue>
#include <memory>
//#include <utility>

namespace CrossbarTeraSLib {
	class TLPManager : public sc_core::sc_object
	{
	public:
		TLPManager(uint32_t size, uint32_t cwSize)
			: mSize(size)
			, mCwSize(cwSize)
		{

		}
		bool pushTLP(TLPQueueType qtype, uint8_t* data);
		bool popTLP(TLPQueueType qtype, uint8_t* data);

		bool isEmpty(TLPQueueType qType);

		bool isFull(TLPQueueType qType);
		uint16_t getSize(TLPQueueType qType);
	private:

		std::queue<std::unique_ptr<uint8_t*> > mMemRdCmdQueue;
		std::queue<std::unique_ptr<uint8_t*> > mMemRdDataQueue;
		std::queue<std::unique_ptr<uint8_t*> > mMemWriteDataQueue;
		std::queue<std::unique_ptr<uint8_t*> > mMemWriteCompletionQueue;

		uint32_t mSize;
		uint32_t mCwSize;

	};
}
#endif