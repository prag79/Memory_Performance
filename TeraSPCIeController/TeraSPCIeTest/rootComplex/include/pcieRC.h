/*******************************************************************
 * File : TestBench.h
 *
 * Copyright(C) crossbar-inc. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains stureucture for Test Bench for PCIe
 * Related Specifications: 
 *
 ********************************************************************/
#ifndef _PCIE_RC_H_
#define _PCIE_RC_H_

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "pciTbCommon.h"
#include "TLPManager.h"
#include "Reporting.h"
#include "Report.h"
#include "MemMgr.h"
#include "tlp.h"
#include <queue>
#include "pcie_common.h"
#include "Config.h"
#include "NvmeQueue.h"

using namespace CrossbarTeraSLib;
static const char *filePCIeRC = "pcieRC.h";
class pcieRC :public sc_module{
public:
	//testbench interface
	tlm_utils::simple_initiator_socket<pcieRC> hostIntfTx;
	tlm_utils::simple_target_socket<pcieRC> hostIntfRx;
	//PCIe controller interface
	tlm_utils::simple_initiator_socket<pcieRC> cntrlIntfTx;
	tlm_utils::simple_target_socket <pcieRC> cntrlIntfRx;
	
	SC_HAS_PROCESS(pcieRC);
	pcieRC(sc_module_name name, uint32_t pciSpeed, uint32_t queueSize, uint32_t cwSize, std::fstream &LogFileHandler, uint32_t SizeOfSQ, uint32_t SizeOfCQ, bool enMultiSim, uint32_t blockSize, uint32_t ioSize, uint32_t tbQueueDepth);
	~pcieRC();

	
private:
#pragma region INTERFACE_METHOD
	/**
	/	reponsible for TB transaction
	**/
	void host_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
	/**
	/	reponsible for PCIe controller transaction
	**/
	void cntrl_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
#pragma endregion
#pragma region TLP_CREATE_FUNC
	/**
	/	creation of configure Write TLP
	**/
	void createConfigWriteTLP(const uint64_t& adr, const uint64_t& data, const unsigned int& len, \
		tConfigWriteReq& iConfigWriteReg);
	/**
	/	creation of completion TLP
	**/
	void createCmplTlp(uint8_t* data, tCompletionTLP& iCmplTlp, const uint32_t& length, const uint32_t& tag);
#pragma endregion
#pragma region MEM_FUNC
	string appendParameters(string name, string format);
	
	/** reponsible for wraping of address to base 
	 /	when SQ is full
	/	@return num of SQ left before SQ full
	**/
	uint32_t CheckNumOfSq(const uint32_t &addr, const uint32_t &len, bool &wrapFlag);
	/**
	/	send TLP to PCIe contrl
	/	calls PCIe b_transport
	**/
	void sendTLP2Dut(uint8_t *dataPtr, const uint32_t& tlpSize);
	/**
	/	send TLP to PCIe contrl
	/	calls PCIe b_transport
	**/
	void sendReg2Dut(uint8_t *dataPtr, const uint32_t& tlpSize, sc_time& delay);
	/**
	/	send TLP/Data to system mem
	**/
	void send2Mem(uint8_t *dataPtr, const uint32_t& length, const uint32_t& addr, tlm::tlm_command type);
	/** arbritaion in TLP queues
	/	@return available TLP
	**/
	TLPQueueType queueArbiterForSysMem();
	TLPQueueType queueArbiterForDut();
	
	//following function used for logging
#pragma region LOGS_FUNC
	void logRcCntrl(const uint64_t &adr, const uint32_t &len);
	void logWrTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len);
	void logRdTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len);
	void logThreadRdTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len);
	void logThreadWrTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len);
#pragma endregion
	
#pragma endregion
#pragma region STATIC_THREAD
	/**
	/	main thread
	**/
	//void pcieRCThread();
	void pcieRCThreadForDut();
	void pcieRCThreadForSysMem();
#pragma endregion
#pragma region MEM_OBJECT
	MemMgr						mMemMngr;
	TLPManager					mTLPQueueMgr;
#pragma endregion

#pragma region MEM_VARIABLES
	uint32_t			mPciSpeed;
	uint32_t            mNumCmd;
	uint8_t				mArbiterToken;
	uint8_t				*tlpCQPayload;
	uint8_t				*tlpDataPayload;
	tMemoryWriteReqTLP	iMemWrRqTlp;
	tMemoryReadReqTLP	tMemRdRqTlp;
	
	sc_event_queue		mMemRdReqTlpQEvent;
	sc_event_queue		mMemWrReqTlpQEvent;

	std::fstream		&mLogFileHandler;
	Report*				mRxBusUtilReport;
	Report*				mRxBusUtilReportCsv;
	Report*				mTxBusUtilReport;
	Report*				mTxBusUtilReportCsv;

	CircularQueue		*mNSubQue;
	CircularQueue		*mNComQue;

	uint32_t			mSizeOfSQ;
	uint32_t			mSizeOfCQ;
	bool				mEnMultiSim;
	uint32_t			mBlockSize;
	uint32_t			mIOSize;
	uint32_t            mTbQueueDepth;
	bool				wrapFlag;

#pragma endregion
};
#endif