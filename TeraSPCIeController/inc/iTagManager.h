/*******************************************************************
 * File : iTagLinkListManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Free iTags Manager used in
 * TeraSPCIeController architecture.
 * Related Specifications: 
 * PCIeControllerSpec.doc
 ********************************************************************/
#ifndef ITAG_MANAGER_H_
#define ITA_MANAGER_H_
#include "Common.h"
#include <map>

namespace CrossbarTeraSLib {
	class iTagManager
	{
	public:
		iTagManager(uint32_t size)
			:mSize(size)
		{
			for (uint32_t iTagIndex = 0; iTagIndex < size; iTagIndex++)
			{
				iTagTable.insert(std::pair<uint32_t, bool>(iTagIndex, false));
			}
		}
		bool getFreeTag(uint32_t& tag);
		bool setTagBusy(uint32_t tag);
		bool resetTag(uint32_t tag);
	private:
		std::map<uint32_t, bool> iTagTable;
		uint32_t mSize;


	};
}
#endif