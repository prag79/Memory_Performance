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
#include "Common.h"
#include "systemc.h"
#include <map>

namespace CrossbarTeraSLib {
	class PendingCmdQueueManager : public sc_core::sc_object
	{
	public:
		typedef std::pair<const sc_core::sc_time, ActiveCmdQueueData> pair_type;
		PendingCmdQueueManager(const char* name) : sc_core::sc_object(name)
		{

		}

		bool popCommand(ActiveCmdQueueData& value);
		void erase();
		void notify(ActiveCmdQueueData& value, const sc_core::sc_time& t);
		sc_core::sc_event& get_event();
		void cancel_all();
		uint8_t getCwBankNum(const uint32_t& cwBankMask);
		bool isEmpty();
		sc_time getTime();
		void notify(const sc_core::sc_time& t);
	private:

		std::multimap<const sc_core::sc_time, ActiveCmdQueueData > mCmdQueue;
		sc_core::sc_event m_event;

	};
}


#endif