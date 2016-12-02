#include "PendingCmdQueueManager.h"

namespace CrossbarTeraSLib {
	/** notify when a cmd is inserted in pending queue manager
	* @param value pending cmd value
	* @param sc_time time
	* @return void
	**/
	void PendingCmdQueueManager::notify(ActiveCmdQueueData& value, const sc_core::sc_time& t)
	{
		sc_time now = sc_time_stamp();
		mCmdQueue.insert(pair_type(t + now, value));
		m_event.notify(t);
	}

	void PendingCmdQueueManager::notify(const sc_core::sc_time& t)
	{
		m_event.notify(t);
	}
	/** pop command from pending queue manager
	* @return uint32_t
	**/
	bool PendingCmdQueueManager::popCommand(ActiveCmdQueueData& value)
	{
		if (mCmdQueue.empty()) {
			value.cmd = cmdType::NO_CMD;
			return false;
		}

		sc_core::sc_time now = sc_core::sc_time_stamp();
		if (mCmdQueue.begin()->first <= now) {
			value = mCmdQueue.begin()->second;
			return true;
		}
		m_event.notify(mCmdQueue.begin()->first - now);

		return false;
	}

	/** erase command from pending queue manager
	* @return void
	**/
	void PendingCmdQueueManager::erase()
	{
		mCmdQueue.erase(mCmdQueue.begin());
	}
	sc_core::sc_event& PendingCmdQueueManager::get_event()
	{
		return m_event;
	}

	/** Cancel all events from the event queue
	* @return void
	**/
	void PendingCmdQueueManager::cancel_all() {
		mCmdQueue.clear();
		m_event.cancel();
	}

	/** get CW bank number
	* @param cwBankMask CW bank Mask
	* @return uint8_t
	**/
	uint8_t PendingCmdQueueManager::getCwBankNum(const uint32_t& cwBankMask)
	{
		ActiveCmdQueueData value = mCmdQueue.begin()->second;
		uint8_t cwBankNum = (uint8_t)value.plba & cwBankMask;
		return cwBankNum;
	}

	bool PendingCmdQueueManager::isEmpty()
	{
		if (mCmdQueue.size() == 0)
		{
			return true;
		}
		else
			return false;
	}

	sc_time PendingCmdQueueManager::getTime()
	{
		return mCmdQueue.begin()->first;
	}
}