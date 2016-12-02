#include "reporting.h"
#include "TeraSMemoryChip.h"

namespace CrossbarTeraSLib {

	static const char *filename = "TeraSMemoryChip.cpp";
	
	TeraSMemoryDevice::~TeraSMemoryDevice()
	{
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++) {
		
			delete[] mPhysicalDataCache.at(chanIndex);
		}

		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		delete [] mEmulationDataCache.at(chanIndex);
		
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			delete mBegRespQueue.at(chanIndex);
			delete mBegRespReadQueue.at(chanIndex);
			delete mEndReqQueue.at(chanIndex);
			delete mEndReadReqQueue.at(chanIndex);
			delete mMemReadQueue.at(chanIndex);
			delete pChipSelect.at(chanIndex);
			delete mSendBackAckProcOpt.at(chanIndex);
			delete mSendBackReadAckProcOpt.at(chanIndex);
			delete mMemReadProcOpt.at(chanIndex);
			delete mMemAccessProcOpt.at(chanIndex);
			delete mMemAccReadProcOpt.at(chanIndex);
			delete mEndRespRxEvent.at(chanIndex);
		}
		mCacheMemory.reset();
		mLogFileHandler.close();
	}

	/** nb_transport_fw
	* non blocking interface
	* @param tlm_generic payload transaction payload
	* @param phase Phase of transaction
	* @param sc_time latency of transaction
	* @return tlm_sync_enum
	**/
	tlm::tlm_sync_enum TeraSMemoryDevice::nb_transport_fw(int id, tlm::tlm_generic_payload\
		&payload, tlm::tlm_phase &phase, sc_time& accessDelay)
	{
		if (checkAddress(payload) == false) {
			payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
			return tlm::TLM_COMPLETED;
		}

		if (payload.get_streaming_width() == payload.get_data_length()) {
			payload.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
			return tlm::TLM_COMPLETED;
		}

		if (checkDataSize(payload) == false) {
			payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
			return tlm::TLM_COMPLETED;
		}
		payload.set_response_status(tlm::TLM_OK_RESPONSE);

		tlm::tlm_sync_enum returnStatus = tlm::TLM_COMPLETED;
		std::ostringstream msg;

		switch (phase)
		{
		case (tlm::BEGIN_REQ) :
		{
								  sc_time peqDelay;
								  sc_core::sc_time currTime = sc_time_stamp();
								
								  peqDelay = accessDelay;
								  uint64_t addr = payload.get_address();
								  uint8_t code;
								  uint8_t cwBankNum;
								  uint32_t pageNum;
								  decodeAddress(addr, cwBankNum, pageNum, code);

								  /*Read Command*/
								  if (code == ONFI_READ_COMMAND)
								  {
									  bool chipSelect = pChipSelect.at(id)->read();
									  mReadChipSelect.at(id).push(chipSelect);
									  mEndReadReqQueue.at(id)->notify(payload, peqDelay);
									 // mEndReqQueue.at(id)->notify(payload, peqDelay);
								  }
								  /*Write Command*/
								  else if ( code == ONFI_WRITE_COMMAND)
								  {
									  bool chipSelect = pChipSelect.at(id)->read();
									  mWriteChipSelect.at(id).push(chipSelect);
									  mEndReqQueue.at(id)->notify(payload, peqDelay);
								  }
								  /*Bank Select Command*/
								  else if (code == ONFI_BANK_SELECT_COMMAND)
								  {
									  mMemReadQueue.at(id)->notify(payload, peqDelay);
								  }

								  returnStatus = tlm::TLM_ACCEPTED;

								  msg.str("");
								  msg << "Memory: "
									  << " BEGIN_REQ  @Time: " << dec << sc_time_stamp().to_double()
									  << " ADDRESS: " << hex << payload.get_address();
								  REPORT_INFO(filename, __FUNCTION__, msg.str());

								  accessDelay = SC_ZERO_TIME;
								  returnStatus = tlm::TLM_ACCEPTED;
								  break;
		}
		case (tlm::END_RESP) :
		{
								 mEndRespRxEvent.at(id)->notify(accessDelay);

								 msg.str("");
								 msg << "Memory: "
									 << " END_RESP : @Time" << dec << sc_time_stamp().to_double()
									 << " ADDRESS: " << hex << payload.get_address();
								 REPORT_INFO(filename, __FUNCTION__, msg.str());

								 returnStatus = tlm::TLM_COMPLETED;
								 break;
		}
		case tlm::END_REQ:
		case tlm::BEGIN_RESP:
		{
								msg.str("");
								msg << "Memory: "
									<< " Illegal phase received -- END_REQ or BEGIN_RESP";
								REPORT_FATAL(filename, __FUNCTION__, msg.str());

								returnStatus = tlm::TLM_ACCEPTED;
								break;
		}
		}//end switch
		return returnStatus;
	}


	/** Get Bank Mask
	* Generate Masks bits based on number of bits
	* @return uint32_t
	**/
	uint32_t TeraSMemoryDevice::getBankMask()
	{
		uint32_t bankMask = 0;

		for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** SC_METHOD sensitive to peq event
	* Sends back ACK of Command Receipt after
	* Data Transfer Speed
	* @ chanNum channel number
	* @return void
	**/
	void TeraSMemoryDevice::sendBackWriteACKMethod(int chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;

		tlm::tlm_phase phase = tlm::END_REQ;
		tlm::tlm_sync_enum status = tlm::TLM_ACCEPTED;
		std::ostringstream msg;
		sc_time tScheduled = SC_ZERO_TIME;
		uint8_t cwBankNum;
		uint32_t pageNum;
		
		next_trigger(mEndReqQueue.at(chanNum)->get_event());
		while ((payloadPtr = mEndReqQueue.at(chanNum)->get_next_transaction()) != NULL) {

			if (payloadPtr->is_write())
			{

				tScheduled = tScheduled + mProgramTime;

				msg.str("");
				msg << "MEMORY WRITE COMMAND Received :@TIME:"
					<< dec << sc_time_stamp().to_double()
					<< " Channel Number: " << dec << (uint32_t)chanNum
					<< " ADDRESS: " << hex << payloadPtr->get_address();
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "MEMORY WRITE COMMAND Received:: "
					<< " @TIME= " << dec << sc_time_stamp().to_double()
					<< " Channel Number: " << dec << (uint32_t)chanNum
					<< " ADDRESS: " << hex << payloadPtr->get_address()
					<< endl;

				uint64_t addr = 0x0;
				addr = payloadPtr->get_address();
				try {
					uint8_t code;
					decodeAddress(addr, cwBankNum, pageNum, code);
					
					uint8_t *dataPtr = payloadPtr->get_data_ptr();
					moveDataToEmulation(dataPtr, chanNum);

					bool chipSelect = false;

					/*Read chipSelect Port*/
					if (!mWriteChipSelect.at(chanNum).empty()) {
						chipSelect = mWriteChipSelect.at(chanNum).front();
						mWriteChipSelect.at(chanNum).pop();
					}

					/*Write data from physical cache to memory file*/
					writeMemory(cwBankNum, pageNum, chipSelect, chanNum);

				}
				catch (const char* msg)
				{
					std::cerr << msg << endl;
				}
				mTotalWrites++;

				/*Wait for Memory access time before starting BEG_RESP phase*/
				mBegRespQueue.at(chanNum)->notify(*payloadPtr, tScheduled);
				status = (*pOnfiBus.at(chanNum))->nb_transport_bw(*payloadPtr, phase, tScheduled);
			}
			
				switch (status)
				{

				case tlm::TLM_ACCEPTED:
				{
										  // more phases will follow
										  break;
				}
				case tlm::TLM_COMPLETED:
				{

										   break;
				}
				case tlm::TLM_UPDATED:
				{
										 break;
				}
				default:
				{
						  break;
				}
				}// end switch
			} //end while
	}

	/** SC_METHOD sensitive to peq event
	* Sends back Read ACK of Command Receipt after
	* Data Transfer Speed
	* @ chanNum channel number
	* @return void
	**/
	void TeraSMemoryDevice::sendBackReadACKMethod(int chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;

		tlm::tlm_phase phase = tlm::END_REQ;
		tlm::tlm_sync_enum status = tlm::TLM_ACCEPTED;
		std::ostringstream msg;
		sc_time tScheduled = SC_ZERO_TIME;
		uint8_t cwBankNum;
		uint32_t pageNum;
		
		next_trigger(mEndReadReqQueue.at(chanNum)->get_event());
		while ((payloadPtr = mEndReadReqQueue.at(chanNum)->get_next_transaction()) != NULL) {

			if (payloadPtr->is_read())
			{
				tScheduled = tScheduled + mReadTime;
				
				msg.str("");
				msg << "MEMORY READ Command Received:: "
					<<" @TIME= "<< dec << sc_time_stamp().to_double()
					<< " Channel Number: " << dec << (uint32_t)chanNum
					<< " ADDRESS: " << hex << payloadPtr->get_address();
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				mLogFileHandler << "MEMORY READ Command Received:: "
					<< " @TIME= " << dec << sc_time_stamp().to_double()
					<< " Channel Number: " << dec << (uint32_t)chanNum
					<< " ADDRESS: " << hex << payloadPtr->get_address()
				    << endl;

 				uint64_t addr = payloadPtr->get_address();
				uint8_t code;
				decodeAddress(addr, cwBankNum, pageNum, code);
			
				if (code == 0x00) {
					mBegRespReadQueue.at(chanNum)->notify(*payloadPtr, tScheduled);
					status = (*pOnfiBus.at(chanNum))->nb_transport_bw(*payloadPtr, phase, tScheduled);
				}
			}
		
			switch (status)
			{

			case tlm::TLM_ACCEPTED:
			{
								
									  break;
			}
			case tlm::TLM_COMPLETED:
			{

									   break;
			}
			case tlm::TLM_UPDATED:
			{
								
									 break;
			}
			default:
			{
					 
					   break;
			}
			}// end switch
		} //end while
	
	}
	/** SC_METHOD sensitive to peq event
	* It is triggered when Bank select command received
	* , pushes the data from data latch2 to emulation cache
	* ready to be shipped out on the ONFI channel
	* @ chanNum channel number
	* @return void
	**/
	void TeraSMemoryDevice::memReadMethod(int chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;

		tlm::tlm_phase phase = tlm::BEGIN_RESP;
		tlm::tlm_sync_enum status = tlm::TLM_ACCEPTED;
		std::ostringstream msg;
		sc_time tScheduled = SC_ZERO_TIME;
		uint8_t cwBankNum;
		uint32_t pageNum;
		next_trigger(mMemReadQueue.at(chanNum)->get_event());
		while ((payloadPtr = mMemReadQueue.at(chanNum)->get_next_transaction()) != NULL) {

			if (payloadPtr->is_read())
			{
				uint64_t addr = payloadPtr->get_address();
				uint8_t code;

				decodeAddress(addr, cwBankNum, pageNum, code);
			
				bool chipSelect = false;

				/*Read chipSelect port*/
				if (!mReadDataChipSelect.at(chanNum).empty())
				{
					chipSelect = mReadDataChipSelect.at(chanNum).front();
					mReadDataChipSelect.at(chanNum).pop();
				}

				if (code == ONFI_BANK_SELECT_COMMAND)
				{
					uint8_t bankNum;
					for (uint8_t cwIndex = 0; cwIndex < mCodeWordBankNum; cwIndex++)
					{

						bankNum = cwBankNum;
					
						copyDataToEmulationCache(cwBankNum, cwIndex, chipSelect, chanNum);
						
						/*Implement wrap around*/
						bankNum++;
						if (bankNum >= mBankNum * mNumDie)
							bankNum = 0;
						cwBankNum++;
						if (cwBankNum >= mBankNum)
							cwBankNum = 0;
					}

					uint8_t* data = payloadPtr->get_data_ptr();

					moveDataFromEmulation(data, chanNum);
					
					tScheduled = (sc_time(mReadDataTransSpeed, SC_NS));
					
					mTotalReads++;
				
					msg.str("");
					msg << "MEMORY DATA READ :@TIME:"
						<< dec << sc_time_stamp().to_double()
						<< " Channel Number: " << dec << (uint32_t)chanNum
						<< " Transaction Number: " << dec << (uint64_t)mTotalReads
						<< " ADDRESS: " << hex << payloadPtr->get_address();
					REPORT_INFO(filename, __FUNCTION__, msg.str());

					mLogFileHandler << " MEMORY DATA READ:: "
						<<" @TIME= "<< dec << sc_time_stamp().to_double()
						<< " Channel Number= " << dec << (uint32_t)chanNum
						<< " Transaction Number= " << dec << (uint64_t)mTotalReads
						<< " ADDRESS: " << hex << payloadPtr->get_address()
						<< endl;

					status = (*pOnfiBus.at(chanNum))->nb_transport_bw(*payloadPtr, phase, tScheduled);
				
				}
				
			}
			
			switch (status)
			{

			case tlm::TLM_ACCEPTED:
			{
						
									  break;
			}
			case tlm::TLM_COMPLETED:
			{

								
									   break;
			}
			case tlm::TLM_UPDATED:
			{
		
									 break;
			}
			default:
			{
					
					   break;
			}
			}// end switch
		} //end while
		
	}

	/** SC_METHOD sensitive to peq event
	* When Memory READ; it reads the memory and
	* pushes the data to the Data latch1
	* @return void
	**/
	void TeraSMemoryDevice::memAccessReadMethod(int chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;
		sc_time tScheduled = SC_ZERO_TIME;
		tlm::tlm_phase phase = tlm::BEGIN_RESP;
		uint8_t cwBankNum;
		uint32_t pageNum;
		std::ostringstream msg;
		tlm::tlm_sync_enum status = tlm::TLM_UPDATED;
		
		uint64_t addr = 0x0;
		
		next_trigger(mBegRespReadQueue.at(chanNum)->get_event());
		while ((payloadPtr = mBegRespReadQueue.at(chanNum)->get_next_transaction()) != NULL) {

			if (payloadPtr->is_read()) {                //Memory Read
				try {
					addr = payloadPtr->get_address();
					uint8_t code;
					decodeAddress(addr, cwBankNum, pageNum, code);
				
					bool chipSelect = false;
					
					/*Read chipSelect port*/
					if (!mReadChipSelect.empty())
					{
						chipSelect = mReadChipSelect.at(chanNum).front();
						mReadDataChipSelect.at(chanNum).push(chipSelect);
						mReadChipSelect.at(chanNum).pop();
					}
					readMemory(cwBankNum, pageNum, chipSelect, chanNum);
									
					tScheduled = SC_ZERO_TIME + mReadTime;
			
					msg.str("");
					msg << "Memory:READ"
						<< "  @Time: " << sc_time_stamp().to_double() 
						<< " Channel Number: " << dec << (uint32_t)chanNum
						<< " ADDRESS: "
						<< std::hex << payloadPtr->get_address();
					REPORT_INFO(filename, __FUNCTION__, msg.str());

				}//try
				catch (const char* msg)
				{
					std::cerr << msg << endl;
				}
			
			}
		}
	}

	/** SC_METHOD sensitive to peq event
	* When Memory WRITE;it writes the data to the memory
	* @return void
	**/
	void TeraSMemoryDevice::memAccessMethod(int chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;// = new tlm::tlm_generic_payload;
		sc_time tScheduled;
		std::ostringstream msg;
		uint64_t addr = 0x0;
		tlm::tlm_sync_enum status;

			next_trigger(mBegRespQueue.at(chanNum)->get_event());
			while ((payloadPtr = mBegRespQueue.at(chanNum)->get_next_transaction()) != NULL) {
				if (payloadPtr->is_write())
				{

					msg.str("");
					msg << "MEMORY WRITE COMPLETE::"
						<< "  @Time: " << sc_time_stamp().to_double()
						<< " Channel Number: " << dec << (uint32_t)chanNum
						<< " ADDRESS: "
						<< std::hex << payloadPtr->get_address();
					REPORT_INFO(filename, __FUNCTION__, msg.str());

					mLogFileHandler << "MEMORY WRITE COMPLETE::"
						<< "  @Time: " << sc_time_stamp().to_double()
						<< " Channel Number: " << dec << (uint32_t)chanNum
						<< " ADDRESS: "
						<< std::hex << payloadPtr->get_address()
						<< endl;
					tScheduled = SC_ZERO_TIME;

					tlm::tlm_phase phase = tlm::BEGIN_RESP;
					

					status = (*pOnfiBus.at(chanNum))->nb_transport_bw(*payloadPtr, phase, tScheduled);
				
				}//end else
				
				bool bailOut = false;

				switch (status)
				{
				case tlm::TLM_COMPLETED:
				{
										   break;
				}
				case tlm::TLM_ACCEPTED:
				{
										  break;
				}
				case tlm::TLM_UPDATED:
				{
                                          break;
				}
				default:
				{
			
						   break;
				}
				}// end switch
				
			} //end while
	
	}

	/** decode Address
	* Decodes Address into starting code word bank and page number
	* memAddr[3:0] - CWBank
	* memAddr[39:4] - PageNum
	* @param memAddr input address
	* @param codeWordBankNum starting CW Bank to access
	* @param pageNum Page Number in each bank to access
	* @return void
	**/
	void TeraSMemoryDevice::decodeAddress(const sc_dt::uint64 memAddr, \
		uint8_t& codeWordBankNum, uint32_t& pageNum, uint8_t& code)
	{
		if (mPageSize == 0)
		{
			throw "Invalid PageSize !";
		}
		codeWordBankNum = (uint32_t)(memAddr & getBankMask());
		pageNum = (uint32_t)((memAddr >> (mBankMaskBits +
			(uint32_t)log2(mPageSize))) & PAGE_MASK_BITS);

		code = (memAddr >> 56) & 0xff;
		std::ostringstream msg;

		/*If Code word Bank is not multiple of mCodeWordSize / mPageSize or code word bank is larger than
		 max number of banks, throw an exception*/
		if (((codeWordBankNum % (mCodeWordSize / mPageSize)) != 0x0) || (codeWordBankNum >= mBankNum))
		{
			msg.str("");
			msg << "Memory: "
				<< " Illegal bank Address: 0x" << std::hex << (uint32_t)codeWordBankNum;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			throw "Illegal bank Address generated";
		}

	}

	/** Check Address
	* Check address out of range
	* @param payload input payload
	* @return bool
	**/
	bool TeraSMemoryDevice::checkAddress(tlm::tlm_generic_payload &payload)
	{
		uint64_t memAddr = payload.get_address();
		memAddr = memAddr & 0x00ffffffffffffff;
		uint64_t memSize = mPageNum * mPageSize;
		memSize *= mBankNum;
		if (memAddr >= (memSize))
			return false;
		else
			return true;
	}

	/** Check Data Size
	* Check address out of range
	* @param payload input payload
	* @return bool
	**/
	bool TeraSMemoryDevice::checkDataSize(tlm::tlm_generic_payload &payload)
	{
		uint32_t length = payload.get_data_length();
		if (mCodeWordSize == 0)
		{
			throw "Invalid CodeWordSize! ";
		}
		if (length != mCodeWordSize)
			return false;
		else
			return true;
	}

	/** readMemory
	* Reads data from memory file
	* Transfers data to Physical data latch 1
	* @param bankNum memory bank to be accessed
	* @param pageNum Page number to be accessed
	* @param numDie Die Number based on  chip select
	* @return void
	**/
	void TeraSMemoryDevice::readMemory(uint8_t& bankNum, \
		uint32_t& pageNum, const uint8_t& numDie, const uint8_t& chanNum)
	{
		uint8_t *dataPtr = new uint8_t[mPageSize];
		uint8_t codeWordCount = mCodeWordBankNum;
		uint8_t cwBankIndex = 0;
		uint8_t bankIndex = bankNum;
		uint32_t pageIndex = pageNum;

		if (codeWordCount == 0)
		{
			throw "Invalid CodeWordSize! ";
		}
		while (codeWordCount)
		{
			/*Read Data from the file*/
			mCacheMemory->readMemory(bankIndex, pageIndex, dataPtr, numDie, chanNum);

			
			copyDataToDataLatch1(dataPtr, bankIndex, numDie, chanNum);

			/*log the transaction for house keeping*/
			mTransactionCount.at(chanNum).at(bankIndex + numDie * mBankNum)++;

			bankIndex++;

			/*implement bank and page wrap around*/
			if (bankIndex >= mBankNum) {
				bankIndex = 0;
				pageIndex++;
				if (pageIndex >= mPageNum) {
					pageIndex = 0;
					bankIndex = 0;
				}
			}
			codeWordCount--;
			cwBankIndex++;
		}

		/*Garbage Collection*/
		delete[] dataPtr;
	}

	/** Copies data from data latch to Physical Cache
	* It is called during memory read
	* @param uint8_t* dataPtr data buffer
	* @param uint8_t bankIndex bank number
	* @return void
	**/
	void TeraSMemoryDevice::copyDataToDataLatch1(uint8_t* dataPtr, \
		const uint8_t& bankIndex, const uint8_t& numDie, const uint8_t& chanNum)
	{
		if (mPageSize == 0)
		{
			throw "Invalid PageSize !";
		}
		if ((mDataLatch1Status.at(chanNum) + (bankIndex + numDie* mBankNum))->getStatus()  == status::FREE)
		{
			//memcpy(mPhysicalDataCache.at(chanNum) + (bankIndex + numDie * mBankNum) * mPageSize, dataPtr, mPageSize);
			memcpy(mPhysicalDataLatch1.at(chanNum) + (bankIndex + numDie * mBankNum) * mPageSize, dataPtr, mPageSize);
			mDL2WriteEvent.at(bankIndex + numDie * mBankNum + mNumDie * mBankNum * chanNum)->notify(SC_ZERO_TIME);
		}
	}

	/** Copies data from Physical data latch2 to Emulation Cache
	* It is called during memory read
	* @param uint8_t bankIndex bank Number
	* @param uint8_t cwBankIndex CW Bank Index
	* @return void
	**/
	void TeraSMemoryDevice::copyDataToEmulationCache(const uint8_t& bankIndex, \
		const uint8_t& cwBankIndex, const uint8_t& numDie, const uint8_t& chanNum)
	{
		if (mPageSize == 0)
		{
			throw "Invalid PageSize !";
		}

		if ((mDataLatch2Status.at(chanNum) + (bankIndex + numDie * mBankNum))->getStatus() == status::BUSY)
		{
			memcpy(mEmulationDataCache.at(chanNum) + cwBankIndex * mPageSize, mPhysicalDataLatch2.at(chanNum) + (bankIndex + numDie * mBankNum) * mPageSize, mPageSize);
			status val = status::FREE;
			(mDataLatch2Status.at(chanNum) + (bankIndex + numDie * mBankNum))->setStatus(val);
		}
		else
			next_trigger(*mDL2WriteEvent.at(bankIndex + numDie * mBankNum + mNumDie * mBankNum * chanNum));
	}

	/** writeMemory
	* Transfers data from Emulation Cache to
	* Physical data latch2
	* Transfers data from Physical data latch2 to
	* physical data latch1
	* @param bankNum memory bank to be accessed
	* @param pageNum Page number to be accessed
	* @param numDie Die Number based on  chip select
	* @return void
	**/
	void TeraSMemoryDevice::writeMemory(uint8_t& bankNum, \
		uint32_t& pageNum, const uint8_t& numDie, const uint8_t& chanNum)
	{
		uint8_t *dataPtr = new uint8_t[mPageSize];
		uint8_t codeWordCount = mCodeWordBankNum;
		uint8_t cwBankIndex = 0;
		uint8_t bankIndex = bankNum;
		uint32_t pageIndex = pageNum;

		if (codeWordCount == 0)
		{
			throw "Invalid CodeWordSize! ";
		}
		while (codeWordCount)
		{
			copyDataFromEmulationCache(cwBankIndex, bankIndex, numDie, chanNum);

			copyDataFromPhysicalCache(bankIndex, dataPtr, numDie, chanNum);

			/*Log the transaction*/
			mTransactionCount.at(chanNum).at(bankIndex + numDie * mBankNum)++;

			/*write data to the memory file*/
			mCacheMemory->writeMemory(bankIndex, pageIndex, dataPtr, numDie, chanNum);
			bankIndex++;

			/*implement wrap around*/
			if (bankIndex >= mBankNum) {
				bankIndex = 0;
				pageIndex++;
				if (pageIndex >= mPageNum) {
					pageIndex = 0;
					bankIndex = 0;
				}
			}
			codeWordCount--;
			cwBankIndex++;
		}

		/*Garbage collection*/
		delete[] dataPtr;
	}

	/** Copies data from Physical Cache to data latch
	* It is called during memory write
	* @param uint8_t bankIndex bank Number
	* @param uint8_t* dataPtr data buffer
	* @return void
	**/
	void TeraSMemoryDevice::copyDataFromPhysicalCache(const uint8_t& bankIndex, \
		uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum)
	{
		if (mPageSize == 0)
		{
			throw "Invalid PageSize !";
		}
		memcpy(dataPtr, mPhysicalDataCache.at(chanNum) + (bankIndex + numDie * mBankNum) * mPageSize, mPageSize);
	}

	/** Copies data from Emulation Cache to Physical Cache
	* It is called during memory write
	* @param uint8_t cwBankIndex CW Bank Number
	* @param uint8_t bankIndex bank Number
	* @return void
	**/
	void TeraSMemoryDevice::copyDataFromEmulationCache(const uint8_t& cwBankIndex, \
		const uint8_t& bankIndex, const uint8_t& numDie, const uint8_t& chanNum)
	{
		if (mPageSize == 0)
		{
			throw "Invalid PageSize !";
		}
		memcpy(mPhysicalDataCache.at(chanNum) + (bankIndex + numDie * mBankNum) * mPageSize, \
			mEmulationDataCache.at(chanNum) + cwBankIndex * mPageSize, mPageSize);
	}

	 
	/** moves data from emulation cache to data pointer payload
	* Data is moved from the emulation cache to a data pointer payload
	* @param uint8_t*
	* @return void
	**/
	void TeraSMemoryDevice::moveDataFromEmulation(uint8_t* dataPtr, const uint8_t& chanNum)
	{
		if (mCodeWordSize == 0)
		{
			throw "Invalid CodeWordSize !";
		}

		memcpy(dataPtr, mEmulationDataCache.at(chanNum), mCodeWordSize);

	}

	/** moves data to emulation cache from memory
	* This is for write access when data is moved from the
	* data pointer payload to emulation cache 
	* @param uint8_t*
	* @return void
	**/
	void TeraSMemoryDevice::moveDataToEmulation(uint8_t* dataPtr, const uint8_t& chanNum)
	{
		if (mCodeWordSize == 0)
		{
			throw "Invalid CodeWordSize !";
		}

		memcpy(mEmulationDataCache.at(chanNum), dataPtr, mCodeWordSize);
	
	}

	/** append parameter in multi simulation
	* @param name name to be modified
	* @param mBlockSize block size
	* @param mQueueDepth queue depth of the host
	* @return void
	**/
	string TeraSMemoryDevice::appendParameters(string name, string format){

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
		_itoa(mQueueDepth, temp, 10);
		name.append("_");
		name.append(temp);
		name.append(format);
		return name;
	}


	void TeraSMemoryDevice::start_of_simulation()
	{
	
		mLocalTime = sc_time_stamp();
		std::ostringstream msg;

		if (!mChannelReport->openFile())
		{
			msg.str("");
			msg << "Channel_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mChannelReportCsv->openFile())
		{
			msg.str("");
			msg << "Channel_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}
	}

	void TeraSMemoryDevice::end_of_simulation()
	{
		for (int chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			for (int bankIndex = 0; bankIndex < (mBankNum * mNumDie); bankIndex++)
			{

				mChannelReport->writeFile("Transaction", bankIndex, chanIndex, mTransactionCount.at(chanIndex).at(bankIndex), " ");
				mChannelReportCsv->writeFile("Transaction", bankIndex, chanIndex, mTransactionCount.at(chanIndex).at(bankIndex), ",");
			}
		}
		std::cout << name() << " ********************************************" << std::endl;
		std::cout << name() << " * " << " Memory Statistic:" << std::endl;
		std::cout << name() << " * -----------------------------------------" << std::endl;
		std::cout << name() << " *-------Device Parameters------------------" << std::endl;
		std::cout << name() << " Emulation Cache Size(Bytes):      " << std::dec << mCodeWordSize << std::endl;
		std::cout << name() << " Page Size(Bytes):                 " << std::dec << mPageSize << std::endl;
		std::cout << name() << " Number Of Pages(per Bank)(Bytes): " << std::dec << mPageNum << std::endl;
		std::cout << name() << " Number Of Banks:                  " << std::dec << (uint32_t)mBankNum << std::endl;
		std::cout << name() << " ONFI Speed:                       " << std::dec << (uint32_t)mOnfiSpeed << std::endl;
		std::cout << name() << " *------------------------------------------" << std::endl;
		std::cout << name() << " * Total Simulation Time:    " << std::dec << \
			(double)(sc_time_stamp().to_double()) << " ns" << std::endl;
		std::cout << name() << " * Bytes written: " << std::dec << (double)((double)(mTotalWrites * mCodeWordSize) / 1024) << " KB" << std::endl;
		std::cout << name() << " * Bytes read:    " << std::dec << (double)((double)(mTotalReads * mCodeWordSize) / 1024) << " KB" << std::endl;
		std::cout << name() << " ******************************************** " << std::endl << std::endl;
		mCacheMemory->closeFile();
		mChannelReport->closeFile();
		mChannelReportCsv->closeFile();
		delete mChannelReport;
		delete mChannelReportCsv;
	}

	void TeraSMemoryDevice::phyDl2WriteMethod(uint32_t dl2Index)
	{
		if (mDataLatch2Status.at(dl2Index)->getStatus() == status::FREE && mDataLatch1Status.at(dl2Index)->getStatus() == status::BUSY)
		{
			memcpy(mPhysicalDataLatch2.at(dl2Index), mPhysicalDataLatch1.at(dl2Index), mPageSize);
			status val1 = status::BUSY;
			status val2 = status::FREE;
			mDataLatch2Status.at(dl2Index)->setStatus(val1);
			mDataLatch1Status.at(dl2Index)->setStatus(val2);
		}
		else
			next_trigger(*mPhyDL2WriteEvent.at(dl2Index));
	}

	void TeraSMemoryDevice::initPhyDLParameters()
	{
		/*Data Latch2 related data structure initialization*/
		for (uint32_t dl2Index = 0; dl2Index < (mNumDie * mBankNum * mChanNum); dl2Index++)
		{
			mPhyDL2WriteEvent.push_back(new sc_event(sc_gen_unique_name("mDL2WriteEvent")));

			mPhyDL2WriteProcOpt.push_back(new sc_spawn_options());
			mPhyDL2WriteProcOpt.at(dl2Index)->spawn_method();
			mPhyDL2WriteProcOpt.at(dl2Index)->set_sensitivity(mPhyDL2WriteEvent.at(dl2Index));
			mPhyDL2WriteProcOpt.at(dl2Index)->dont_initialize();
			sc_spawn(sc_bind(&TeraSMemoryDevice::phyDl2WriteMethod, \
				this, dl2Index), sc_gen_unique_name("phyDl2WriteMethod"), mPhyDL2WriteProcOpt.at(dl2Index));
		}
	}

	void TeraSMemoryDevice::initChannelSpecificParameters()
	{
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			mTransactionCount.push_back(std::vector<uint64_t>(mNumDie *mBankNum, 0));
			pChipSelect.push_back(new sc_in<bool>(sc_gen_unique_name("pChipSelect")));
			pOnfiBus.push_back(new tlm_utils::simple_target_socket_tagged<TeraSMemoryDevice, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>(sc_gen_unique_name("pOnfiBus")));
			pOnfiBus.at(chanIndex)->register_nb_transport_fw(this, &TeraSMemoryDevice::nb_transport_fw, chanIndex);
			mEndRespRxEvent.push_back(new sc_event(sc_gen_unique_name("mEndRespRxEvent")));

			mReadChipSelect.push_back(std::queue<bool>());
			mReadDataChipSelect.push_back(std::queue<bool>());
			mWriteChipSelect.push_back(std::queue<bool>());

			try {
				/*Initialize status of the data latch to free*/
				mDataLatch1Status.push_back(new PhysicalDLStatus[mNumDie * mBankNum]);
				mDataLatch2Status.push_back(new PhysicalDLStatus[mNumDie * mBankNum]);
			}
			catch (std::bad_alloc& ba)
			{
				std::cerr << "TeraSMemoryDevice: Physical Data Latch status bad allocation" << ba.what() << std::endl;
			}
			try {
				mPhysicalDataCache.push_back(new uint8_t[mPageSize * mNumDie * mBankNum]);
				memset(mPhysicalDataCache.at(chanIndex), 0x0, mPageSize * mNumDie * mBankNum);
				mPhysicalDataLatch1.push_back(new uint8_t[mPageSize * mNumDie * mBankNum]);
				mPhysicalDataLatch2.push_back(new uint8_t[mPageSize * mNumDie * mBankNum]);
			}
			catch (std::bad_alloc& ba)
			{
				std::cerr << "TeraSMemoryDevice: Physical Data Cache bad allocation" << ba.what() << std::endl;
			}


			try {
				mEmulationDataCache.push_back(new uint8_t[mCodeWordSize]);
				memset(mEmulationDataCache.at(chanIndex), 0x0, mCodeWordSize);
			}
			catch (std::bad_alloc& ba)
			{
				std::cerr << "TeraSMemoryDevice: Emulation Data Cache bad allocation" << ba.what() << std::endl;
			}

			try {
				mEndReqQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload >(sc_gen_unique_name("mEndReqQueue")));
				mEndReadReqQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload >(sc_gen_unique_name("mEndReadReqQueue")));
				mBegRespQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload >(sc_gen_unique_name("mBegRespQueue")));
				mBegRespReadQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload >(sc_gen_unique_name("mBegRespReadQueue")));
				mMemReadQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload >(sc_gen_unique_name("mMemReadQueue")));
			}
			catch (std::bad_alloc& ba)
			{
				std::cerr << "TeraSMemoryDevice: PEQ bad allocation" << ba.what() << std::endl;
			}

			initDynamicProcess(chanIndex);
			

		}//end for(chanIndex
	}

	void TeraSMemoryDevice::initDynamicProcess(uint8_t chanIndex)
	{
		mSendBackAckProcOpt.push_back(new sc_spawn_options());
		mSendBackAckProcOpt.at(chanIndex)->spawn_method();
		mSendBackAckProcOpt.at(chanIndex)->set_sensitivity(&mEndReqQueue.at(chanIndex)->get_event());
		mSendBackAckProcOpt.at(chanIndex)->dont_initialize();
		sc_spawn(sc_bind(&TeraSMemoryDevice::sendBackWriteACKMethod, \
			this, chanIndex), sc_gen_unique_name("sendBackACKMethod"), mSendBackAckProcOpt.at(chanIndex));

		mSendBackReadAckProcOpt.push_back(new sc_spawn_options());
		mSendBackReadAckProcOpt.at(chanIndex)->spawn_method();
		mSendBackReadAckProcOpt.at(chanIndex)->set_sensitivity(&mEndReadReqQueue.at(chanIndex)->get_event());
		mSendBackReadAckProcOpt.at(chanIndex)->dont_initialize();
		sc_spawn(sc_bind(&TeraSMemoryDevice::sendBackReadACKMethod, \
			this, chanIndex), sc_gen_unique_name("sendBackReadACKMethod"), mSendBackReadAckProcOpt.at(chanIndex));

		mMemReadProcOpt.push_back(new sc_spawn_options());
		mMemReadProcOpt.at(chanIndex)->spawn_method();
		mMemReadProcOpt.at(chanIndex)->set_sensitivity(&mMemReadQueue.at(chanIndex)->get_event());
		mMemReadProcOpt.at(chanIndex)->dont_initialize();
		sc_spawn(sc_bind(&TeraSMemoryDevice::memReadMethod, \
			this, chanIndex), sc_gen_unique_name("memReadMethod"), mMemReadProcOpt.at(chanIndex));

		mMemAccessProcOpt.push_back(new sc_spawn_options());
		mMemAccessProcOpt.at(chanIndex)->set_sensitivity(&mBegRespQueue.at(chanIndex)->get_event());
		mMemAccessProcOpt.at(chanIndex)->spawn_method();
		mMemAccessProcOpt.at(chanIndex)->dont_initialize();
		sc_spawn(sc_bind(&TeraSMemoryDevice::memAccessMethod, \
			this, chanIndex), sc_gen_unique_name("memAccessMethod"), mMemAccessProcOpt.at(chanIndex));

		mMemAccReadProcOpt.push_back(new sc_spawn_options());
		mMemAccReadProcOpt.at(chanIndex)->set_sensitivity(&mBegRespReadQueue.at(chanIndex)->get_event());
		mMemAccReadProcOpt.at(chanIndex)->spawn_method();
		mMemAccReadProcOpt.at(chanIndex)->dont_initialize();
		sc_spawn(sc_bind(&TeraSMemoryDevice::memAccessReadMethod, \
			this, chanIndex), sc_gen_unique_name("memAccessReadMethod"), mMemAccReadProcOpt.at(chanIndex));
	}

	void TeraSMemoryDevice::createReportFiles()
	{
		try {
			string filename;
			if (mEnMultiSim){

				if (mIsPCIe)
				{
					filename = "./Reports_PCIe/channel_report";
				}
				else
				{
					filename = "./Reports/channel_report";
				}
				mChannelReport = new Report(appendParameters(filename, ".log"));
				mChannelReportCsv = new Report(appendParameters(filename, ".csv"));
			}
			else{
				string filenameCsv;
				if (mIsPCIe)
				{
					filename = "./Reports_PCIe/channel_report.log";
					filenameCsv = "./Reports_PCIe/channel_report.csv";
				}
				else
				{
					filename = "./Reports/channel_report.log";
					filenameCsv = "./Reports/channel_report.csv";
				}
				mChannelReport = new Report(filename);
				mChannelReportCsv = new Report(filenameCsv);
			}
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice:Bad File allocation" << ba.what() << std::endl;
		}
	}
}