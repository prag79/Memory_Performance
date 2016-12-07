#include "reporting.h"
#include "TeraSController.h"
namespace  CrossbarTeraSLib {
	
#pragma region THREADS
	/** Channel Manager Process
	* sc_spawn; Sends command to the ONFI interface per channel
	* from the queue
	* @param chanNum Channel Number
	* @return void
	**/
	void TeraSController::chanManagerProcess(uint8_t chanNum)
	{

		while (1)
		{

			switch (queueArbitration(chanNum))
			{
			case SHORT_QUEUE:
			{
								if (!mAscq.at(chanNum).isEmpty())
								{
									sc_time currTime = sc_time_stamp();
									processShortQueueCmd(chanNum);
								}

								break;
			}
			case LONG_QUEUE:
			{
							   if (!mAlcq.at(chanNum).isEmpty())
							   {
								   processLongQueueCmd(chanNum);
							   }

							   break;
			}

			}//switch

			if (mAlcq.at(chanNum).isEmpty() && mAscq.at(chanNum).isEmpty())
			{
				wait(*mAlcqNotEmptyEvent.at(chanNum) | *mAscqNotEmptyEvent.at(chanNum));
			}


		}//while(1)
	}

	/** Command Dispatcher
	* sc_spawn; dispatches command from the linked list if the
	* the active queues are not full and the corresponding banks
	* are not busy
	* @param chanNum Channel Number
	* @return void
	**/
	void TeraSController::cmdDispatcherProcess(uint8_t chanNum)
	{
		uint16_t offset = 0;

		std::ostringstream msg;
		uint8_t numBanksBusy = 0;

		while (1)
		{
			/*loops through corresponding banks linked list head, if there is no command left
			then makes the corresponding bank's status so that the dispatcher opts out of polling that 
			bank repeatedly */
			checkCmdDispatcherBankStatus(chanNum);
			
			/*If all the command dispatchers bank status is busy, it means there are no
			commands left in the HEAD of the linked list to be processed, go into wait state*/
			pollCmdDispatcherBankStatus(chanNum);
			

			for (uint16_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
			{
				mBankLinkList.getTail(chanNum, cwBankIndex, queueTail.at(chanNum).at(cwBankIndex));
				queueHead.at(chanNum).at(cwBankIndex) = mBankLinkList.getHead(chanNum, cwBankIndex);

				if (queueHead.at(chanNum).at(cwBankIndex) != -1)
				{
					// if Bank is busy then it goes to the next link list
					//if (mCwBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_FREE)//if not busy
					uint64_t cmd;
					sc_core::sc_time timeVal;
					mCmdQueue.fetchCommand(queueHead.at(chanNum).at(cwBankIndex), cmd, timeVal);
					cmdType cmdType;
					cmdType = getPhysicalCmdType(cmd);
					uint64_t queueVal;

					if (mPhyDL1Status.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_FREE && cmdType == cmdType::READ)
					{
						/*Update Head and Tail*/
						processBankLinkList(cwBankIndex, cmd, timeVal, chanNum);
						createActiveCmdEntry(cmd, queueHead.at(chanNum).at(cwBankIndex), queueVal);
						
						/*Make corresponding Bank and DL1 busy*/
						mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY; //Make it busy
						mPhyDL1Status.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
						DispatchShortQueue(chanNum, queueVal, timeVal, cwBankIndex);

						
						
					}//if (mCwBankStatus.at(chanNum).at(cwBankIndex) == false)
					else if (mPhyDL2Status.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_FREE && cmdType == cmdType::WRITE)
					{
						/*Update Head and Tail*/
						processBankLinkList(cwBankIndex, cmd, timeVal, chanNum);
						createActiveCmdEntry(cmd, queueHead.at(chanNum).at(cwBankIndex), queueVal);
						DispatchLongQueue(chanNum, queueVal, timeVal, cwBankIndex);

						/*Make corresponding Bank and DL2 busy*/
						mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
						mPhyDL2Status.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
					}
					/*else
					{
					continue;
					}*/

				}//if(!-1)

				/*else {
				continue;
				}*/
				//	wait(100, SC_NS);
			}//for

		}//while
	}

#pragma endregion
#pragma region METHODS
	/** pending Write Command process
	* @param chanNum channel Number
	* @return void
	**/
	void TeraSController::pendingWriteCmdProcess(uint8_t chanNum)
	{
		uint64_t cmd;
		cmdType ctype;
		uint16_t lba;
		uint16_t cwCnt;
		uint16_t slotNum;
		uint8_t queueAddr;

		std::ostringstream msg;

		next_trigger(mPendingCmdQueue.at(chanNum)->get_event());
		while ((cmd = mPendingCmdQueue.at(chanNum)->popCommand()) != 0)
		{
			uint32_t cwBankMask = getCwBankMask();
			uint16_t cwBankIndex = mPendingCmdQueue.at(chanNum)->getCwBankNum(cwBankMask);
			mPendingCmdQueue.at(chanNum)->erase();
			decodePendingCmdEntry(cmd, ctype, lba, cwCnt, slotNum, queueAddr);

			if (ctype == WRITE)
			{
				bool chipSelect = (bool)(lba >> mCwBankMaskBits) & 0x1;
				if ((cwBankIndex == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCodeWordSize / mPageSize)))
				{
					cwBankIndex = 0;
					chipSelect = 1;

				}
				if ((chipSelect == 1) && (mNumDie == 1))
				{
					chipSelect = 0;
				}
				if (chipSelect && (mNumDie > 1))
					cwBankIndex = cwBankIndex + (mCodeWordNum / mNumDie);


				mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
				mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);
				uint16_t cnt = mIntermediateQueue.update(cmd, mPendingLBAMaskBits);

				msg.str("");
				msg << "PENDING QUEUE PUSH: WRITE COMMAND: "
					<< "  @Time: " << dec << sc_time_stamp().to_double()
					<< " LBA =0x" << hex << (uint32_t)lba
					<< " Slot Number: " << dec << (uint32_t)slotNum
					<< " Command Payload: " << hex << (uint32_t)cmd
					<< " CwBank Index: " << hex << (uint32_t)cwBankIndex
					<< " Chip Select: " << hex << (uint32_t)chipSelect
					<< " Channel Number: " << hex << (uint32_t)chanNum;
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "PENDING QUEUE PUSH::WRITE COMMAND: "
					<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< " LBA =0x" << hex << (uint32_t)lba
					<< " Slot Number= " << dec << (uint32_t)slotNum
					<< " Command Payload= 0x" << hex << (uint32_t)cmd
					<< endl;
				if (!cnt)
				{
					cmdType cmd = mSlotTable.at(slotNum).cmd;
					uint16_t cwCnt = mSlotTable.at(slotNum).cnt;
					uint16_t value;
					mLatency.at(slotNum).endDelay = sc_time_stamp().to_double() + mNumCmdWrites * mDataTransferTime;
					mLatency.at(slotNum).latency = (mLatency.at(slotNum).endDelay - mLatency.at(slotNum).startDelay);
					mWaitStartTime.at(slotNum) = sc_time_stamp().to_double();
					createCompletionEntry(cmd, slotNum, cwCnt, value);
					mNumCmdWrites++;
					if (mMode)
					{
						mCompletionQueue_mcore.setStatus(slotNum);
					}
					else {

						mCompletionQueue.pushCommand(value);
					}
					//createCompletionEntry(cmd, slotNum, cwCnt, value);
					//mCompletionQueue.pushCommand(value);
					//mIntermediateQueue.popQueue(slotNum);

					msg.str("");
					msg << "COMPLETION QUEUE PUSH::WRITE COMMAND "
						<< "  @Time= " << dec << sc_time_stamp().to_double()
						<< " Slot Number= " << dec << (uint32_t)slotNum
						<< " Command Payload= 0x" << hex << (uint32_t)value;
					REPORT_INFO(filename, __FUNCTION__, msg.str());

					mLogFileHandler << "COMPLETION QUEUE PUSH::WRITE COMMAND "
						<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
						<< " Slot Number= " << dec << (uint32_t)slotNum
						<< " Command Payload= 0x" << hex << (uint32_t)value
						<< endl;

				}
			}//if (ctype == WRITE)
		}//while

	}

	/** pending Read Command process
	* @param chanNum channel Number
	* @return void
	**/
	void TeraSController::pendingReadCmdProcess(uint8_t chanNum)
	{
		uint64_t cmd;
		cmdType ctype;
		uint16_t lba;
		uint16_t cwCnt;
		uint16_t slotNum;
		uint8_t queueAddr;
		uint32_t pageNum;
		uint64_t cmdVal;
		std::ostringstream msg;

		next_trigger(mPendingReadCmdQueue.at(chanNum)->get_event());
		while (((cmd = mPendingReadCmdQueue.at(chanNum)->popCommand()) != 0) /*&& (!mAlcq.at(chanNum).isFull())*/)
		{
			if (!mAlcq.at(chanNum).isFull())
			{

				mPendingReadCmdQueue.at(chanNum)->erase();
				decodePendingCmdEntry(cmd, ctype, lba, cwCnt, slotNum, queueAddr);
				
				if (ctype == READ)
				{
					createActiveCmdEntry(slotNum, queueAddr, ctype, lba, cwCnt, pageNum, cmdVal);

					sc_core::sc_time delay = sc_time_stamp();
					ActiveCmdQueueData value;
					value.cmd = cmdVal;
					value.time = delay;
					mAlcq.at(chanNum).pushQueue(value);

					uint16_t cwBankIndex;

					cwBankIndex = getCwBankIndex(lba);
					
					/*If DL2 is free, then data from DL1 can be moved to DL2 , making
					DL1 free*/
					if (mPhyDL2Status.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_FREE)
					{

						/*Make DL1 status free again*/
						mPhyDL1Status.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
						mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
						mPhyDL2Status.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
						mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);
					}
					lba |= ((uint64_t)chanNum & 0x0F);
					msg.str("");
					msg << "PENDING QUEUE PUSH:: READ COMMAND: "
						<< "  @Time: " << dec << sc_time_stamp().to_double()
						<< " LBA =0x" << hex << (uint32_t)lba
						<< " Command Payload: " << hex << (uint64_t)cmdVal
						<< " Channel Number: " << dec << (uint32_t)chanNum;

					REPORT_INFO(filename, __FUNCTION__, msg.str());

					mLogFileHandler << "PENDING QUEUE PUSH::READ COMMAND: "
						<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
						<< " LBA =0x" << hex << (uint32_t)lba
						<< " Command Payload= 0x" << hex << (uint64_t)cmdVal
						<< " Channel Number= " << dec << (uint32_t)chanNum
						<< endl;
					mAlcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);

				}//if(READ)
			}
			else
			{
				next_trigger(*mAlcqNotFullEvent.at(chanNum));
				break;
			}

		}//while

	}


	/** completionMethod
	* This method is notified when running count
	* for a particular slot goes down to zero
	* This method pushes finished command to the
	* command completion queue
	* @return void
	**/
	void TeraSController::completionMethod()
	{
		std::ostringstream msg;
		uint16_t slotNum = mSlotNumQueue.front();
		mSlotNumQueue.pop();
		cmdType cmd = mSlotTable.at(slotNum).cmd;
		uint16_t cwCnt = mSlotTable.at(slotNum).cnt;
		uint16_t value;
		createCompletionEntry(cmd, slotNum, cwCnt, value);
		if (mMode)
		{
			mCompletionQueue_mcore.setStatus(slotNum);
		}
		else {

			mCompletionQueue.pushCommand(value);
		}
		// mIntermediateQueue.popQueue(numSlot);
		uint32_t addr;
		mCmdQueue.getQueueAddress(slotNum, addr);
		uint64_t dummyCmd;
		sc_core::sc_time timeVal;
		mCmdQueue.fetchCommand(addr, dummyCmd, timeVal);
		mLatency.at(slotNum).endDelay = sc_time_stamp().to_double() /*+ mNumCmdReads * mDataTransferTime*/;
		mLatency.at(slotNum).latency = (mLatency.at(slotNum).endDelay - mLatency.at(slotNum).startDelay);
		mWaitStartTime.at(slotNum) = sc_time_stamp().to_double();
		msg.str("");
		msg << "COMPLETION QUEUE PUSH::READ COMMAND: "
			<< "  @Time: " << dec << sc_time_stamp().to_double()
			/*<< " Channel Number: " << dec << (uint32_t)chanNum*/
			<< " Slot Number: " << dec << (uint32_t)slotNum
			<< " Command Payload: " << hex << (uint32_t)value;
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "COMPLETION QUEUE PUSH::READ COMMAND: "
			<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
			/*<< " Channel Number= " << dec << (uint32_t)chanNum*/
			<< " Slot Number= " << dec << (uint32_t)slotNum
			<< " Command Payload: 0x" << hex << (uint32_t)value
			<< endl;
	}

	/** checkRespProcess
	* Final Phase of transaction processing
	* @param chanNum channel number
	* @return void
	**/
	void TeraSController::checkRespProcess(uint8_t chanNum)
	{
		tlm::tlm_generic_payload *payloadPtr;

		tlm::tlm_phase phase = tlm::END_RESP;
		tlm::tlm_sync_enum status = tlm::TLM_COMPLETED;
		std::ostringstream msg;
		sc_time tScheduled = SC_ZERO_TIME;
		next_trigger(mRespQueue.at(chanNum)->get_event());
		while ((payloadPtr = mRespQueue.at(chanNum)->get_next_transaction()) != NULL)
		{

			if (payloadPtr->is_write())
			{
				(*pOnfiBus.at(chanNum))->nb_transport_fw(*payloadPtr, phase, tScheduled);
				payloadPtr->release();

				mTotalWrites++;
				msg.str("");
				msg << "WRITE COMPLETE: "
					<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
					<< " Transaction Number: " << dec << mTotalWrites;
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				mEndReadEvent.at(chanNum)->notify(SC_ZERO_TIME);
			}
			else if (payloadPtr->is_read()){


				mAvailableCredit.at(chanNum)++;
				mCreditPositiveEvent.at(chanNum)->notify(SC_ZERO_TIME);
				mEndReadEvent.at(chanNum)->notify(SC_ZERO_TIME);
				mTotalReads++;
				msg.str("");
				msg << "READ COMPLETE: "
					<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
					<< " Transaction Number: " << dec << mTotalReads;
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				tlm::tlm_sync_enum returnStatus = (*pOnfiBus.at(chanNum))->nb_transport_fw(*payloadPtr, phase, tScheduled);
				switch (returnStatus)
				{
				case tlm::TLM_ACCEPTED:
				{
										  break;
				}
				case tlm::TLM_COMPLETED:
				{
										   break;
				}
				default: //protocol error
					break;
				}//end switch

				payloadPtr->release();

			}//end else if
		}
	}
#pragma endregion

#pragma region INTERFACE_HELPER_METHODS

	void TeraSController::processPCMDQueue(uint8_t* dataPtr, sc_time& delay, uint16_t& slotNum)
	{
		std::ostringstream msg;
		uint64_t lba;

		mCmdQueue.insertCommand(slotNum, dataPtr, delay);
		sc_time currTime = sc_time_stamp();
		cmdType ctype = mCmdQueue.getCmdType(slotNum);
		mCmdType = ctype;
		string cmdType;
		if (ctype == READ)
		{
			cmdType = "READ";
			mLatency.at(slotNum).startDelay = sc_time_stamp().to_double() + delay.to_double();
		}
		else {
			cmdType = "WRITE";
		}
		uint16_t cwCount;
		mCmdQueue.getCwCount(slotNum, cwCount);
		mSlotTable.at(slotNum).cnt = cwCount;
		mSlotTable.at(slotNum).cmd = ctype;
		mCmdQueue.replicateCommand(slotNum, cwCount);
		memcpy(&lba, dataPtr, 8);
		lba = ((lba >> 9) & getLBAMask());

		msg.str("");
		msg << "COMMAND QUEUE WRITE: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " LBA= 0x" << hex << (uint64_t)lba
			<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
			<< " Command Type= " << cmdType
			<< " CW Count= " << dec << cwCount;
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "COMMAND QUEUE WRITE: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
			<< " LBA=0x" << hex << (uint64_t)lba
			<< " Command Type= " << cmdType
			<< " CW Count= " << dec << cwCount
			<< endl;
		TeraSBankLLMethod();
	}

	void TeraSController::processData(tlm::tlm_generic_payload& trans, uint8_t* dataPtr, sc_time& delay, uint16_t& slotNum)
	{
		std::ostringstream msg;
		uint32_t length = trans.get_data_length();
		if (trans.is_read())
		{
			mDataBuffer_1.readBlockData(slotNum, dataPtr, length);
			double dataTransferTime = (length * 1000) / (16 * mDDRSpeed);

			if (mNumCmdReads)
			{
				mNextReadCount++;
			}
			mLatency.at(slotNum).latency += (mNextReadCount* dataTransferTime + mECCDelay);
			mLatencyReport->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, " ");
			mLatencyReportCsv->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, ",");

			mNumCmdReads--;
			if (mNumCmdReads == 0)
			{
				mNextReadCount = 0;
			}
			mLogFileHandler << "ECC DELAY ADDED "
				<< "= " << dec << mECCDelay << " ns"
				<< endl;

			msg.str("");
			msg << "DATA BUFFER READ: "
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " Data Size= " << dec << length;
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << "DATA BUFFER READ: "
				<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " Data Size= " << dec << length
				<< endl;
			mLogFileHandler << "READ COMMAND LATENCY: "
				<< dec << (double)(mLatency.at(slotNum).latency) << " ns" << endl;
		}
		else
		{
			mLatency.at(slotNum).startDelay = sc_time_stamp().to_double();
			mDataBuffer_1.writeBlockData(slotNum, dataPtr, length);
			mDataTransferTime = (double)(length * 1000) / (double)(16 * mDDRSpeed);
			msg.str("");
			msg << "DATA BUFFER WRITE: "
				<< " @Time= " << dec << sc_time_stamp().to_double() + delay.to_double() << " ns "
				<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " Data Size= " << dec << length;
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << "DATA BUFFER WRITE: "
				<< " @Time= " << dec << sc_time_stamp().to_double() + delay.to_double() << " ns"
				<< " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " Data Size= " << dec << length
				<< endl;
		}
	}

	void TeraSController::readCompletionQueue_mode0(uint32_t& length, uint8_t* dataPtr)
	{
		std::ostringstream msg;
		msg.str("");
		msg << "COMPLETION QUEUE PING: "
			<< " @Time= " << dec << sc_time_stamp().to_double() << " ns";

		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "COMPLETION QUEUE PING:: "
			<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
			<< endl;

		uint8_t count = mCompletionQueue.getQueueCount();
		uint8_t* data = new uint8_t[length];
		memset(data, 0x00, length);
		if (count)
		{

			for (uint32_t dataIndex = 0; dataIndex < (length - 2); dataIndex += COMPLETION_WORD_SIZE)
			{
				uint16_t slotNum;
				mCompletionQueue.getSlotNum(slotNum);
				cmdType cmd;
				mCompletionQueue.getCmdType(cmd);

				mCompletionQueue.popCommand((data + dataIndex));
				mWaitEndTime.at(slotNum) = sc_time_stamp().to_double();
				mWaitTime.at(slotNum) = mWaitEndTime.at(slotNum) - mWaitStartTime.at(slotNum);
				mLatency.at(slotNum).latency += mWaitTime.at(slotNum);

				mLogFileHandler << "COMPLETION QUEUE: COMMAND WAIT TIME: "
					<< dec << (double)mWaitTime.at(slotNum) << " ns" <<
					" SLOT NUMBER: " << (uint32_t)slotNum << endl;

				if (cmd == cmdType::READ)
				{
					mNumCmdReads++;
				}
				else {
					mLogFileHandler << "WRITE COMMAND LATENCY: "
						<< dec << (double)(mLatency.at(slotNum).latency) << " ns" <<
						" SLOT NUMBER: " << (uint32_t)slotNum << endl;
					mLatencyReport->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, " ");
					mLatencyReportCsv->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, ",");
				}
				if (mCompletionQueue.isEmpty())
				{
					mCompletionQueue.resetQueueCount();
					break;
				}
			}
		}
		createPayload(data, count, length, dataPtr);

		delete[] data;
	}

	void TeraSController::readCompletionQueue_mode1(uint32_t& length, uint8_t* dataPtr, uint16_t& slotNum)
	{
		std::ostringstream msg;
		uint8_t data = (uint8_t)mCompletionQueue_mcore.getStatus(slotNum) & 0xf;
		msg.str("");
		msg << "COMPLETION QUEUE PING: "
			<< " @Time= " << dec << sc_time_stamp().to_double() << " ns";

		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "COMPLETION QUEUE PING:: "
			<< " @Time= " << dec << sc_time_stamp().to_double() << " ns"
			<< endl;
		if (data)
		{

			mWaitEndTime.at(slotNum) = sc_time_stamp().to_double();
			mWaitTime.at(slotNum) = mWaitEndTime.at(slotNum) - mWaitStartTime.at(slotNum);
			mLatency.at(slotNum).latency += mWaitTime.at(slotNum);
			if (mCmdType == cmdType::READ)
			{
				mLogFileHandler << "COMPLETION QUEUE: COMMAND WAIT TIME: "
					<< dec << (double)mWaitTime.at(slotNum) << " ns" <<
					" SLOT NUMBER: " << (uint32_t)slotNum << endl;

				msg.str("");
				msg << "COMPLETION QUEUE: COMMAND WAIT TIME: "
					<< dec << (double)mWaitTime.at(slotNum) << " ns" <<
					" SLOT NUMBER: " << (uint32_t)slotNum << endl;

				mNumCmdReads++;
			}
			else if (mCmdType == WRITE)
			{
				mLogFileHandler << "COMPLETION QUEUE: COMMAND WAIT TIME: "
					<< dec << (double)mWaitTime.at(slotNum) << " ns" <<
					" SLOT NUMBER: " << (uint32_t)slotNum << endl;

				msg.str("");
				msg << "COMPLETION QUEUE: COMMAND WAIT TIME: "
					<< dec << (double)mWaitTime.at(slotNum) << " ns" <<
					" SLOT NUMBER: " << (uint32_t)slotNum << endl;

				mLatencyReport->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, " ");
				mLatencyReportCsv->writeFile(slotNum, (double)(mLatency.at(slotNum).latency) / 1000, ",");
			}
			mCompletionQueue_mcore.resetStatus(slotNum);
		}


		createPayload(data, length, dataPtr);
	}

	/** TeraSBankLLMethod
	* Creates Linked List of command Queue
	* @return void
	**/
	void TeraSController::TeraSBankLLMethod()
	{
		int32_t addr;
		uint16_t slotNum;
		uint16_t cwCount;
		uint8_t chanNum, chipSelect;
		uint16_t cwBank;
		uint32_t pageNum;
		uint64_t genLba = 0;
		std::ostringstream msg;

		mCmdQueue.getSlotNum(slotNum);
		mCmdQueue.getCwCount(slotNum, cwCount);
		mCmdQueue.getPCMDQAddress(addr);
		for (uint32_t cmdIndex = 0; cmdIndex < cwCount; cmdIndex++)
		{
			mCmdQueue.getLBAField(addr, chanNum, cwBank, chipSelect, pageNum);
			if ((mCwBankMaskBits == 1) && (cwBank == 1) && (mBankNum <= (mCodeWordSize / mPageSize)))
			{
				cwBank = 0;
				chipSelect = 1;
			}

			if ((chipSelect == 1) && (mNumDie == 1))
			{
				chipSelect = 0;
				pageNum++;
				if (pageNum == mPageNum)
				{
					pageNum = 0;
				}
			}
			genLba = (((uint64_t)pageNum & getPageMask()) << (mCwBankMaskBits + mChanMaskBits + 1)) |
				(((uint64_t)chipSelect & 0x1) << (mCwBankMaskBits + mChanMaskBits)) | (((uint64_t)cwBank & getCwBankMask()) << mChanMaskBits) | ((uint64_t)chanNum & getChanMask());


			msg.str("");
			msg << " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " LBA =0x" << hex << (uint64_t)genLba
				<< " Channel Number= " << hex << (uint32_t)chanNum
				<< " CW Bank= " << hex << (uint32_t)cwBank
				<< " Chip Select= " << hex << (uint32_t)chipSelect
				<< " Page Address= " << hex << (uint32_t)pageNum;
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << " BANK LL Manager:: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + mDelay.to_double()) << " ns"
				<< "  SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " LBA =0x" << hex << (uint64_t)genLba
				<< " Channel Number= " << dec << (uint32_t)chanNum
				<< " CW Bank= " << dec << (uint32_t)cwBank
				<< " Chip Select= " << dec << (uint32_t)chipSelect
				<< " Page Address= 0x" << hex << (uint32_t)pageNum
				<< endl;

			if (chipSelect && (mNumDie > 1))
				cwBank = cwBank + mCodeWordNum / mNumDie;

			if (mBankLinkList.getHead(chanNum, cwBank) == -1) //Link List Empty
			{
				mBankLinkList.updateHead(chanNum, cwBank, addr);
				mBankLinkList.updateTail(chanNum, cwBank, addr);

			}
			else { //Read Tail and update the tail with the current address
				int32_t tailAddr;
				mBankLinkList.getTail(chanNum, cwBank, tailAddr);
				mCmdQueue.updateNextAddress(tailAddr, addr);
				mBankLinkList.updateTail(chanNum, cwBank, addr);
			}

			msg.str("");
			msg << " SLOT NUMBER= " << dec << (uint32_t)slotNum
				<< " LBA =0x" << hex << (uint64_t)genLba
				<< " Channel Number= " << hex << (uint32_t)chanNum
				<< " CW Bank= " << hex << (uint32_t)cwBank
				<< " Chip Select= " << hex << (uint32_t)chipSelect
				<< " Page Address= " << hex << (uint32_t)pageNum;
			REPORT_INFO(filename, __FUNCTION__, msg.str());
			addr++;
			mCmdDispatchEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}

	}

	/** decodeAddress
	* decodes DDR address into corresponding queue and slot number
	* @param address DDR Address
	* @param queueType Command Queue/Data Buffer/Completion Queue
	* @param slotNum Slot Number
	* @return void
	**/
	void TeraSController::decodeAddress(const uint64_t& address, QueueType& queueType, uint16_t& slotNum)
	{
		uint8_t rank = (uint8_t)((address >> 15) & 0x3);
		slotNum = (uint16_t)(address & 0xff);

		switch (rank)
		{
		case 0x0: queueType = PCMDQueue;
			break;
		case 0x1: queueType = CompletionQueue;
			break;
		case 0x2: queueType = DataBuffer_1;
			break;
		case 0x3: queueType = DataBuffer_2;
			break;
		}
	}
#pragma endregion

#pragma region CMD_DISPATCHER_HELPER_METHODS

	void TeraSController::DispatchLongQueue(uint8_t chanNum, uint64_t queueVal, sc_core::sc_time timeVal, uint16_t cwBankIndex)
	{
		while (mAlcq.at(chanNum).isFull())
		{
			wait(*(mAlcqNotFullEvent.at(chanNum)));
		}
		std::ostringstream msg;
		ActiveCmdQueueData value;
		value.cmd = queueVal;
		value.time = timeVal;
		uint16_t slotNum;
		cmdType ctype;
		uint64_t lba;
		uint8_t queueNum;
		uint16_t cwCnt;
		decodeActiveCommand(queueVal, slotNum, ctype, lba, queueNum, cwCnt);

		msg.str("");
		msg << "WRITE COMMAND: "
			<< " @Time: " << dec << (sc_time_stamp().to_double() + timeVal.to_double())
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Slot Number: " << dec << (uint32_t)slotNum
			<< " Queue Number: " << dec << (uint32_t)queueNum
			<< " CW Count: " << dec << (uint32_t)cwCnt
			<< " Bank Index " << hex << (uint32_t)cwBankIndex
			<< " Address: " << hex << (uint32_t)queueHead.at(chanNum).at(cwBankIndex);
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "CommandDispatcher::WRITE COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + timeVal.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Slot Number= " << dec << (uint32_t)slotNum
			<< " Queue Number= " << dec << (uint32_t)queueNum
			<< " CW Count= " << dec << (uint32_t)cwCnt
			<< " Bank Index= " << dec << (uint32_t)cwBankIndex
			<< " Address= 0x" << hex << (uint32_t)queueHead.at(chanNum).at(cwBankIndex)
			<< endl;

		mAlcq.at(chanNum).pushQueue(value);
		mAlcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
		
	}

	void TeraSController::DispatchShortQueue(uint8_t chanNum, uint64_t queueVal, sc_core::sc_time timeVal, uint16_t cwBankIndex)
	{
		while (mAscq.at(chanNum).isFull())
		{
			wait(*(mAscqNotFullEvent.at(chanNum)));
		}
		std::ostringstream msg;
		ActiveCmdQueueData value;
		value.cmd = queueVal;
		value.time = timeVal;
		uint16_t slotNum;
		cmdType ctype;
		uint64_t lba;
		uint8_t queueNum;
		uint16_t cwCnt;
		decodeActiveCommand(queueVal, slotNum, ctype, lba, queueNum, cwCnt);

		msg.str("");
		msg << "READ COMMAND: "
			<< " @Time= " << dec << sc_time_stamp().to_double() + timeVal.to_double()
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Slot Number: " << dec << (uint32_t)slotNum
			<< " Queue Number: " << dec << (uint32_t)queueNum
			<< " CW Count: " << dec << (uint32_t)cwCnt
			<< " CW Bank " << hex << (uint32_t)cwBankIndex
			<< " Address: " << dec << queueHead.at(chanNum).at(cwBankIndex);
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "CommandDispatcher::READ COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + timeVal.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Slot Number= " << dec << (uint32_t)slotNum
			<< " Queue Number= " << dec << (uint32_t)queueNum
			<< " CW Count= " << dec << (uint32_t)cwCnt
			<< " CW Bank= " << dec << (uint32_t)cwBankIndex
			<< " Address: 0x" << dec << queueHead.at(chanNum).at(cwBankIndex)
			<< endl;

		mAscq.at(chanNum).pushQueue(value);
		mAscqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
		
	}

	void TeraSController::processBankLinkList(uint16_t cwBankIndex, uint64_t& cmd, sc_core::sc_time& timeVal, uint8_t chanNum)
	{
		int32_t nextAddr;
		int32_t tailAddr;
		mBankLinkList.getTail(chanNum, cwBankIndex, tailAddr);
		mCmdQueue.fetchCommand(queueHead.at(chanNum).at(cwBankIndex), cmd, timeVal);
		mCmdQueue.fetchNextAddress(queueHead.at(chanNum).at(cwBankIndex), nextAddr);
		if (queueHead.at(chanNum).at(cwBankIndex) == tailAddr)
		{
			mBankLinkList.updateHead(chanNum, cwBankIndex, -1);
			mBankLinkList.updateTail(chanNum, cwBankIndex, -1);
		}
		else {
			mBankLinkList.updateHead(chanNum, cwBankIndex, nextAddr);
		}
	}

	/** decode active command
	* @param cmd  cmd
	* @param slotNum slot index
	* @param ctype command type
	* @param lba lba
	* @param queueNum queue index
	* @param cwCnt code word Count
	* @return void
	**/
	void TeraSController::decodeActiveCommand(const uint64_t cmd, uint16_t& slotNum, cmdType& ctype, uint64_t& lba, uint8_t& queueNum, uint16_t& cwCnt)
	{
		ctype = (cmdType)((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
		lba = (uint64_t)((cmd >> 9) & getActiveLBAMask());
		queueNum = (uint8_t)((cmd >> (mActiveLBAMaskBits + 11)) & 0xff);
		cwCnt = (uint16_t)(cmd & 0x1ff);
		slotNum = (uint8_t)((cmd >> (mActiveLBAMaskBits + 19)) & 0xff);
	}

	/** create Active command entry
	* @param cmd command payload
	* @param queueAddr queue address
	* @param queueVal active command value
	* @return void
	**/
	void TeraSController::createActiveCmdEntry(const uint64_t cmd, \
		const int32_t queueAddr, uint64_t& queueVal)
	{
		uint64_t lba = (uint64_t)((cmd >> 9) & getLBAMask());
		uint64_t activeLba = (uint64_t)((lba >> mChanMaskBits) & getActiveLBAMask());
		uint16_t cwCnt = (uint16_t)(cmd & 0x1ff);
		uint8_t cmdVal = (uint8_t)((cmd >> (mLBAMaskBits + 9)) & 0x3);
		uint8_t slotNum = (uint8_t)((cmd >> (mLBAMaskBits + 11)) & 0xff);
		uint8_t queueNum = (uint8_t)(queueAddr - ((uint32_t)(slotNum) << mAddrShifter));
		queueVal = ((uint64_t)slotNum << (mActiveLBAMaskBits + 19)) | ((uint64_t)queueNum << (mActiveLBAMaskBits + 11)) | ((uint64_t)cmdVal << (mActiveLBAMaskBits + 9)) | ((uint64_t)activeLba << 9) | (cwCnt & 0x1ff);
	}

	/** create Active command entry
	* @param slotNum slot index
	* @param queueAddr queue address
	* @param ctype command type
	* @param lba activelba
	* @param cwCnt code word count
	* @param pageNum num of pages
	* @param cmdVal command value
	* @return void
	**/
	void TeraSController::createActiveCmdEntry(const uint16_t& slotNum, const uint8_t& queueAddr, \
		const cmdType& ctype, const uint16_t& lba, const uint16_t& cwCnt, \
		const uint32_t& pageNum, uint64_t& cmdVal)
	{
		cmdVal = (((uint64_t)slotNum & 0xff) << (mActiveLBAMaskBits + 19)) | \
			(((uint64_t)queueAddr & 0xff) << (mActiveLBAMaskBits + 11)) | \
			(((uint64_t)ctype & 0x3) << (mActiveLBAMaskBits + 9)) | \
			(((uint64_t)lba | 0x0000000000000000) << 9) | (cwCnt & 0x1ff);
	}

	void TeraSController::checkCmdDispatcherBankStatus(uint8_t chanNum)
	{

		/*This loop sweeps through all the heads of the bank link list
		 if there are no commands pointed to by the head it makes the corresponding
		 Cmd Dispatcher bank status to busy, so that when the command dispatcher finds that all the 
		 banks for a particular channel are busy, it goes into wait state, preventing this thread from polling continuosly
		 thereby increasing simulation performance */
		for (uint16_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
		{
			queueHead.at(chanNum).at(cwBankIndex) = mBankLinkList.getHead(chanNum, cwBankIndex);
			if (queueHead.at(chanNum).at(cwBankIndex) == -1)
			{
				mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
			}
			else if (mCwBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_BUSY)
			{
				mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
			}
			else {
				mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
			}
		}

		
	}

	void TeraSController::pollCmdDispatcherBankStatus(uint8_t chanNum)
	{
		/*This loop checks the status of all the banks and then goes into wait state if
		all the banks are busy, it wakes up only when either the bank becomes free,indicating the bank command
		processing is done or new command is added to the linked list  */
		for (uint16_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
		{
			if (mCmdDispBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_BUSY)
			{
				if (cwBankIndex == (mCodeWordNum - 1))
				{
					wait(*(mCmdDispatchEvent.at(chanNum)) | *(mTrigCmdDispEvent.at(chanNum)));
				}
			}
			else
				break;
		}
	}
	cmdType TeraSController::getPhysicalCmdType(const uint64_t& cmd)
	{
		uint8_t ctype = (cmd >> (mLBAMaskBits + 9)) & 0x3;
		return (cmdType)ctype;
	}
	
#pragma endregion

#pragma region CHANNEL_MGR_HELPER_METHODS
	void TeraSController::processShortQueueCmd(uint8_t chanNum)
	{

		uint64_t cmd;
		ActiveCmdQueueData queueData;
		cmdType ctype;
		uint64_t lba;
		uint8_t queueNum;
		uint16_t cwCnt;
		uint16_t slotNum;
		std::ostringstream msg;
		if (mAscq.at(chanNum).isFull())
		{
			mAscqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}

		queueData = mAscq.at(chanNum).popQueue();

		if (mAscq.empty())
		{
			mAscqEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}
		cmd = queueData.cmd;
		sc_core::sc_time delay = queueData.time;
		decodeActiveCommand(cmd, slotNum, ctype, lba, queueNum, cwCnt);

		msg.str("");
		msg << "SHORT COMMAND: "
			<< " @Time " << dec << (sc_time_stamp().to_double() + delay.to_double())
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Command Payload: " << hex << cmd
			<< " Command Type :" << hex << (uint32_t)ctype
			<< " Slot Number: " << dec << (uint32_t)slotNum
			<< " Queue Number: " << dec << (uint32_t)queueNum
			<< " CodeWord Count: " << dec << cwCnt;

		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "ChannelManager::SHORT COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Command Payload: 0x" << hex << cmd
			<< " Command Type= " << dec << (uint32_t)ctype
			<< " Slot Number= " << dec << (uint32_t)slotNum
			<< " Queue Number= " << dec << (uint32_t)queueNum
			<< " CodeWord Count= " << dec << cwCnt
			<< endl;

		RRAMInterfaceMethod(cmd, chanNum, mReadData.at(chanNum), delay, ONFI_READ_COMMAND);

		uint64_t pendingCmd;
		createPendingCmdEntry(cmd, pendingCmd);

		mPendingReadCmdQueue.at(chanNum)->notify(pendingCmd, mReadTime);

	}

	void TeraSController::processLongQueueCmd(uint8_t chanNum)
	{
		uint64_t cmd;
		cmdType ctype;
		uint32_t addr;
		uint64_t lba;
		uint8_t queueNum;
		uint16_t cwCnt;
		uint16_t slotNum;
		ActiveCmdQueueData queueData;
		std::ostringstream msg;

		if (mAlcq.at(chanNum).isFull())
		{
			mAlcqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}
		queueData = mAlcq.at(chanNum).popQueue();
		if (mAlcq.empty())
		{
			mAlcqEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}

		cmd = queueData.cmd;
		decodeActiveCommand(cmd, slotNum, ctype, lba, queueNum, cwCnt);


		sc_core::sc_time delay = queueData.time;
		msg.str("");
		msg << "LONG COMMAND: "
			<< " @Time: " << dec << sc_time_stamp().to_double() + delay.to_double() << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)lba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Command Payload: " << hex << cmd
			<< " Command Type :" << hex << (uint32_t)ctype
			<< " Slot Number: " << dec << (uint32_t)slotNum
			<< " Queue Number: " << dec << (uint32_t)queueNum
			<< " CodeWord Count: " << dec << cwCnt;
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		getActiveCmdType(cmd, ctype);
		getQueueAddress(cmd, addr);

		if (ctype == WRITE)
		{
			mDataBuffer_1.readCWData(addr, mWriteData.at(chanNum));

			mLogFileHandler << "ChannelManager::LONG COMMAND:WRITE "
				<< " @Time: " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " Active LBA =0x" << hex << (uint64_t)lba
				<< " Channel Number= " << dec << (uint32_t)chanNum
				<< " Command Payload= 0x" << hex << cmd
				<< " Command Type= " << dec << (uint32_t)ctype
				<< " Slot Number= " << dec << (uint32_t)slotNum
				<< " Queue Number= " << dec << (uint32_t)queueNum
				<< " CodeWord Count= " << dec << cwCnt
				<< endl;

			sc_time currentTime = sc_time_stamp();
			RRAMInterfaceMethod(cmd, chanNum, mWriteData.at(chanNum), delay, ONFI_WRITE_COMMAND);
			uint64_t pendingCmd;
			currentTime = sc_time_stamp();
			createPendingCmdEntry(cmd, pendingCmd);
			mPendingCmdQueue.at(chanNum)->notify(pendingCmd, mProgramTime);
			currentTime = sc_time_stamp();


		}
		else {

			mLogFileHandler << "ChannelManager::LONG COMMAND:READ "
				<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
				<< " Active LBA =0x" << hex << (uint64_t)lba
				<< " Channel Number= " << dec << (uint32_t)chanNum
				<< " Command Payload= 0x" << hex << cmd
				<< " Command Type= " << dec << (uint32_t)ctype
				<< " Slot Number= " << dec << (uint32_t)slotNum
				<< " Queue Number= " << dec << (uint32_t)queueNum
				<< " CodeWord Count= " << dec << cwCnt
				<< endl;

			sc_core::sc_time mCurrTime = sc_time_stamp();
			delay = sc_time(0, SC_NS);
			RRAMInterfaceMethod(cmd, chanNum, mReadData.at(chanNum), delay, ONFI_BANK_SELECT_COMMAND);

			mOnfiChanTime.at(chanNum) = (double)((double)(mCodeWordSize * 1000) / mOnfiSpeed);

			mOnfiChanUtilReport->writeFile(sc_time_stamp().to_double(), chanNum, 0, " "/* + delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(sc_time_stamp().to_double(), chanNum, 0, ","/* + delay.to_double()*/);

			mCurrTime = sc_time_stamp();
			uint16_t cwBankIndex;
			uint32_t page;
			bool chipSelect;
			decodeLBA(cmd, cwBankIndex, chipSelect, page);
			if ((cwBankIndex == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCodeWordSize / mPageSize)))
			{
				cwBankIndex = 0;
				chipSelect = 1;
			}
			mDataBuffer_1.writeCWData(addr, mReadData.at(chanNum));
			if ((chipSelect == 1) && (mNumDie == 1))
			{
				chipSelect = 0;
				page++;
				if (page == mPageNum)
				{
					page = 0;
				}
			}
			if (chipSelect && (mNumDie > 1))
				cwBankIndex = cwBankIndex + mCodeWordNum / 2;

			//mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
			/*Data Latch 2 status is set to free*/
			mPhyDL2Status.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;
			mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);
			uint64_t pendingCmd;
			createPendingCmdEntry(cmd, pendingCmd);
			uint16_t cnt = mIntermediateQueue.update(pendingCmd, mPendingLBAMaskBits);

			//Check if running count goes down to zero
			if (!cnt)
			{
				mSlotNumQueue.push(slotNum);
				mCmdCompleteEvent.notify(1000, SC_NS);//ECC Delay
			}
		}
	}

	void TeraSController::setCmdTransferTime(const uint8_t& code, sc_core::sc_time& tDelay, sc_core::sc_time& delay, uint8_t chanNum)
	{
		sc_time currTime;
		double writeCmdDataTransSpeed = (double)(mCmdTransferTime + (double)((mCodeWordSize)* 1000) / mOnfiSpeed); //NS
		//double writeCmdTransSpeed = (double)((double)((ONFI_COMMAND_LENGTH) * 1000) / mOnfiSpeed);
		double readBankCmdTransSpeed = (double)((double)(ONFI_BANK_SELECT_CMD_LENGTH * 1000 * 2) / mOnfiSpeed); //NS
		if (code == ONFI_WRITE_COMMAND)
		{
			tDelay = sc_time(writeCmdDataTransSpeed, SC_NS) + delay;
			mOnfiChanTime.at(chanNum) = (mCmdTransferTime - delay.to_double());
			currTime = sc_time_stamp() + delay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanTime.at(chanNum) = ((writeCmdDataTransSpeed - mCmdTransferTime) - delay.to_double());
			mOnfiChanUtilReport->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		}
		else if (code == ONFI_READ_COMMAND) {
			tDelay = sc_time(mCmdTransferTime, SC_NS) + delay;
			mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());

			currTime = sc_time_stamp() + delay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		}
		else if (code == ONFI_BANK_SELECT_COMMAND) {
			tDelay = sc_time(readBankCmdTransSpeed, SC_NS);
			mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());
			double readDataTransSpeed = (double)((double)(mCodeWordSize * 1000) / mOnfiSpeed);

			currTime = sc_time_stamp() + delay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
			currTime = sc_time_stamp() + delay + tDelay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, " ");
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, ",");
		}
	}

	/** RRAM Interface
	* creates payload and send data across ONFI
	* @param cmd Active Command
	* @param chanNum Channel Number
	* @param * data  Data Pointer
	* @param delay Command latency
	* @param code ONFI Command Code
	* @return void
	**/
	void TeraSController::RRAMInterfaceMethod(uint64_t& cmd, const uint8_t& chanNum, uint8_t* data, sc_core::sc_time& delay,
		const uint8_t& code)
	{
		bool chipSelect;
		uint16_t cwBank;
		uint32_t page;
		cmdType ctype;
		tlm::tlm_generic_payload *payloadPtr;
		sc_time tDelay = SC_ZERO_TIME;


		decodeCommand(cmd, ctype, cwBank, chipSelect, page);

		payloadPtr = mTransMgr.allocate();
		payloadPtr->acquire();
		createPayload(chipSelect, ctype, cwBank, page, code, data, payloadPtr);

		double_t startTime = 0;
		if (mOnfiChanActivityStart.at(chanNum) == false)
		{
			mOnfiStartActivity.at(chanNum) = sc_time_stamp().to_double() + delay.to_double();
			mOnfiChanActivityStart.at(chanNum) = true;
			startTime = delay.to_double();
		}
		tlm::tlm_sync_enum returnStatus;
		pChipSelect.at(chanNum)->write(chipSelect);
		wait(SC_ZERO_TIME);
		
		tlm::tlm_phase phase = tlm::BEGIN_REQ;

		setCmdTransferTime(code, tDelay, delay, chanNum);
		returnStatus = (*pOnfiBus[chanNum])->nb_transport_fw(*payloadPtr, phase, tDelay);

		switch (returnStatus)
		{
		case tlm::TLM_ACCEPTED:

			if (code == ONFI_BANK_SELECT_COMMAND)
				wait(*(mEndReadEvent.at(chanNum)));
			else
				wait(*(mEndReqEvent.at(chanNum)));
			break;

		case tlm::TLM_COMPLETED:

			data = payloadPtr->get_data_ptr();

			payloadPtr->release();
			break;

		default: //protocol error
			break;
		}//end switch

		if (code == ONFI_BANK_SELECT_COMMAND)
		{
			mOnfiEndActivity.at(chanNum) = sc_time_stamp().to_double();
		}
		else {
			mOnfiEndActivity.at(chanNum) = sc_time_stamp().to_double();
		}
		startTime = 0;
	}

	/**  create generic payload
	* @param chipSelect chip select
	* @param ctype command type
	* @param cwBank code word Bank
	* @param page page
	* @param code ONFI command code
	* @param dataptr pointer to the payload
	* @return void
	**/
	void TeraSController::createPayload(bool& chipSelect, const cmdType ctype, uint16_t cwBank, \
		uint32_t page, const uint8_t& code, uint8_t* dataPtr, tlm::tlm_generic_payload* payloadPtr)
	{
		uint64_t addr = getAddress(page, cwBank, chipSelect);
		addr = ((uint64_t)code << 56) | addr;

		payloadPtr->set_address(addr);
		payloadPtr->set_data_ptr(dataPtr);
		payloadPtr->set_data_length(mCodeWordSize);
		if (ctype == WRITE)
			payloadPtr->set_command(tlm::TLM_WRITE_COMMAND);
		else
			payloadPtr->set_command(tlm::TLM_READ_COMMAND);
	}

	/**  create generic payload
	* @param data data
	* @param count
	* @param length length of data
	* @param dataptr pointer to the payload
	* @return void
	**/
	void TeraSController::createPayload(uint8_t* data, uint8_t& count, const uint32_t& length, uint8_t* dataPtr)
	{
		memcpy(dataPtr, &count, sizeof(uint8_t));
		memcpy(dataPtr + sizeof(uint8_t), (uint8_t*)data, length - 2);
	}

	void TeraSController::createPayload(uint8_t data, const uint32_t& length, uint8_t* dataPtr)
	{
		//memcpy(dataPtr, &count, sizeof(uint8_t));
		memcpy(dataPtr, (uint8_t*)&data, length);
	}

	/** QueueArbitration
	* responsible for arbitration of cmds between
	* short and long queue
	* @param chanNum channel Number
	* @return SHORT/LONG
	**/
	qType TeraSController::queueArbitration(const uint8_t& chanNum)
	{
		int64_t shortCmd = mAscq.at(chanNum).peekQueue();
		int64_t longCmd = mAlcq.at(chanNum).peekQueue();

		if (mAvailableCredit.at(chanNum) != 0)
		{
			if (shortCmd != -1)
			{
				mAvailableCredit.at(chanNum)--;
				return SHORT_QUEUE;
			}
			else if (longCmd != -1)
			{
				return LONG_QUEUE;
			}

		}
		else
		{
			if (longCmd != -1)
			{
				return LONG_QUEUE;
			}
			else {

				wait(*mAlcqNotEmptyEvent.at(chanNum));
				return LONG_QUEUE;
			}

		}

	}
#pragma endregion
	
#pragma region INTERFACE_METHOD
/** b_transport
* blocking interface
* @param tlm_generic payload transaction payload
* @param sc_time latency of transaction
* @return void
**/
	void TeraSController::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
	{
		uint8_t* dataPtr = trans.get_data_ptr();
		uint64_t addr = trans.get_address();
		uint32_t length = trans.get_data_length();
	   
		QueueType mQueueType;
		uint16_t slotNum;
		uint64_t lba;
		//cmdType type = READ;
		std::ostringstream msg;
		decodeAddress(addr, mQueueType, slotNum);
		mDelay = delay;

		switch (mQueueType)
		{
		case PCMDQueue:
		{
						  processPCMDQueue(dataPtr, delay, slotNum);
						  break;
		}
		case DataBuffer_1:
		{
							 processData(trans, dataPtr, delay, slotNum);
							 break;
		}

		case DataBuffer_2:
		{
							 if (trans.is_read())
							 {
								 mDataBuffer_2.readBlockData(slotNum, dataPtr, length);
							 }
							 else
							 {
								 mDataBuffer_2.writeBlockData(slotNum, dataPtr, length);
							 }
							 break;
		}
		case CompletionQueue:
		{
								if (mMode == 0)
								{
									readCompletionQueue_mode0(length, dataPtr);
								}
								else {
									
									readCompletionQueue_mode1(length, dataPtr, slotNum);
								}
								break;
		}
		}
		trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);
	}

	/** nb_transport_bw
	* @param int chanNum
	* @param tlm::tlm_generic_payload& generin payload
	* @param tlm::tlm_phase phase 
	* @param sc_time time
	* @return void
	**/
	tlm::tlm_sync_enum TeraSController::nb_transport_bw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
	{

		tlm::tlm_sync_enum returnStatus = tlm::TLM_COMPLETED;
		std::ostringstream msg;

		switch (phase)
		{
		case(tlm::BEGIN_RESP) :
		{
								  sc_time currTime = sc_time_stamp();
								  mRespQueue.at(id)->notify(trans, delay);
								 
								  msg.str("");
								  msg << " BEGIN_RESP: "
									  << " Channel Number: " << dec << (uint32_t)id
									  << " @Time: " << dec << sc_time_stamp().to_double() << " ns";
								  REPORT_INFO(filename, __FUNCTION__, msg.str());
								  
								  returnStatus = tlm::TLM_ACCEPTED;
								  break;
		}//end case
		case (tlm::END_REQ) :
		{
								mEndReqEvent.at(id)->notify(SC_ZERO_TIME);
								/*if (trans.is_write())
								{
									
								}
								else
								{
									mEndReqEvent.at(id)->notify(SC_ZERO_TIME);
								}*/
								returnStatus = tlm::TLM_ACCEPTED;
								break;
		}
		}//end switch
		return returnStatus;
	}

#pragma endregion
	

#pragma region HELPER_METHODS
	/** append parameter in multi simulation
	* @param name name to be modified
	* @param mBlockSize block size
	* @param mQueueDepth queue depth of the host
	* @return void
	**/
	string TeraSController::appendParameters(string name, string format){

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


	/** get queue address
	* @param cmd active cmd
	* @param addr queue address
	* @return void
	**/
	void TeraSController::getQueueAddress(const uint64_t& cmd, uint32_t& addr)
	{
		uint8_t slotNum = (uint8_t)((cmd >> (mActiveLBAMaskBits + 19)) & 0xff);
		uint8_t queueAddr = (uint8_t)((cmd >> (mActiveLBAMaskBits + 11)) & 0xff);
		uint32_t tempSlotNum = slotNum << mAddrShifter;
		addr = tempSlotNum + queueAddr;
	}

	/** create pending command entry
	* @param cmd  active cmd
	* @param cmdVal pending command value
	* @return void
	**/
	void TeraSController::createPendingCmdEntry(const uint64_t cmd, uint64_t& cmdVal)
	{
		uint64_t lba = (uint64_t)((cmd >> 9) & getActiveLBAMask());
		uint8_t tempLba = (uint8_t)(lba & getPendingLBAMask());
		uint8_t CmdVal = (uint8_t) ((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
		uint16_t slotNum = (uint16_t) ((cmd >> (mActiveLBAMaskBits + 19)) & 0xff);
		uint8_t queueAddr = (uint8_t) ((cmd >> (mActiveLBAMaskBits + 11)) & 0xff);
		uint16_t cwCnt = (uint16_t)(cmd & 0x1ff);
		cmdVal = ((uint64_t) (slotNum) << (mPendingLBAMaskBits + 19)) | ((uint64_t) (queueAddr) << (mPendingLBAMaskBits + 11)) \
			| ((uint64_t) (CmdVal) << (mPendingLBAMaskBits + 9)) | ((uint64_t) (tempLba) << 9) |(uint64_t) cwCnt;
	}

	
	/** create completion entry
	* @param cmd command type
	* @param slotNum slot index
	* @param cwCnt code word count
	* @param value completion entry value
	* @return void
	**/
	void TeraSController::createCompletionEntry(const cmdType cmd, \
		const uint16_t slotNum, const uint16_t cwCnt, uint16_t& value)
	{
		value = ((uint16_t)((bool)(cmd & 0x1)) << 15) | ((uint16_t)(slotNum & 0xff) << 7) | ((uint16_t)(cwCnt) & 0x7f);
	}

	/** decode command payload recieved
	* @param cmd command payload
	* @param slotNum slot index
	* @param cwCnt code word count
	* @param value completion entry value
	* @return void
	**/  
	void TeraSController::decodeCommand(const uint64_t cmd, cmdType& ctype, \
		uint16_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (uint16_t) ((cmd >> 9) & getCwBankMask());
		chipSelect = (bool) ((cmd >> (mCwBankMaskBits + 9)) & 0x1);
		
		page = (uint32_t)((cmd >> (mCwBankMaskBits + 10)) & getActivePageMask());
		ctype = (cmdType)((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
	}

	
	/** Get  address
	* @param uint64_t page
	* @param uint8_t bank
	* @return uint64_t
	**/
	uint64_t TeraSController::getAddress(uint32_t& page,uint16_t& bank, bool& chipSelect)
	{
		
		if ((bank == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCodeWordSize / mPageSize)))
		{
			bank = 0;
			chipSelect = 1;
		}
		
		if ((chipSelect == 1) && (mNumDie == 1))
		{
			chipSelect = 0;
			page++;
			if (page == mPageNum)
			{
				page = 0; 
			}
			
		}
		
		uint32_t bankBitShifter = 0;
		if (mCodeWordSize != mPageSize)
		{
			bankBitShifter = (uint32_t)(log2(mCodeWordSize / mPageSize));
		}
		else {
			bankBitShifter = 0;
		}
 
		uint8_t memBank = (uint8_t)((bank << bankBitShifter) & mBankMask);
		uint32_t memPage = (uint32_t)((page << ((uint32_t)(log2(mPageSize)))) & mPageMask);

		uint64_t address = ((uint64_t)(memBank) | ((uint64_t)(memPage) << mBankMaskBits));
		return address;
	}

	/** Get Bank Mask
	* Generate Masks bits based on number of bits
	* @return uint32_t
	**/
	uint32_t TeraSController::getBankMask()
	{
		uint32_t bankMask = 0;

		for (uint16_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** Get Code word Bank Mask
	* Generate code word Masks bits based on number of bits
	* @return uint32_t
	**/
	uint32_t TeraSController::getCwBankMask()
	{
		uint32_t bankMask = 0;

		for (uint16_t bitIndex = 0; bitIndex < mCwBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** Get page Mask
	* Generate code word Masks bits based on number of bits
	* @return uint32_t
	**/
	uint64_t TeraSController::getPageMask()
	{
		uint64_t pageMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < (mPageMaskBits + (uint32_t)(log2(mPageSize))); bitIndex++)
			pageMask |= ((uint64_t)0x1 << bitIndex);
		return pageMask;

	}

	/** Get chan Mask
	* Generate chan Masks bits based on number of bits
	* @return uint64_t
	**/
	uint64_t TeraSController::getChanMask()
	{
		uint64_t chanMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < (mChanMaskBits); bitIndex++)
			chanMask |= ((uint64_t)0x1 << bitIndex);
		return chanMask;

	}

	/** Get active page Mask
	* Generate active page Masks bits based on page mask bits
	* @return uint64_t
	**/
	uint64_t TeraSController::getActivePageMask()
	{
		uint64_t pageMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mPageMaskBits; bitIndex++)
			pageMask |= ((uint64_t)0x1 << bitIndex);
		return pageMask;

	}

	/** Get LBA Mask
	* Generate lba Masks bits based on number of bits
	* @return uint64_t
	**/
	uint64_t TeraSController::getLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}

	/** Get active LBA Mask
	* Generate active LBA Masks bits based on lba mask
	* @return uint64_t
	**/
	uint64_t TeraSController::getActiveLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mActiveLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}

	/** Get pending LBA Mask
	* Generate pending LBA Masks bits based on lba mask
	* @return uint64_t
	**/
	uint64_t TeraSController::getPendingLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mPendingLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}

	/** get active cmd type
	* @param cmd active cmd
	* @param cmdType cal cmd type
	* @return void
	**/
	void TeraSController::getActiveCmdType(const uint64_t& cmd, cmdType& CmdType)
	{
		CmdType = (cmdType)((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
	}

	/** decode pending cmd entry
	* @param cmd pending cmd
	* @param cmdType cmd type
	* @param lba lba
	* @param CwCnt code word count
	* @param slotNum slot index
	* @param queueAddr queue address
	* @return void
	**/
	void TeraSController::decodePendingCmdEntry(const uint64_t cmd, cmdType& ctype, \
		uint16_t& lba, uint16_t& cwCnt, uint16_t& slotNum, uint8_t& queueAddr)
	{
		slotNum = (uint16_t)((cmd >> (mPendingLBAMaskBits + 19)) & 0xff);
		queueAddr = (uint16_t)((cmd >> (mPendingLBAMaskBits + 11)) & 0xff);
		ctype = (cmdType)((cmd >> (mPendingLBAMaskBits + 9)) & 0x3);
		lba = (uint8_t) ((cmd >> 9) & getPendingLBAMask());
		cwCnt = (uint16_t)(cmd & 0x1ff);
	}

	/** decode LBA
	* @param cmd  cmd
	* @param cwbank cw bank
	* @param chipselect chip select
	* @param page page
	* @return void
	**/
	void TeraSController::decodeLBA(uint64_t cmd, uint16_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (uint16_t) ((cmd >> 9) & getCwBankMask());
		chipSelect = (bool)((cmd >> (mCwBankMaskBits + 9)) & 0x1);
		page = (uint32_t) ((cmd >> (mCwBankMaskBits + 10)) & getActivePageMask());
	}

	uint16_t TeraSController::getCwBankIndex(const uint16_t& lba)
	{
		uint16_t cwBank = (uint16_t)((lba >> 9) & getCwBankMask());
		return cwBank;
	}
#pragma endregion
	
	/** end of simulation 
	* @return void
	**/
	void TeraSController::end_of_simulation()
	{
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			mOnfiChanActivityReport->writeFile(chanIndex, mOnfiStartActivity.at(chanIndex), mOnfiEndActivity.at(chanIndex), " ");
		}
	}

	void TeraSController::initChannelSpecificParameters()
	{
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{


			mOnfiChanTime.push_back(0);
			mOnfiTotalTime.push_back(0);

			mOnfiStartActivity.push_back(0);
			mOnfiEndActivity.push_back(0);
			mOnfiChanActivityStart.push_back(false);

			mAlcq.push_back(ActiveCommandQueue(mCodeWordNum));
			mAscq.push_back(ActiveCommandQueue(mCodeWordNum));
			mAvailableCredit.push_back(mCredit);
			mCwBankStatus.push_back(std::vector<cwBankStatus>(mCodeWordNum, cwBankStatus::BANK_FREE));
			mPhyDL1Status.push_back(std::vector<cwBankStatus>(mCodeWordNum, cwBankStatus::BANK_FREE));
			mPhyDL2Status.push_back(std::vector<cwBankStatus>(mCodeWordNum, cwBankStatus::BANK_FREE));

			mCmdDispBankStatus.push_back(std::vector<cwBankStatus>(mCodeWordNum, cwBankStatus::BANK_FREE));

			pOnfiBus.push_back(new tlm_utils::simple_initiator_socket_tagged<TeraSController, ONFI_BUS_WIDTH, tlm::tlm_base_protocol_types>(sc_gen_unique_name("pOnfiBus")));
			pOnfiBus.at(chanIndex)->register_nb_transport_bw(this, &TeraSController::nb_transport_bw, chanIndex);


			mCmdDispatchEvent.push_back(new sc_event(sc_gen_unique_name("mCmdDispatchEvent")));
			mTrigCmdDispEvent.push_back(new sc_event(sc_gen_unique_name("mTrigCmdDispEvent")));
			mChanMgrEvent.push_back(new sc_event(sc_gen_unique_name("mChanMgrEvent")));
			mAlcqNotFullEvent.push_back(new sc_event(sc_gen_unique_name("mAlcqNotFullEvent")));
			mAscqNotFullEvent.push_back(new sc_event(sc_gen_unique_name("mAscqNotFullEvent")));
			mAlcqNotEmptyEvent.push_back(new sc_event(sc_gen_unique_name("mAlcqNotEmptyEvent")));
			mAlcqEmptyEvent.push_back(new sc_event_queue(sc_gen_unique_name("mAlcqEmptyEvent")));

			mAscqNotEmptyEvent.push_back(new sc_event(sc_gen_unique_name("mAscqNotEmptyEvent")));
			mAscqEmptyEvent.push_back(new sc_event_queue(sc_gen_unique_name("mAscqEmptyEvent")));

			mEndReqEvent.push_back(new sc_event(sc_gen_unique_name("mEndReqEvent")));
			mBegRespEvent.push_back(new sc_event(sc_gen_unique_name("mBegRespEvent")));
			mEndReadEvent.push_back(new sc_event(sc_gen_unique_name("mEndReadEvent")));

			mCreditPositiveEvent.push_back(new sc_event(sc_gen_unique_name("mEndReadEvent")));

			mRespQueue.push_back(new tlm_utils::peq_with_get<tlm::tlm_generic_payload>(sc_gen_unique_name("respQueueEvent")));

			pChipSelect.push_back(new sc_core::sc_out<bool>);
			mPendingCmdQueue.emplace_back(std::make_unique<PendingCmdQueueManager>(sc_gen_unique_name("mPendingQueue")));
			mPendingReadCmdQueue.emplace_back(std::make_unique<PendingCmdQueueManager>(sc_gen_unique_name("mPendingReadQueue")));

			mReadData.push_back(new uint8_t[mCodeWordSize]);
			mWriteData.push_back(new uint8_t[mCodeWordSize]);
			queueHead.push_back(std::vector<int32_t>(mCodeWordNum, -1));
			queueTail.push_back(std::vector<int32_t>(mCodeWordNum, -1));

			mCmdDispatcherProcOpt.push_back(new sc_spawn_options());

			mCmdDispatcherProcOpt.at(chanIndex)->set_sensitivity(mCmdDispatchEvent.at(chanIndex));
			mCmdDispatcherProcOpt.at(chanIndex)->set_sensitivity(mTrigCmdDispEvent.at(chanIndex));
			mCmdDispatcherProcOpt.at(chanIndex)->dont_initialize();
			sc_spawn(sc_bind(&TeraSController::cmdDispatcherProcess, \
				this, chanIndex), sc_gen_unique_name("cmdDispatcherProcess"), mCmdDispatcherProcOpt.at(chanIndex));

			mChanMgrProcOpt.push_back(new sc_spawn_options());
			mChanMgrProcOpt.at(chanIndex)->set_sensitivity(mAlcqNotEmptyEvent.at(chanIndex));
			mChanMgrProcOpt.at(chanIndex)->set_sensitivity(mAscqNotEmptyEvent.at(chanIndex));
			mChanMgrProcOpt.at(chanIndex)->dont_initialize();
			sc_spawn(sc_bind(&TeraSController::chanManagerProcess, \
				this, chanIndex), sc_gen_unique_name("chanManagerProcess"), mChanMgrProcOpt.at(chanIndex));

			mPendingCmdProcOpt.push_back(new sc_spawn_options());
			mPendingCmdProcOpt.at(chanIndex)->spawn_method();
			mPendingCmdProcOpt.at(chanIndex)->set_sensitivity(&mPendingCmdQueue.at(chanIndex)->get_event());
			mPendingCmdProcOpt.at(chanIndex)->dont_initialize();
			sc_spawn(sc_bind(&TeraSController::pendingWriteCmdProcess, \
				this, chanIndex), sc_gen_unique_name("pendingCmdProcess"), mPendingCmdProcOpt.at(chanIndex));

			mPendingReadCmdProcOpt.push_back(new sc_spawn_options());
			mPendingReadCmdProcOpt.at(chanIndex)->spawn_method();
			mPendingReadCmdProcOpt.at(chanIndex)->set_sensitivity(&mPendingReadCmdQueue.at(chanIndex)->get_event());
			mPendingReadCmdProcOpt.at(chanIndex)->dont_initialize();
			sc_spawn(sc_bind(&TeraSController::pendingReadCmdProcess, \
				this, chanIndex), sc_gen_unique_name("pendingCmdReadProcess"), mPendingReadCmdProcOpt.at(chanIndex));

			mCheckRespProcOpt.push_back(new sc_spawn_options());
			mCheckRespProcOpt.at(chanIndex)->spawn_method();
			mCheckRespProcOpt.at(chanIndex)->set_sensitivity(&mRespQueue.at(chanIndex)->get_event());
			mCheckRespProcOpt.at(chanIndex)->dont_initialize();
			sc_spawn(sc_bind(&TeraSController::checkRespProcess, \
				this, chanIndex), sc_gen_unique_name("checkRespProcess"), mCheckRespProcOpt.at(chanIndex));


		}//for
	}

	void TeraSController::createReportFiles()
	{
		try {
			if (mEnMultiSim){

				filename = "./Reports/cntrl_latency_report";
				mLatencyReport = new Report(appendParameters(filename, ".log"));
				filename = "./Reports/onfi_chan_util_report";
				mOnfiChanUtilReport = new Report(appendParameters(filename, ".log"));
				filename = "./Reports/onfi_chan_activity_report";
				mOnfiChanActivityReport = new Report(appendParameters(filename, ".log"));
				filename = "./Reports/longQueue_report";

				mLatencyReportCsv = new Report(appendParameters("./Reports/cntrl_latency_report", ".csv"));
				mOnfiChanUtilReportCsv = new Report(appendParameters("./Reports/onfi_chan_util_report", ".csv"));
				mOnfiChanActivityReportCsv = new Report(appendParameters("./Reports/onfi_chan_activity_report", ".csv"));
			}
			else{

				mLatencyReport = new Report("./Reports/cntrl_latency_report.log");
				mOnfiChanUtilReport = new Report("./Reports/onfi_chan_util_report.log");
				mOnfiChanActivityReport = new Report("./Reports/onfi_chan_activity_report.log");

				mLatencyReportCsv = new Report("./Reports/cntrl_latency_report.csv");
				mOnfiChanUtilReportCsv = new Report("./Reports/onfi_chan_util_report.csv");
				mOnfiChanActivityReportCsv = new Report("./Reports/onfi_chan_activity_report.csv");
			}
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "TeraSMemoryDevice:Bad File allocation" << ba.what() << std::endl;
		}

		if (!mOnfiChanActivityReport->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "ONFI_CHAN_ACTIVITY_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mLatencyReport->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "LATENCY_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mOnfiChanUtilReport->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "ONFI_CHANNEL_UTILIZATION_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mOnfiChanActivityReportCsv->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "ONFI_CHAN_ACTIVITY_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mLatencyReportCsv->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "LATENCY_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}

		if (!mOnfiChanUtilReportCsv->openFile())
		{
			std::ostringstream msg;
			msg.str("");
			msg << "ONFI_CHANNEL_UTILIZATION_REPORT LOG: "
				<< " ERROR OPENING FILE" << std::endl;
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			exit(EXIT_FAILURE);
		}
	}

	void TeraSController::initMasks()
	{
		mLBAMaskBits = mPageMaskBits + mCwBankMaskBits + mChanMaskBits + 1;
		mActiveLBAMaskBits = mPageMaskBits + mCwBankMaskBits + 1;
		mPendingLBAMaskBits = mCwBankMaskBits + 1;

		mBankMask = getBankMask();
		mCwBankMask = getCwBankMask();
		mActiveLBAMask = getActiveLBAMask();
		mActivePageMask = getActivePageMask();
		mChanMask = getChanMask();
		mLBAMask = getLBAMask();
		mPendingLBAMask = getPendingLBAMask();
	}
}
