#include "MemMgr.h"
namespace CrossbarTeraSLib {

	uint8_t* MemMgr::getPtr(uint32_t size)
	{
		return new uint8_t[size];
	}

	void MemMgr::freePtr(uint8_t* ptr)
	{
		delete [] ptr;
	}
}