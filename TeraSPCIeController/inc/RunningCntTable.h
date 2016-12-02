/*******************************************************************
 * File : RunningCntTable.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Running Count Table used in
 * TeraSPCIeController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/

#ifndef RUNNING_CNT_TABLE_H_
#define RUNNING_CNT_TABLE_H_

#include "Common.h"

namespace CrossbarTeraSLib {
	class RunningCntTable
	{

	public:
		RunningCntTable(uint32_t size, uint32_t cwSize)
			: mCwSize(cwSize)
		{
			mRunningCntTable.resize(size, 0);
		}

		void insert(const uint32_t tag, const uint32_t blkCnt);
		void updateCnt(const uint32_t tag);
		uint16_t getRunningCnt(const uint32_t tag);

	private:

		std::vector<uint16_t> mRunningCntTable;
		std::vector<uint16_t>::iterator it;
		uint32_t mCwSize;
	};
}
#endif