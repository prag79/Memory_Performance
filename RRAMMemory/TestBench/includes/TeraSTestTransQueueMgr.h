#ifndef TESTS_TERAS_CMD_QUEUE_H_
#define TESTS_TERAS_CMD_QUEUE_H_

#include "tlm.h"
#include "Common.h"
#include <queue>


class TransQueueManager : public tlm::tlm_mm_interface
{

public:
	TransQueueManager(){}
	void enqueue(void);
	void release(tlm::tlm_generic_payload *transaction_ptr);
	bool isEmpty(void);
	size_t size(void);
	virtual void free(tlm::tlm_generic_payload *transaction_ptr);
	tlm::tlm_generic_payload* dequeue(void);

private:
	std::queue<tlm::tlm_generic_payload*> mPayloadQueue;
};

/** enqueue()
* Create Payload Pointer and push it
* in a queue
* @return void
**/
void TransQueueManager::enqueue(void)
{
	try {
		tlm::tlm_generic_payload  *transaction_ptr = new tlm::tlm_generic_payload(this); /// transaction pointer
		mPayloadQueue.push(transaction_ptr);
	}
	catch (std::bad_alloc& ba){
		std::cerr << "TransQueueManager: Transaction Pointer bad allocation" << ba.what() << std::endl;
	}
	
}

/** dequeue()
* Pop payload Pointer from the
* Queue
* @return void
**/
tlm::tlm_generic_payload* TransQueueManager::dequeue(void)
{
	tlm::tlm_generic_payload *transaction_ptr = mPayloadQueue.front();

	mPayloadQueue.pop();

	return transaction_ptr;
}

/** release()
* Release payload pointer
* @param tlm::tlm_generic_payload 
* @return void
**/
void TransQueueManager::release(tlm::tlm_generic_payload *transaction_ptr)           /// transaction pointer
{
	transaction_ptr->release();
}

/** isEmpty()
* Check if queue is empty
* @return bool
**/
bool TransQueueManager::isEmpty(void)                                              /// queue empty
{
	return mPayloadQueue.empty();
}

/** size()
* Returns size of the queue
* @return size_t
**/
size_t TransQueueManager::size(void)
{
	return mPayloadQueue.size();
}

/** free()
* resets the payload pointer and pushes it into the queue
* @return size_t
**/
void TransQueueManager::free(tlm::tlm_generic_payload *transaction_ptr)
{
	transaction_ptr->reset();
	mPayloadQueue.push(transaction_ptr);
}


#endif