/*******************************************************************
 * File : PendingCmdQueueManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Pending Command Queue used in
 * TeraSMemoryController architecture.
 * Related Specifications: 
 * TeraS_Controller_Specification_ver_1.0.doc
 ********************************************************************/
#ifndef PENDING_CMD_QUEUE_MANAGER_H_
#define PENDING_CMD_QUEUE_MANAGER_H_
#include "dimm_common.h"
#include "systemc.h"
#include <map>

class PendingCmdQueueManager: public sc_core::sc_object
{
public:
	typedef std::pair<const sc_core::sc_time, uint64_t> pair_type;
	PendingCmdQueueManager(const char* name) : sc_core::sc_object(name)
	{
		
	}

	uint64_t popCommand();
	void erase();
	void notify(uint64_t& value, const sc_core::sc_time& t);
	sc_core::sc_event& get_event();
	void cancel_all();
	uint16_t getCwBankNum(const uint32_t& cwBankMask);

private:

	std::multimap<const sc_core::sc_time, uint64_t > mCmdQueue;
	sc_core::sc_event m_event;
	
};



#endif