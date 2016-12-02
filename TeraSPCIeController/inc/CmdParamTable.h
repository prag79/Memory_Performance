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
#include "common.h"

namespace CrossbarTeraSLib {
	class CmdParamTable
	{
	public:
		CmdParamTable(uint32_t size)
		{
			for (uint32_t paramIndex = 0; paramIndex < size; paramIndex++)
			{
				mParamTable.push_back(cmdTableField());
			}
		}
		/*bool getCmdId(uint32_t& tag, uint16_t& cid);
		bool getBlockCnt(uint32_t& tag, uint16_t& blkCnt);
		bool getCmd(uint32_t& tag, cmdType& cmd);
		bool getLBA(uint32_t& tag, uint64_t lba);
		bool getNextAddr(uint32_t& tag, uint32_t na);*/
		bool getParam(uint32_t& tag, uint16_t& cid, cmdType& cmd, uint64_t& lba, uint16_t& blkCnt, uint64_t& hAddr0, uint64_t& hAddr1);
		bool insertParam(uint32_t& tag, uint16_t& cid, cmdType& cmd, uint64_t& lba, uint16_t& blkCnt, uint64_t& hAddr0, uint64_t& hAddr1);
		void getHostAddr(uint32_t& tag, uint64_t& hAddr0, uint64_t& hAddr1);
		bool getCID(uint32_t& tag, uint16_t& cid);
		void resetParam(uint32_t tag);
	private:
		std::vector<cmdTableField> mParamTable;

	};
}
#endif