#ifndef CMD_EXT_MANAGER__H_
#define CMD_EXT_MANAGER_H_

#include "Common.h"
#include <queue>
#include <memory>

class CmdExtManager {

public:
	CmdExtManager(){}
	void enqueue(void);
	void release(CmdExtension* extension);
	bool isEmpty(void);
	size_t size(void);
	CmdExtension* dequeue(void);
private:
	std::queue<CmdExtension* > mExtensionQueue;
};

void CmdExtManager::enqueue(void)
{
	try {
		mExtensionQueue.push(new CmdExtension());
	}
	catch (std::bad_alloc& ba){
		std::cerr << "CmdExtManager: Command Extension bad allocation" << ba.what() << std::endl;
	}

}

CmdExtension* CmdExtManager::dequeue(void)
{
	CmdExtension* extension_ptr = mExtensionQueue.front();

	mExtensionQueue.pop();

	return extension_ptr;
}

void CmdExtManager::release(CmdExtension* extension_ptr)           /// transaction pointer
{
	delete extension_ptr;
}

bool CmdExtManager::isEmpty(void)                                              /// queue empty
{
	return mExtensionQueue.empty();
}

size_t CmdExtManager::size(void)
{
	return mExtensionQueue.size();
}

#endif