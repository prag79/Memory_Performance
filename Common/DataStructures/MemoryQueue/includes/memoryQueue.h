#ifndef __MemoryQUEUE_H__
#define __MemoryQUEUE_H__

#include<vector>
#include<cstdint>

template<typename T>
class MemoryQueue
{
public:
	MemoryQueue(uint32_t queueSize)
		:mQueueSize(queueSize)
	{
		mQueue.resize(queueSize);
	};

	void pushToQueue(T *cmdData);
	T* popQueue();
	T* peekQueue(uint32_t position);
	//void release(tlm::tlm_generic_payload& cmdData);
	void freeQueue();

	inline bool isEmpty();
	inline bool isFull();
	inline size_t getSize();


private:
	uint32_t mQueueSize;
	std::vector<T*> mQueue;
	typename std::vector<T*>::iterator it;
};

template<typename T>
void MemoryQueue<T>::pushToQueue(T* cmdData)
{
	if (mQueue.size() < mQueueSize) {
		mQueue.push_back(cmdData);
	}
	else {
		//TODO: REPORT ERROR
	}
}

template<typename T>
T* MemoryQueue<T>::popQueue()
{
	if (!isEmpty())	{
		it = mQueue.begin();
		T* temp = *it;

		mQueue.erase(it);
		return (temp);
	}
	else {
		//TODO: REPORT_ERROR
		T* pd;
		pd = new T();

		return pd;
	}
}

template<typename T>
T* MemoryQueue<T>::peekQueue(uint32_t position)
{
	if (!isEmpty())	{
		return mQueue.at(position);
	}
	else {
		//TODO:REPORT ERROR
	}
}

//void MemoryQueue::release(tlm::tlm_generic_payload cmdData)
//{
//	cmdData.release();
//}

template<typename T>
void MemoryQueue<T>::freeQueue()
{
	mQueue.clear();
}

template<typename T>
bool MemoryQueue<T>::isEmpty()
{
	return mQueue.empty();
}

template<typename T>
bool MemoryQueue<T>::isFull()
{
	if (mQueue.size() == mQueueSize)
		return true;
	else
		return false;
}

template<typename T>
size_t MemoryQueue<T>::getSize()
{
	return mQueue.size();
}
#endif