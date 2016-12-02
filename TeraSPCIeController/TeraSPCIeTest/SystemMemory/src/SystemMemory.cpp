/*******************************************************************
 * File : SystemMemory.cpp
 *
 * Copyright(C) crossbar-inc. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains functionality definition of memory module
 * Related Specifications: 
 *
 ********************************************************************/
#include "SystemMemory.h"
  

SystemMemory::SystemMemory(sc_module_name _name, const sc_core::sc_time ReadDelay,const sc_core::sc_time WriteDelay) 
	: t_socket("socket")
	, mReadDelay(ReadDelay)
	, mWriteDelay(WriteDelay)
{
  t_socket.register_b_transport(this, &SystemMemory::b_transport);
 
  memset(mem, 0, MEMSIZE);
}

void SystemMemory::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
{
  tlm::tlm_command cmd = trans.get_command();
  sc_dt::uint64    adr = trans.get_address() ;
  unsigned char*   ptr = trans.get_data_ptr();
  unsigned int     len = trans.get_data_length();
  unsigned char*   byt = trans.get_byte_enable_ptr();
  unsigned int     wid = trans.get_streaming_width();

  sc_time tScheduled = SC_ZERO_TIME;
  adr -= BA_MEM;
  if (adr >= MEMSIZE * 4) {
    trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
    return;
  }
  if (byt != 0) {
    trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
    return;
  }
 /* if (len > 4 || wid < len) {
    trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
    return;
  }*/

  //wait(delay);
  //delay = SC_ZERO_TIME;
  if ((adr >= BA_MEM + BA_CQ0) && (adr < BA_MEM + BA_CQ0 * SZ_CQ0)){
	  if (cmd == tlm::TLM_READ_COMMAND)
	  {
		  memcpy(ptr, &mem[adr], len);
		  tScheduled += mReadDelay;
	  }
	  else if (cmd == tlm::TLM_WRITE_COMMAND)
	  {
		  memcpy(&mem[adr], ptr, len);
		  tScheduled += mWriteDelay;
	  }
  }
  else
  {
	  if (cmd == tlm::TLM_READ_COMMAND)
	  {
		  if (len > 0){
			  for (uint32_t numCmd = 0; numCmd < len / 64; numCmd++)
			  {
				  memcpy(ptr + 64 * numCmd, &mem[adr + 64 * numCmd], 64);
			  }
			  tScheduled += mReadDelay;
		  }
	  }
	  else if (cmd == tlm::TLM_WRITE_COMMAND)
	  {
		  for (uint32_t numCmd = 0; numCmd < len / 64; numCmd++)
		  {
			  memcpy(&mem[adr + 64 * numCmd], ptr + 64 * numCmd, 64);
		  }
		  tScheduled += mWriteDelay;
	  }
  }
  delay = tScheduled;
  trans.set_response_status( tlm::TLM_OK_RESPONSE );
}

unsigned int SystemMemory::decode_address(sc_dt::uint64 address, sc_dt::uint64& masked_address)
{
	unsigned int target_nr = static_cast<unsigned int>((address >> 16) & 0x3);
	masked_address = address & 0xFFFF;
	return target_nr;
}



