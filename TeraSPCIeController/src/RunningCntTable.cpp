#include "RunningCntTable.h"

namespace CrossbarTeraSLib {
	/** insert subcommand Count
	* @param tag iTag based index
	* @param blkCnt Block Count
	* @return void
	**/
	void RunningCntTable::insert(const uint32_t tag, const uint32_t blkCnt)
	{
		mRunningCntTable[tag] = (uint16_t)(blkCnt);// << (mBlkSize/mCwSize));

		mRunningCntTable[tag];

	}
	/** decrements the subcommand count
	* @param tag iTag based index
	* @return void
	**/
	void RunningCntTable::updateCnt(const uint32_t tag)
	{
		mRunningCntTable[tag]--;
	}
	/** get the current sub command count
	* @param tag iTag based index
	* @return uint16_t
	**/
	uint16_t RunningCntTable::getRunningCnt(const uint32_t tag)
	{
		return mRunningCntTable[tag];
	}

}