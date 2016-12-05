/*******************************************************************
 * File : CmdParamTable.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Command Parameter Table used in
 * TeraSPCIeController architecture to store command parameters. It is indexed
 * by free iTags.
 * Related Specifications: 
 * PCIeControllerSpec.doc
 ********************************************************************/
#ifndef __CMD_PARAM_TABLE_H
#define __CMD_PARAM_TABLE_H

#include <cstdint>
#include "pcie_common.h"
#include <map>

namespace CrossbarTeraSLib {
	class PCmdQueueLLManager
	{
	public:
		PCmdQueueLLManager(uint32_t size)
			:mSize(size)
		{
			for (uint32_t qIndex = 0; qIndex < size; qIndex++)
			{
				pCmdQStatus.insert(std::pair<uint32_t, status>(qIndex, status::FREE));
			}
		}
		bool getFreeQueueAddr(uint32_t& qAddr);
		void setQueueBusy(uint32_t& qAddr);
		void resetQueueStatus(uint32_t& qAddr);
	private:
		std::map<uint32_t, status> pCmdQStatus;
		uint32_t mSize;
	};
}
#endif