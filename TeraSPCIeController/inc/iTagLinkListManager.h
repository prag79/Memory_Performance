/*******************************************************************
 * File : iTagLinkListManager.h
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

#ifndef ITAG_LL_MANAGER_H_
#define ITAG_LL_MANAGER_H_
#include "pcie_common.h"

class iTagLinkListManager
{
public:
	iTagLinkListManager(uint32_t codeWordNum, uint16_t chanNum)
		:mTagLinkList(chanNum, std::vector<mTagList>(codeWordNum))
	{

	}

	void updateHead(const uint8_t& chanNum, const uint8_t& codeWordNum, const int32_t& value);
	int32_t getHead(const uint8_t& chanNum, const uint8_t& codeWordNum);
	void updateTail(const uint8_t& chanNum, const uint8_t& codeWordNum, const int32_t& value);
	void getTail(const uint8_t& chanNum, const uint8_t& codeWordNum, int32_t& value);

private:
	struct TagLinkList {
		int32_t head;
		int32_t tail;
		TagLinkList()
		{
			reset();
		}
		void reset()
		{
			head = -1;
			tail = -1;
		}
	};
	typedef struct TagLinkList mTagList;
	std::vector<std::vector<mTagList> > mTagLinkList;

};
#endif