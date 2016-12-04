#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H
#include<cstdint>

namespace CrossbarTeraSLib {
	class MemMgr
	{
	public:
		MemMgr()
		{}
	
		uint8_t* getPtr(uint32_t size);
		void freePtr(uint8_t* ptr);
	};

	
}
#endif