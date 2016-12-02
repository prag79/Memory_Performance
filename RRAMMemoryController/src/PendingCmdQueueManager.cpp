#include "PendingCmdQueueManager.h"

/** notify when a cmd is inserted in pending queue manager
* @param value pending cmd value
* @param sc_time time 
* @return void
**/
void PendingCmdQueueManager::notify(uint64_t& value, const sc_core::sc_time& t)
{
	mCmdQueue.insert(pair_type(t + sc_core::sc_time_stamp(), value));
	m_event.notify(t);
}


/** pop command from pending queue manager
* @return uint32_t
**/
uint64_t PendingCmdQueueManager::popCommand()
{
	if (mCmdQueue.empty()) {
		return 0;
	}

	sc_core::sc_time now = sc_core::sc_time_stamp();
	if (mCmdQueue.begin()->first <= now) {
		uint64_t value = mCmdQueue.begin()->second;
		return value;
	}
	m_event.notify(mCmdQueue.begin()->first - now);

	

	return 0;
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
uint16_t PendingCmdQueueManager::getCwBankNum(const uint32_t& cwBankMask)
{
	uint32_t value = mCmdQueue.begin()->second;
	uint16_t cwBankNum = (value >> 9) & cwBankMask;
	return cwBankNum;
}
