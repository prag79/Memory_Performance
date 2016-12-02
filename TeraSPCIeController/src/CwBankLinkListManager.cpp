#include "CwBankLinkListManager.h"
#include "reporting.h"

static const char *bankLLFile = "TeraSMemoryController.log";

/**get tail of CWbank link list manager
* @param chanNum channel number
* @param codeWordNum code word Number
* @param value tail value
* @return void
**/
void CwBankLinkListManager::getTail(const uint8_t& chanNum, const uint8_t& codeWordNum, int32_t& value)
{
	value = mBankLinkList.at(chanNum).at(codeWordNum).tail;
}

/**update head CWbank link list manager
* @param chanNum channel number
* @param codeWordNum code word Number
* @param value value to be updated
* @return void
**/
void CwBankLinkListManager::updateHead(const uint8_t& chanNum, const uint8_t& codeWordNum, const int32_t& value)
{
	mBankLinkList.at(chanNum).at(codeWordNum).head = value;
	/*std::ostringstream msg;
	msg.str("");
	msg << "Channel Number: " << dec << (uint32_t) chanNum
		<<" HEAD: " << dec << (uint32_t)value
		<< " CW Bank " << hex << (uint32_t)codeWordNum;
	REPORT_INFO(bankLLFile, __FUNCTION__, msg.str());*/
}

/**update tail CWbank link list manager
* @param chanNum channel number
* @param codeWordNum code word Number
* @param value value to be updated
* @return void
**/
void CwBankLinkListManager::updateTail(const uint8_t& chanNum, const uint8_t& codeWordNum, const int32_t& value)
{
	mBankLinkList.at(chanNum).at(codeWordNum).tail = value;
	/*std::ostringstream msg;
	msg.str("");
	msg << "Channel Number: " << dec << (uint32_t)chanNum
		<< " TAIL: " << hex << value
		<< " CW Bank " << hex << (uint32_t)codeWordNum;
	REPORT_INFO(bankLLFile, __FUNCTION__, msg.str());*/
}

/**get head of CWbank link list manager
* @param chanNum channel number
* @param codeWordNum code word Number
* @param value tail value
* @return void
**/ 
int32_t CwBankLinkListManager::getHead(const uint8_t& chanNum, const uint8_t& codeWordNum)
{
	int32_t value = mBankLinkList.at(chanNum).at(codeWordNum).head;
	return value;
}