
#include "ActiveCommandQueue.h"

namespace CrossbarTeraSLib {
	/** insert command in Active Command queue
	* @param cmddata command
	* @return void
	**/
	void ActiveCommandQueue::pushQueue(ActiveCmdQueueData cmdData)
	{
		if (mQueue.size() < mQueueLength) {
			mQueue.push_back(cmdData);
		}
		else {

		}
	}
	/** pop command in Active Command queue
	* @return ActiveCmdQueueaData
	**/
	ActiveCmdQueueData ActiveCommandQueue::popQueue()
	{
		if (!isEmpty())	{
			it = mQueue.begin();
			ActiveCmdQueueData temp = *it;

			mQueue.erase(it);
			return (temp);
		}
		else {

			ActiveCmdQueueData pd;
			return pd;
		}
	}

	/** peek command in Active Command queue
	* @return int64_t
	**/
	int64_t ActiveCommandQueue::peekQueue()
	{
		if (!isEmpty())	{
			ActiveCmdQueueData value = mQueue.front();
			return value.cmd;
		}
		else {
			return -1;

		}
	}

	/** clear  Active Command queue
	* @return void
	**/
	void ActiveCommandQueue::freeQueue()
	{
		mQueue.clear();
	}

	/** check Active Command queue is empty
	* @return bool
	**/
	bool ActiveCommandQueue::isEmpty()
	{
		return mQueue.empty();
	}

	/** check Active Command queue is full
	* @return bool
	**/
	bool ActiveCommandQueue::isFull()
	{
		if (mQueue.size() == mQueueLength)
			return true;
		else
			return false;
	}

	/** get Active Command queue size
	* @return size_t
	**/
	size_t ActiveCommandQueue::getSize()
	{
		return mQueue.size();
	}

	void ActiveDMACmdQueue::pushQueue(ActiveDMACmdQueueData cmdData)
	{
		if (mQueue.size() < mQueueSize) {
			mQueue.push_back(cmdData);
		}
		else {

		}
	}
	/** pop command in Active Command queue
	* @return ActiveCmdQueueaData
	**/
	ActiveDMACmdQueueData ActiveDMACmdQueue::popQueue()
	{
		if (!isEmpty())	{
			it = mQueue.begin();
			ActiveDMACmdQueueData temp = *it;

			mQueue.erase(it);
			return (temp);
		}
		else {

			ActiveDMACmdQueueData pd;
			return pd;
		}
	}

	/** peek command in Active Command queue
	* @return int64_t
	**/
	int64_t ActiveDMACmdQueue::peekQueue()
	{
		if (!isEmpty())	{
			ActiveDMACmdQueueData value = mQueue.front();
			return value.cmd;
		}
		else {
			return -1;

		}
	}

	/** clear  Active Command queue
	* @return void
	**/
	void ActiveDMACmdQueue::freeQueue()
	{
		mQueue.clear();
	}

	/** check Active Command queue is empty
	* @return bool
	**/
	bool ActiveDMACmdQueue::isEmpty()
	{
		return mQueue.empty();
	}

	/** check Active Command queue is full
	* @return bool
	**/
	bool ActiveDMACmdQueue::isFull()
	{
		if (mQueue.size() == mQueueSize)
			return true;
		else
			return false;
	}

	/** get Active Command queue size
	* @return size_t
	**/
	size_t ActiveDMACmdQueue::getSize()
	{
		return mQueue.size();
	}
}