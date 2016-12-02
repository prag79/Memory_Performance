/*******************************************************************
 * File : Router.h
 *
 * Copyright(C) crossbar-inc. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains header for Bus interconnect
 * Related Specifications: 
 *
 ********************************************************************/
#ifndef _ROUTER_H_
#define _ROUTER_H_

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "pciTbCommon.h"


template<unsigned int N_TARGETS>
class Router:public sc_module
{
public:
  
  tlm_utils::simple_target_socket<Router> tbSocket;
  tlm_utils::simple_target_socket<Router> pciRCSocket;
  tlm_utils::simple_initiator_socket<Router> routerSocket[N_TARGETS];

  Router(sc_module_name _name)
  {
	  tbSocket.register_b_transport(this, &Router::tb_b_transport);
	  pciRCSocket.register_b_transport(this, &Router::tb_b_transport);

	 /* for (unsigned int i = 0; i < N_TARGETS; i++)
	  {
		  char txt[20];
		  sprintf(txt, "socket_%d", i);
		  routerSocket[i] = new tlm_utils::simple_initiator_socket_tagged<Router>(txt);
	  }*/
  }
  SC_HAS_PROCESS(Router);
  virtual void tb_b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );
  virtual void pcieRC_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  inline unsigned int decode_address( sc_dt::uint64 address, sc_dt::uint64& masked_address );
  inline sc_dt::uint64 compose_address( unsigned int target_nr, sc_dt::uint64 address);
};

template<unsigned int N_TARGETS>
void Router<N_TARGETS>::pcieRC_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
{
}

template<unsigned int N_TARGETS>
void Router<N_TARGETS>::tb_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
{
	sc_dt::uint64 address = trans.get_address();
	sc_dt::uint64 masked_address;
	unsigned int target_nr = decode_address(address, masked_address);
	trans.set_address(masked_address);
	(routerSocket[target_nr])->b_transport(trans, delay);
}

template<unsigned int N_TARGETS>
inline unsigned int Router<N_TARGETS>::decode_address(sc_dt::uint64 address, sc_dt::uint64& masked_address)
{
	unsigned int target_nr = static_cast<unsigned int>((address >> 28) & 0x3);
	masked_address = address &0xFFFFFFF;
	return target_nr;
}
template<unsigned int N_TARGETS>
inline sc_dt::uint64 Router<N_TARGETS>::compose_address(unsigned int target_nr, sc_dt::uint64 address)
{
	return (target_nr << 8) | (address & 0xFF);
}
#endif