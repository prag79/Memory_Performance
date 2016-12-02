/*******************************************************************
 * File : CwBankLinkListManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Code Word Bank Link list used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/

#ifndef CW_BANK_LL_MANAGER_H_
#define CW_BANK_LL_MANAGER_H_
#include "dimm_common.h"

class CwBankLinkListManager
{
public:
	CwBankLinkListManager(uint32_t codeWordNum, uint16_t chanNum)
		:mBankLinkList(chanNum, std::vector<mBankList>(codeWordNum))
	{
	
	}

	void updateHead(const uint8_t& chanNum, const uint16_t& codeWordNum, const int32_t& value);
	int32_t getHead(const uint8_t& chanNum, const uint16_t& codeWordNum);
	void updateTail(const uint8_t& chanNum, const uint16_t& codeWordNum, const int32_t& value);
	void getTail(const uint8_t& chanNum, const uint16_t& codeWordNum, int32_t& value);

private:
	struct BankLinkList {
		int32_t head;
		int32_t tail;
		BankLinkList()
		{
			reset();
		}
		void reset()
		{
			head = -1;
			tail = -1;
		}
	};
	typedef struct BankLinkList mBankList;
	std::vector<std::vector<mBankList> > mBankLinkList;
	
};
#endif