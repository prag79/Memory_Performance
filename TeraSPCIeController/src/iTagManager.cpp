#include "iTagManager.h"

namespace CrossbarTeraSLib {
	bool iTagManager::getFreeTag(uint32_t& tag)
	{
		uint32_t tagIndex = 0;
		for (uint32_t tagIndex = 0; tagIndex < mSize; tagIndex++)
		{
			if (iTagTable.at(tagIndex) == false)
			{
				tag = tagIndex;
				return true;
			}
		}
		return false;
	}

	bool iTagManager::setTagBusy(uint32_t tag)
	{
		if (tag < iTagTable.size())
		{
			iTagTable.at(tag) = true;
			return true;
		}
		else
			return false;
	}

	bool iTagManager::resetTag(uint32_t tag)
	{
		if (tag < iTagTable.size())
		{
			iTagTable.at(tag) = false;
			return true;
		}
		else
			return false;
	}
}