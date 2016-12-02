#include "CmdParamTable.h"


namespace CrossbarTeraSLib {
	bool CmdParamTable::getParam(uint32_t& tag, uint16_t& cid, cmdType& cmd, \
		uint64_t& lba, uint16_t& blkCnt, uint64_t& hAddr0, uint64_t& hAddr1)
	{
		if (tag < mParamTable.size())
		{

			cid = mParamTable.at(tag).cid;
			cmd = mParamTable.at(tag).cmd;
			lba = mParamTable.at(tag).lba;
			blkCnt = mParamTable.at(tag).blockCnt;
			hAddr0 = mParamTable.at(tag).hAddr0;
			hAddr1 = mParamTable.at(tag).hAddr1;
			return true;
		}
		else
			return false;
	}

	bool CmdParamTable::getCID(uint32_t& tag, uint16_t& cid)
	{
		cid = mParamTable.at(tag).cid;
		return true;
	}
	bool CmdParamTable::insertParam(uint32_t& tag, uint16_t& cid, cmdType& cmd, \
		uint64_t& lba, uint16_t& blkCnt, uint64_t& hAddr0, uint64_t& hAddr1)
	{
		if (tag < mParamTable.size())
		{

			mParamTable.at(tag).cid = cid;
			mParamTable.at(tag).cmd = cmd;
			mParamTable.at(tag).lba = lba;
			mParamTable.at(tag).blockCnt = blkCnt;
			mParamTable.at(tag).hAddr0 = hAddr0;
			mParamTable.at(tag).hAddr1 = hAddr1;
			return true;
		}
		else
			return false;
	}

	void CmdParamTable::resetParam(uint32_t tag)
	{
		mParamTable.at(tag).reset();
	}
	void CmdParamTable::getHostAddr(uint32_t& tag, uint64_t& hAddr0, uint64_t& hAddr1)
	{
		hAddr0 = mParamTable.at(tag).hAddr0;
		hAddr1 = mParamTable.at(tag).hAddr1;
	}
}