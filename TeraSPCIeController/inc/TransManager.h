/*******************************************************************
 * File : TransManager.h
 *
 * Copyright(C) crossbar-inc. 2016  
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of transaction Memory Manager
 * used in AT modeling.
 * Related Specifications: 
 * SystemC_LRM.pdf
 ********************************************************************/

// *******************************************************************
// User-defined memory manager, which maintains a pool of transactions
// *******************************************************************
#ifndef TRANS_MANAGER_H_
#define TRANS_MANAGER_H_

#include "tlm.h"

namespace CrossbarTeraSLib {
	class TransManager : public tlm::tlm_mm_interface
	{
		typedef tlm::tlm_generic_payload gp_t;

	public:
		TransManager() : free_list(0), empties(0) {}

		gp_t* allocate();
		void  free(gp_t* trans);

	private:
		struct access
		{
			gp_t* trans;
			access* next;
			access* prev;
		};

		access* free_list;
		access* empties;
	};
}

#endif