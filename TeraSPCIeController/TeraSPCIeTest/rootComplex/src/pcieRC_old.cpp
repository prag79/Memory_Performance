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
#include "pcieRC.h"
pcieRC::pcieRC(sc_module_name name, uint32_t pciSpeed, uint32_t queueSize, uint32_t cwSize, std::fstream &LogFileHandler, uint32_t SizeOfSQ, uint32_t SizeOfCQ, bool enMultiSim, uint32_t blockSize, uint32_t ioSize, uint32_t tbQueueDepth)
	:sc_module(name),
	mPciSpeed(pciSpeed),
	mTLPQueueMgr(queueSize, cwSize),
	mLogFileHandler(LogFileHandler),
	mNumCmd(0),
	mSizeOfSQ(SizeOfSQ),
	mSizeOfCQ(SizeOfSQ),
	mEnMultiSim(enMultiSim),
	mBlockSize(blockSize),
	mIOSize(ioSize),
	mTbQueueDepth(tbQueueDepth)
{
	//mLogFileHandler.open("./Logs/TeraSPCIeRootComplex.log", std::fstream::trunc | std::fstream::out);
	hostIntfRx.register_b_transport(this, &pcieRC::host_b_transport);
	cntrlIntfRx.register_b_transport(this, &pcieRC::cntrl_b_transport);

	mNSubQue = new CircularQueue(true, BA_MEM + BA_SQ0, mSizeOfSQ);
	//mNComQue = new CircularQueue(false, BA_MEM + BA_CQ0, mSizeOfCQ);
	
	SC_THREAD(pcieRCThread);
	sensitive << mMemReqTlpQEvent;
	//sensitive << mMemWrReqTlpQEvent;
	dont_initialize();

#pragma region REPORT_INSTANCE
	try{
		if (mEnMultiSim){
			string filename = "./Reports_PCIe/RxPci_bus_utilization";
			mRxBusUtilReport = new Report(appendParameters(filename, ".log"));
			mRxBusUtilReportCsv = new Report(appendParameters(filename, ".csv"));
			filename = "./Reports_PCIe/TxPci_bus_utilization";
			mTxBusUtilReport = new Report(appendParameters(filename, ".log"));
			mTxBusUtilReportCsv = new Report(appendParameters(filename, ".csv"));
		}
		else{
			mRxBusUtilReport = new Report("./Reports_PCIe/RxPci_bus_utilization.log");
			mRxBusUtilReportCsv = new Report("./Reports_PCIe/RxPci_bus_utilization.csv");
			mTxBusUtilReport = new Report("./Reports_PCIe/TxPci_bus_utilization.log");
			mTxBusUtilReportCsv = new Report("./Reports_PCIe/TxPci_bus_utilization.csv");
		}
	}
	catch (std::bad_alloc& ba)
	{
		std::cerr << "PciRC:Bad File allocation" << ba.what() << std::endl;
	}

	if (!mRxBusUtilReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "RX_PCI_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filePCIeRC, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mRxBusUtilReportCsv->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "RX_PCI_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filePCIeRC, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}

	if (!mTxBusUtilReport->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "TX_PCI_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filePCIeRC, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
	if (!mTxBusUtilReportCsv->openFile())
	{
		std::ostringstream msg;
		msg.str("");
		msg << "RX_PCI_BUS_REPORT LOG: "
			<< " ERROR OPENING FILE" << std::endl;
		REPORT_FATAL(filePCIeRC, __FUNCTION__, msg.str());
		exit(EXIT_FAILURE);
	}
#pragma endregion
}

pcieRC::~pcieRC(){
	mRxBusUtilReport->closeFile();
	mRxBusUtilReportCsv->closeFile();
	mTxBusUtilReport->closeFile();
	mTxBusUtilReportCsv->closeFile();
	//mLogFileHandler.close();
	mMemMngr.freePtr(tlpCQPayload);
	mMemMngr.freePtr(tlpDataPayload);
}

#pragma region INTERFACE_METHODS
	/** b_transport registered with simple_target_socket
	* hostIntfRx
	* This function implements Rx interface from TB
	* Has following function
	* register read/write,
	* @return void
	**/
	void pcieRC::host_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay){
	tlm::tlm_command cmd = trans.get_command();
	sc_dt::uint64    adr = trans.get_address() ;
	unsigned char*   ptr = trans.get_data_ptr();
	unsigned int     len = trans.get_data_length();
	std::ostringstream msg;

	//adr += BA_CNTL;//temp
	if (!((adr == (CAP_ADD & 0xFFFFFFF)) || (adr == (VS_ADD & 0xFFFFFFF)) || (adr == (BA_CNTL & 0xFFFFFFF)) \
		|| (adr == (INTMS_ADD & 0xFFFFFFF)) || (adr == (INTMC_ADD & 0xFFFFFFF)) || (adr == (CC_ADD & 0xFFFFFFF))\
		|| (adr == (RES_ADD & 0xFFFFFFF)) || (adr == (CSTS_ADD & 0xFFFFFFF)) || (adr == (NSSR_ADD & 0xFFFFFFF))\
		|| (adr == (AQA_ADD & 0xFFFFFFF)) || (adr == (ASQ_ADD & 0xFFFFFFF)) || (adr == (ACQ_ADD & 0xFFFFFFF)) \
		|| (adr == (SQ0TDBL_ADD & 0xFFFFFFF)) || (adr == (CQ0HDBL_ADD & 0xFFFFFFF)) || (adr == (SQ1TDBL_ADD & 0xFFFFFFF)) \
		|| (adr == (CQ1HDBL_ADD & 0xFFFFFFF))))
	
	{
		trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
		return;
	}
	if (!((len == 8) || (len == 4))){
		trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
		return;
	}
	msg.str("");
	msg << "\n pcieRC::b_transport() called from host "
		<< "   address=0x" << hex << (uint32_t)adr 
		<< "   length="<<hex <<len
		<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
	REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
	mLogFileHandler << "\n pcieRC::b_transport() called from host "
		<< "  address=0x" << hex << (uint32_t)adr
		<< "  length=" << hex << len
		<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns" ;
	

	if (cmd == tlm::TLM_WRITE_COMMAND){
		
		ConfigWriteReq iConfigWriteReg = { 0 };
		createConfigWriteTLP(adr, *reinterpret_cast<uint64_t*>(ptr), len,iConfigWriteReg);
		sendTLP2Dut(reinterpret_cast<uint8_t*>(&iConfigWriteReg),len);
	}
	else{
		trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
		return;
	}
	trans.set_response_status(tlm::TLM_OK_RESPONSE);
}
	/** b_transport registered with simple_target_socket
	* cntrlIntfRx
	* This function implements Rx interface from Pci Cntrl
	* Has following function
	* read/write TLP Processing 
	* @return void
	**/	
	void pcieRC::cntrl_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay){
		tlm::tlm_command cmd = trans.get_command();
		sc_dt::uint64    adr = trans.get_address();
		unsigned char*   ptr = trans.get_data_ptr();
		unsigned int     len = trans.get_data_length();

		logRcCntrl(adr, len);
		tDw0MemoryTLP dw0Tlp;
		memcpy(&dw0Tlp, reinterpret_cast<tDw0MemoryTLP*>(ptr), sizeof(tDw0MemoryTLP));

		//for Write TLP
		if ((dw0Tlp.format == 0x02) && (cmd == tlm::TLM_WRITE_COMMAND)){
			//iMemWrRqTlp = new tMemoryWriteReqTLP();
			memcpy(&iMemWrRqTlp, reinterpret_cast<tMemoryWriteReqTLP*>(ptr), sizeof(tMemoryWriteReqTLP));
			
			//for Write TLP with Completion tlp
			if (dw0Tlp.length == 4){
				tlpCQPayload = mMemMngr.getPtr(16);
				memcpy(tlpCQPayload, iMemWrRqTlp.data, 16);
				iMemWrRqTlp.data = tlpCQPayload;
				//mNumCmd++;
				logWrTlp(1, iMemWrRqTlp.address, MEM_WR_COMPLETION_WIDTH);
				mTLPQueueMgr.pushTLP(MEM_WRITE_COMP_Q, reinterpret_cast<uint8_t*>(&iMemWrRqTlp));
			}
			//for Write TLP with Data
			else{
				tlpDataPayload = mMemMngr.getPtr(dw0Tlp.length * 4);
				memcpy(tlpDataPayload, iMemWrRqTlp.data, (dw0Tlp.length * 4));
				iMemWrRqTlp.data = tlpDataPayload;

				logWrTlp(1, iMemWrRqTlp.address, MEM_WR_COMPLETION_WIDTH);
				mTLPQueueMgr.pushTLP(MEM_WRITE_DATA_Q, reinterpret_cast<uint8_t*>(&iMemWrRqTlp));
			}
			mMemReqTlpQEvent.notify(SC_ZERO_TIME);
		}
		//for Read TLP
		else if ((dw0Tlp.format == 0x0) && (cmd == tlm::TLM_READ_COMMAND)){
			memcpy(&tMemRdRqTlp, reinterpret_cast<tMemoryReadReqTLP*>(ptr), sizeof(tMemoryReadReqTLP));

			//for Read TLP with cmds
			if (((tMemRdRqTlp.Dw1MemReadReqTLP.tag & TAG_MASK) >> 6) == 0x1){

				//mNumCmd += (tMemRdRqTlp.Dw0MemReadReqTLP.length/16);//for debug
				logRdTlp(1, tMemRdRqTlp.address, tMemRdRqTlp.Dw0MemReadReqTLP.length);
				mTLPQueueMgr.pushTLP(MEM_READ_CMD_Q, ptr);
			}
			//for Read TLP with data
			else if ((((tMemRdRqTlp.Dw1MemReadReqTLP.tag & TAG_MASK) >> 6) == 0x2 \
				|| ((tMemRdRqTlp.Dw1MemReadReqTLP.tag & TAG_MASK) >> 6) == 0x3)){

				//mNumCmd++;
				logRdTlp(0, tMemRdRqTlp.address, tMemRdRqTlp.Dw0MemReadReqTLP.length);
				mTLPQueueMgr.pushTLP(MEM_READ_DATA_Q, ptr);
			}
			mMemReqTlpQEvent.notify(SC_ZERO_TIME);
		}
		else{
			trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
			return;
		}
		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		wait(delay);
		mRxBusUtilReport->writeFile(sc_time_stamp().to_double(), len, " ");
		mRxBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), len, ",");
	}
#pragma endregion

#pragma region STATIC_THREAD
	/** main thread of RC
	* Trigerred by when a Memory R/W Tlp recvd
	* has Following functions
	* Reading/Writing from system Memory 
	* acc to TLP recieved
	* completion TLP creation
	* @return void
	**/
	void pcieRC::pcieRCThread(){
		while (1)
		{
			if (mTLPQueueMgr.isEmpty(TLPQueueType::MEM_READ_CMD_Q) && mTLPQueueMgr.isEmpty(TLPQueueType::MEM_READ_DATA_Q)\
				&& mTLPQueueMgr.isEmpty(TLPQueueType::MEM_WRITE_COMP_Q) && mTLPQueueMgr.isEmpty(TLPQueueType::MEM_WRITE_DATA_Q))
			{
				wait();
				//mArbitrateEvent.cancel_all();
			}
			else
			{
				switch (queueArbiter())
				{
			   //READ TLP for SQ 
				case TLPQueueType::MEM_READ_CMD_Q:
				{
					uint8_t data[MEM_RD_CMD_WIDTH];
					mTLPQueueMgr.popTLP(TLPQueueType::MEM_READ_CMD_Q, data);
					tMemoryReadReqTLP  iMemReadReqTlp;
					memcpy(&iMemReadReqTlp, reinterpret_cast<tMemoryReadReqTLP*>(data), MEM_RD_CMD_WIDTH);

					uint8_t* readMemData = new uint8_t[(iMemReadReqTlp.Dw0MemReadReqTLP.length / 16) * SQ_WIDTH];
					logThreadRdTlp(1, iMemReadReqTlp.address, (iMemReadReqTlp.Dw0MemReadReqTLP.length / 16) * SQ_WIDTH);

					wrapFlag = false ;
					uint16_t numOfSQ = CheckNumOfSq(iMemReadReqTlp.address, iMemReadReqTlp.Dw0MemReadReqTLP.length / 16, wrapFlag);
		
					send2Mem(readMemData, numOfSQ * SQ_WIDTH, iMemReadReqTlp.address, tlm::TLM_READ_COMMAND);

					/*if (numOfSQ < (iMemReadReqTlp.Dw0MemReadReqTLP.length / 16)){
						uint64_t currAddr = numOfSQ * SQ_WIDTH;
						numOfSQ = CheckNumOfSq(iMemReadReqTlp.address, (iMemReadReqTlp.Dw0MemReadReqTLP.length / 16) - numOfSQ);
						send2Mem(&readMemData[currAddr], (numOfSQ * SQ_WIDTH), BA_SQ0, tlm::TLM_READ_COMMAND);
					}*/

					if (wrapFlag){
						uint64_t currAddr = numOfSQ * SQ_WIDTH;
						numOfSQ =((iMemReadReqTlp.Dw0MemReadReqTLP.length / 16) - numOfSQ);
						send2Mem(&readMemData[currAddr], (numOfSQ * SQ_WIDTH), BA_SQ0, tlm::TLM_READ_COMMAND);
					}
					
					mNumCmd += (iMemReadReqTlp.Dw0MemReadReqTLP.length / 16);//for debug
					tCompletionTLP iCmplTlp = { 0 };
					createCmplTlp(readMemData, iCmplTlp, iMemReadReqTlp.Dw0MemReadReqTLP.length, iMemReadReqTlp.Dw1MemReadReqTLP.tag);
					sendTLP2Dut(reinterpret_cast<uint8_t*>(&iCmplTlp), (COMP_TLP_WIDTH + ((iMemReadReqTlp.Dw0MemReadReqTLP.length / 16)* SQ_WIDTH)));
					
					delete readMemData;
					break;
				}
				//READ TLP for DATA
				case TLPQueueType::MEM_READ_DATA_Q:
				{
					uint8_t data[MEM_RD_DATA_WIDTH];
					mTLPQueueMgr.popTLP(TLPQueueType::MEM_READ_DATA_Q, data);
					tMemoryReadReqTLP  iMemReadReqTlp;
					memcpy(&iMemReadReqTlp, reinterpret_cast<tMemoryReadReqTLP*>(data), MEM_RD_CMD_WIDTH);

					logThreadRdTlp(0, iMemReadReqTlp.address, (iMemReadReqTlp.Dw0MemReadReqTLP.length * 4));
					uint8_t* readMemData = new uint8_t[(iMemReadReqTlp.Dw0MemReadReqTLP.length * 4)];
					send2Mem(readMemData, (iMemReadReqTlp.Dw0MemReadReqTLP.length * 4), iMemReadReqTlp.address, tlm::TLM_READ_COMMAND);

					tCompletionTLP iCmplTlp = { 0 };
					createCmplTlp(readMemData, iCmplTlp, iMemReadReqTlp.Dw0MemReadReqTLP.length, iMemReadReqTlp.Dw1MemReadReqTLP.tag);
					sendTLP2Dut(reinterpret_cast<uint8_t*>(&iCmplTlp), (COMP_TLP_WIDTH + (iMemReadReqTlp.Dw0MemReadReqTLP.length * 4)));

					delete readMemData;
					break;
				}
				//Write TLP for Ccompletion queue with CQ
				case TLPQueueType::MEM_WRITE_COMP_Q:
				{
					uint8_t data[MEM_WR_COMPLETION_WIDTH];
					mTLPQueueMgr.popTLP(TLPQueueType::MEM_WRITE_COMP_Q, data);
					
					tMemoryWriteReqTLP  iMemWriteReqTlp;
					memcpy(&iMemWriteReqTlp, reinterpret_cast<tMemoryWriteReqTLP*>(data), MEM_WR_COMPLETION_WIDTH);

					logThreadWrTlp(1, iMemWriteReqTlp.address, 16);
					send2Mem(iMemWriteReqTlp.data, 16, iMemWriteReqTlp.address, tlm::TLM_WRITE_COMMAND);

					//mMemMngr.freePtr(tlpCQPayload);
					break;
				}
				//Write TLP for data
				case TLPQueueType::MEM_WRITE_DATA_Q:
				{
					uint8_t data[MEM_WR_COMPLETION_WIDTH];
					mTLPQueueMgr.popTLP(TLPQueueType::MEM_WRITE_DATA_Q, data);
					//dw0 is used only to know the length 
					tDw0MemoryTLP dw0Tlp;
					memcpy(&dw0Tlp, reinterpret_cast<tDw0MemoryTLP*>(data), sizeof(tDw0MemoryTLP));

					tMemoryWriteReqTLP  iMemWriteReqTlp;
					memcpy(&iMemWriteReqTlp, reinterpret_cast<tMemoryWriteReqTLP*>(data), MEM_WR_COMPLETION_WIDTH);

					logThreadWrTlp(0, iMemWriteReqTlp.address, (iMemWriteReqTlp.Dw0MemWriteReqTLP.length *4));
					send2Mem(iMemWriteReqTlp.data, (iMemWriteReqTlp.Dw0MemWriteReqTLP.length *4), iMemWriteReqTlp.address, tlm::TLM_WRITE_COMMAND);

					//mMemMngr.freePtr(tlpDataPayload);
					break;

				}
				default: break;
				}
			}
		}
	}
#pragma endregion

#pragma region MEM_FUNC
	/** append parameter in multi simulation
	* @param name name to be modified
	* @param mBlockSize block size
	* @param mQueueDepth queue depth of the host
	* @return void
	**/
	string pcieRC::appendParameters(string name, string format){

		char temp[64];
		name.append("_iosize");
		_itoa(mIOSize, temp, 10);
		name.append("_");
		name.append(temp);
		name.append("_blksize");
		_itoa(mBlockSize, temp, 10);
		name.append("_");
		name.append(temp);
		name.append("_qd");
		_itoa(mTbQueueDepth, temp, 10);
		name.append("_");
		name.append(temp);
		name.append(format);
		return name;
	}

	/*uint32_t pcieRC::CheckNumOfSq(uint32_t len){
		for (uint32_t sqIndex = 0; sqIndex < len; sqIndex++){
			mNSubQue->push();
			if (mNSubQue->getTail() == BA_SQ0){
				if (sqIndex == 0){
					return sqIndex + 1;
				}
				else{
					return sqIndex;
				}
				break;
			}
		}
		return len;
	}*/
	uint32_t pcieRC::CheckNumOfSq(const uint64_t &addr,const uint32_t &len,bool &wrapFlag){
		
		mNSubQue->setTail(addr);

		/*if (mNSubQue->isFull()){
			mNSubQue->push();
			return 1;
		}*/
		for (uint32_t sqIndex = 0; sqIndex < len; sqIndex++){
			mNSubQue->push();
			if (mNSubQue->getTail() == BA_SQ0){
				//sqIndex++;
				wrapFlag = true;
				return sqIndex;
			}
			//break;
		}
		return len;
	}
	void pcieRC::send2Mem(uint8_t *dataPtr, const uint32_t& length, const uint32_t& addr, tlm::tlm_command type){
	tlm::tlm_generic_payload trans;
	std::ostringstream msg;
	tlm::tlm_response_status payloadStatus;
	sc_core::sc_time delay = sc_time(0, SC_NS);

	trans.set_command(type);
	trans.set_address(addr);
	trans.set_data_ptr(dataPtr);
	trans.set_data_length(length);
	
	hostIntfTx->b_transport(trans, delay);
	payloadStatus = trans.get_response_status();
	if (payloadStatus != tlm::TLM_OK_RESPONSE)
	{
		msg << "\n PCI Test Bench: b_transport called: "
			<< "Incorrect status returned = " << payloadStatus << endl;
		REPORT_ERROR(filePCIeRC, __FUNCTION__, msg.str());
	}
	wait(delay);
}

	void pcieRC::sendTLP2Dut(uint8_t *dataPtr,const uint32_t& tlpSize){
	tlm::tlm_generic_payload trans;
	std::ostringstream msg;
	tlm::tlm_response_status payloadStatus;
	sc_core::sc_time txTime = sc_time((tlpSize * 1000 / mPciSpeed), SC_NS);
	
	trans.set_command(tlm::TLM_WRITE_COMMAND);
	//trans->set_address(address);
	trans.set_data_ptr(dataPtr);
	trans.set_data_length(tlpSize);
	cntrlIntfTx->b_transport(trans, txTime);
	payloadStatus = trans.get_response_status();

	mTxBusUtilReport->writeFile(sc_time_stamp().to_double(), txTime.to_double(), " ");
	mTxBusUtilReportCsv->writeFile(sc_time_stamp().to_double(), txTime.to_double(), ",");
	/*if (payloadStatus != tlm::TLM_OK_RESPONSE)
	{
		msg << "PCI Test Bench: b_transport called: "
			<< "Incorrect status returned = " << payloadStatus << endl;
		REPORT_ERROR(filePCIeRC, __FUNCTION__, msg.str());
	}*/
	wait(txTime);
}

	TLPQueueType pcieRC::queueArbiter(){
		if (mArbiterToken == 0)
		{
			if (!mTLPQueueMgr.isEmpty(TLPQueueType::MEM_READ_CMD_Q))
			{
				return TLPQueueType::MEM_READ_CMD_Q;
			}
			else {
				mArbiterToken++;
				if (mArbiterToken > 3)
					mArbiterToken = 0;
				return TLPQueueType::NONE;
			}
		}
		else if (mArbiterToken == 1)
		{
			if (!mTLPQueueMgr.isEmpty(TLPQueueType::MEM_READ_DATA_Q))
			{
				return TLPQueueType::MEM_READ_DATA_Q;
			}
			else  {
				mArbiterToken++;
				if (mArbiterToken > 3)
					mArbiterToken = 0;
				return TLPQueueType::NONE;
			}
		}
		else if (mArbiterToken == 2)
		{
			if (!mTLPQueueMgr.isEmpty(TLPQueueType::MEM_WRITE_COMP_Q))
			{
				return TLPQueueType::MEM_WRITE_COMP_Q;
			}
			else {
				mArbiterToken++;
				if (mArbiterToken > 3)
					mArbiterToken = 0;
				return TLPQueueType::NONE;
			}
		}
		else if (mArbiterToken == 3)
		{
			if (!mTLPQueueMgr.isEmpty(TLPQueueType::MEM_WRITE_DATA_Q))
			{
				return TLPQueueType::MEM_WRITE_DATA_Q;
			}
			else {
				mArbiterToken++;
				if (mArbiterToken > 3)
					mArbiterToken = 0;
				return TLPQueueType::NONE;
			}
		}

	}

	void pcieRC::logRcCntrl(const uint64_t &adr, const uint32_t &len){
		std::ostringstream msg;
		msg.str(" ");
		msg << "\n pcieRC::b_transport() called from Pcie Controller "
			<< "  address=0x" << hex << (uint32_t)adr
			<< "  length=" << hex << len
			<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
		mLogFileHandler << "\n pcieRC::b_transport() called from Pci Controller "
			<< "  address=0x" << hex << (uint32_t)adr
			<< "  length=" << hex << len
			<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
		REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
	}

	void pcieRC::logWrTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len){
		std::ostringstream msg;
		if (tlpType){
			msg.str("");
			msg << "\n pcieRC::MemoryWriteRequestTLP with CompletionTLP recieved "
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  length=" << hex << len
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << "\n pcieRC::MemoryWriteRequestTLP with CompletionTLP recieved "
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  length=" << hex << len
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
		else{
			msg.str("");
			msg << "\n pcieRC::MemoryWriteRequestTLP with data recieved "
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  length=" << hex << len
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << "\n pcieRC::MemoryWriteRequestTLP with data recieved "
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  length=" << hex << len
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
	}

	void pcieRC::logRdTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len){
		std::ostringstream msg;

		if (tlpType){
			msg.str("");
			msg << "\n pcieRC::MemoryReadRequestTLP recieved for cmds "
				<< "  Address=0x" << hex << (uint32_t)adr
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << "\n pcieRC::MemoryReadRequestTLP recieved "
				<< "  Address=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
		else{
			msg.str("");
			msg << "\n pcieRC::MemoryReadRequestTLP recieved for data "
				<< "  Address=0x" << hex << (uint32_t)adr
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << "\n pcieRC::MemoryReadRequestTLP recieved "
				<< "  Address=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
	}

	void pcieRC::logThreadRdTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len){
		std::ostringstream msg;
		if (tlpType){
			msg.str(" ");
			msg << "\n pcieRC::MemoryReadRequestTLP for cmd Processing "
				<< "\n pcieRC::Read SQ from system Memory "
				<< " Length=" << dec << (uint32_t)len
				<< " address=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << "\n pcieRC::MemoryReadRequestTLP for cmd Processing "
				<< "\n pcieRC::Read SQ from system Memory "
				<< " Length=" << dec << (uint32_t)len
				<< " address=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
		else{
			msg.str(" ");
			msg << " \n pcieRC::MemoryReadRequestTLP with data Processing "
				<< "\n Read Data from system Memory "
				<< " Length=" << dec << (uint32_t)(len)
				<< " address=0x" << hex << (uint32_t)adr
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << " \n pcieRC::MemoryReadRequestTLP with data Processing "
				<< "\n pcieRC::Read Data from system Memory "
				<< " Length=" << dec << (uint32_t)len
				<< " address=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
	}

	void pcieRC::logThreadWrTlp(const bool &tlpType, const uint64_t &adr, const uint32_t &len){
		std::ostringstream msg;
		if (tlpType){
			msg.str("");
			msg << " \n pcieRC::MemoryWriteRequestTLP for completion Processing "
				<< "\n pcieRC::Write to system Memory "
				<< "  Length=" << dec << (uint32_t)len
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << " \n pcieRC::MemoryWriteRequestTLP for completion Processing"
				<< "\n pcieRC::Read from system Memory "
				<< "  Length=" << dec << (uint32_t)len
				<< "  addresss=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
		else{
			msg.str("");
			msg << " \n pcieRC::MemoryWriteRequestTLP for data Processing "
				<< "\n pcieRC::Write to system Memory "
				<< "  Length=" << dec << (uint32_t)len
				<< "  address=0x" << hex << (uint32_t)adr
				<< "  @Time=" << dec << sc_time_stamp().to_double() << " ns";
			REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
			mLogFileHandler << " \n pcieRC::MemoryWriteRequestTLP data Processing"
				<< "\n pcieRC::Read from system Memory "
				<< "  Length=" << dec << (uint32_t)len
				<< "  addresss=0x" << hex << (uint32_t)adr
				<< " @Time=" << dec << sc_time_stamp().to_double() << " ns";
		}
	}
#pragma endregion

#pragma region CREATE_TLP
	void pcieRC::createConfigWriteTLP(const uint64_t& adr, const uint64_t& data, const unsigned int& len, \
										tConfigWriteReq& iConfigWriteReg){
	std::ostringstream msg;

	iConfigWriteReg.Dw0CfgWriteReqTLP.length = len;
	iConfigWriteReg.Dw0CfgWriteReqTLP.type = 0x4;
	iConfigWriteReg.Dw0CfgWriteReqTLP.format = 0x2;
	//iConfigWriteReg.Dw1CfgWriteReqTLP.firstByteEnable = 0xf;
	iConfigWriteReg.Dw1CfgWriteReqTLP.byteEnable = 0xf;
	iConfigWriteReg.address = (uint32_t)adr;
	iConfigWriteReg.data = data;
	msg.str("");
	msg << "\n pcieRC::create ConfigWriteTLP  "
		<< "  Length(in bytes)=" << dec << iConfigWriteReg.Dw0CfgWriteReqTLP.length
		<< "  Address=0x" << hex << iConfigWriteReg.address
		<< "  Data=0x"   << hex << (uint32_t)iConfigWriteReg.data
		<< "  @Time=" << dec << (sc_time_stamp().to_double()) << " ns" ;
	REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
	mLogFileHandler << "\n pcieRC::create ConfigWriteTLP  "
		<< "  Length(in bytes)=" << dec << iConfigWriteReg.Dw0CfgWriteReqTLP.length
		<< "  Address=0x" << hex << iConfigWriteReg.address
		<< "  Data=0x" << hex << (uint32_t)iConfigWriteReg.data
		<< "  @Time=" << dec << (sc_time_stamp().to_double()) << " ns";

}
	void pcieRC::createCmplTlp(uint8_t* data, tCompletionTLP& iCmplTlp, const uint32_t& length, const uint32_t& tag){
	std::ostringstream msg;
	iCmplTlp.data = data;
	iCmplTlp.Dw0CompletionTLP.length = length;
	iCmplTlp.Dw2CompletionTLP.tag = tag;
	iCmplTlp.Dw0CompletionTLP.format = 0x2;
	iCmplTlp.Dw0CompletionTLP.type = 0xa;
	iCmplTlp.Dw1CompletionTLP.completerID = 0x0100;
	iCmplTlp.Dw2CompletionTLP.requesterID = 0x0;
	msg.str("");
	msg << "\n pcieRC::create Completion TLP "
		<< "  Length=" << hex <<(uint32_t) iCmplTlp.Dw0CompletionTLP.length
		<< "  Tag=" << hex << (uint32_t)iCmplTlp.Dw2CompletionTLP.tag
		<< "  @Time=" << dec << (sc_time_stamp().to_double()) << " ns" ;
	REPORT_INFO(filePCIeRC, __FUNCTION__, msg.str());
	mLogFileHandler << "\n pcieRC::create completion TLP "
		<< "  Length=" << hex <<(uint32_t) iCmplTlp.Dw0CompletionTLP.length
		<< "  Tag=" << hex <<(uint32_t) iCmplTlp.Dw2CompletionTLP.tag
		<< "  @Time=" << dec << (sc_time_stamp().to_double()) << " ns" ;
}
#pragma endregion
