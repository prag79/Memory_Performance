#include "reporting.h"
#include "TeraSController.h"
namespace  CrossbarTeraSLib {
	
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
						  mCmdQueue.insertCommand(slotNum,dataPtr, delay);
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
							  << " LBA=0x" << hex << (uint64_t)lba
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
						  break;
		}
		case DataBuffer_1:
		{
							 if (trans.is_read())
							 {
								 mDataBuffer_1.readBlockData(slotNum, dataPtr, length);
								 double dataTransferTime = (length * 1000) / (16*  mDDRSpeed);
							
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
									 << " @Time= " << dec << sc_time_stamp().to_double() + delay.to_double()<< " ns"
									 << " SLOT NUMBER= " << dec << (uint32_t)slotNum
									 << " Data Size= " << dec << length
									 << endl;
							 }
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
								else {
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
										else if(mCmdType == WRITE)
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
								break;
		}
		}
		trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);
	}

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

	/** decodeAddress
	* decodes DDR address into corresponding queue and slot number
	* @param address DDR Address
	* @param queueType Command Queue/Data Buffer/Completion Queue
	* @param slotNum Slot Number
	* @return void
	**/
	void TeraSController::decodeAddress(const uint64_t& address, QueueType& queueType, uint16_t& slotNum)
	{
		uint8_t rank = (uint8_t) ((address >> 15) & 0x3);
		slotNum = (uint16_t) (address & 0xff);

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

	/** TeraSBankLLMethod
	* Creates Linked List of command Queue
	* @return void
	**/
	void TeraSController::TeraSBankLLMethod()
	{
		int32_t addr;
		uint16_t slotNum;
		uint16_t cwCount;
		uint8_t chanNum, cwBank, chipSelect;
		uint32_t pageNum;
		uint64_t genLba=0;
		std::ostringstream msg;

		mCmdQueue.getSlotNum(slotNum);
		mCmdQueue.getCwCount(slotNum, cwCount);
		mCmdQueue.getPCMDQAddress(addr);
		for (uint32_t cmdIndex = 0; cmdIndex < cwCount; cmdIndex++)
		{
			mCmdQueue.getLBAField(addr, chanNum, cwBank, chipSelect, pageNum);
			if ((mCwBankMaskBits == 1) && (cwBank == 1) && (mBankNum <= (mCodeWordSize/mPageSize)))
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
			msg << " SLOT NUMBER= " <<dec<< (uint32_t) slotNum
				<< " LBA =0x" << hex << (uint64_t)genLba
				<< " Channel Number= " << hex << (uint32_t)chanNum
				<< " CW Bank= " << hex << (uint32_t)cwBank
				<< " Chip Select= " << hex << (uint32_t)chipSelect
				<< " Page Address= " << hex << (uint32_t)pageNum;
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << " BANK LL Manager:: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + mDelay.to_double()) << " ns"
				<<"  SLOT NUMBER= " << dec<<(uint32_t)slotNum
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
			addr++;
			mCmdDispatchEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}
		
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
			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
			{
				queueHead.at(chanNum).at(cwBankIndex) = mBankLinkList.getHead(chanNum, cwBankIndex);
				if (queueHead.at(chanNum).at(cwBankIndex) == -1)
				{
					mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BUSY;
				}
				else if (mCwBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BUSY)
				{
					mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BUSY;
				}
				else {
					mCmdDispBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::FREE;
				}
			}

			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
			{
				if (mCmdDispBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BUSY)
				{
					if (cwBankIndex == (mCodeWordNum - 1))
					{
						wait(*(mCmdDispatchEvent.at(chanNum)) | *(mTrigCmdDispEvent.at(chanNum)));
					}
				}
				else
					break;
			}
			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
			{
				mBankLinkList.getTail(chanNum, cwBankIndex, queueTail.at(chanNum).at(cwBankIndex));
				if (queueHead.at(chanNum).at(cwBankIndex) != -1)
				{
					// if Bank is busy then it goes to the next link list
					if (mCwBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::FREE)//if not busy
					{
						uint64_t cmd;
						int32_t nextAddr;
						cmdType commandType;
						uint64_t queueVal;
						sc_core::sc_time timeVal;

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
						
						createActiveCmdEntry(cmd, queueHead.at(chanNum).at(cwBankIndex), queueVal);
						getActiveCmdType(queueVal, commandType);

						if (commandType == READ) // In case of Read
						{
							while (mAscq.at(chanNum).isFull())
							{
								wait(*(mAscqNotFullEvent.at(chanNum)));
							}
							
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
								mShortQueueSize.at(chanNum) = (uint16_t)mAscq.at(chanNum).getSize();
								mShortQueueReport->writeFile("SHORT", sc_time_stamp().to_double() + timeVal.to_double(), chanNum, mShortQueueSize.at(chanNum), " ");
								mShortQueueReportCsv->writeFile("SHORT", sc_time_stamp().to_double() + timeVal.to_double(), chanNum, mShortQueueSize.at(chanNum), ",");
								mAscqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
								mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BUSY; //Make it busy
		
						
						}
						else {  //In case of Write

							while (mAlcq.at(chanNum).isFull())
							{
								wait(*(mAlcqNotFullEvent.at(chanNum)));
							}
							
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
								mLongQueueSize.at(chanNum) = (uint16_t)mAlcq.at(chanNum).getSize();
								mLongQueueReport->writeFile("LONG", sc_time_stamp().to_double() + timeVal.to_double(), chanNum, mLongQueueSize.at(chanNum), " ");
								mLongQueueReportCsv->writeFile("LONG", sc_time_stamp().to_double() + timeVal.to_double(), chanNum, mLongQueueSize.at(chanNum), ",");
								mAlcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);
								mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BUSY;
						
						}
					}//if (mCwBankStatus.at(chanNum).at(cwBankIndex) == false)
					else
					{
						continue;
					}
					
				}//if(!-1)
				else {
					continue;
				}
			}//for
			
		}//while
	}

	/** Channel Manager Process
	* sc_spawn; Sends command to the ONFI interface per channel
	* from the queue
	* @param chanNum Channel Number
	* @return void
	**/
	void TeraSController::chanManagerProcess(uint8_t chanNum)
	{
		std::ostringstream msg;
		while (1)
		{
			//wait till there is a command in either of the active 
			// queue
 			if (mAscq.at(chanNum).isEmpty() && mAlcq.at(chanNum).isEmpty())
			{
 				wait(*mAlcqNotEmptyEvent.at(chanNum) | *mAscqNotEmptyEvent.at(chanNum));
			}
			else {
				switch (queueArbitration(chanNum))
				{
				case SHORT:
				{
							  uint64_t cmd;
							  ActiveCmdQueueData queueData;
							  cmdType ctype;
							  uint64_t lba;
							  uint8_t queueNum;
							  uint16_t cwCnt;
							  uint16_t slotNum;

							  if (mAscq.at(chanNum).isFull())
							  {
						  		  mAscqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
							  }
							  queueData = mAscq.at(chanNum).popQueue();
							  mShortQueueSize.at(chanNum) = (uint16_t)mAscq.at(chanNum).getSize();
							  
							  cmd = queueData.cmd;
							  sc_core::sc_time delay = queueData.time;
							  decodeActiveCommand(cmd, slotNum, ctype, lba, queueNum, cwCnt);
							 
							  mShortQueueReport->writeFile("SHORT", sc_time_stamp().to_double() + delay.to_double(), chanNum, mShortQueueSize.at(chanNum), " ");
							  mShortQueueReportCsv->writeFile("SHORT", sc_time_stamp().to_double() + delay.to_double(), chanNum, mShortQueueSize.at(chanNum), ",");

							  msg.str("");
							  msg << "SHORT COMMAND: "
								  << " @Time " << dec << (sc_time_stamp().to_double() + delay.to_double())
								  << " Active LBA =0x" << hex << (uint64_t)lba
								  << " Channel Number: " << dec << (uint32_t)chanNum
								  << " Command Payload: " << hex << cmd
								  << " Command Type :" << hex << (uint32_t) ctype
								  << " Slot Number: " << dec <<(uint32_t) slotNum
								  << " Queue Number: " << dec << (uint32_t) queueNum
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

							  mOnfiCode.at(chanNum) = ONFI_READ_COMMAND;
							
							  RRAMInterfaceMethod(cmd, chanNum, mReadData.at(chanNum), delay , ONFI_READ_COMMAND);
							
							  uint32_t pendingCmd;
							  createPendingCmdEntry(cmd, pendingCmd);
							  
							  mPendingReadCmdQueue.at(chanNum)->notify(pendingCmd, mReadTime); 
							  break;
				}
				case LONG:
				{
							 uint64_t cmd;
							 cmdType ctype;
							 uint32_t addr;
							 uint64_t lba;
							 uint8_t queueNum;
							 uint16_t cwCnt;
							 uint16_t slotNum;
							 ActiveCmdQueueData queueData;

							 if (mAlcq.at(chanNum).isFull())
							 {
								 mAlcqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
							 }
							 queueData = mAlcq.at(chanNum).popQueue();
							 mLongQueueSize.at(chanNum) = (uint16_t)mAlcq.at(chanNum).getSize();
							 
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
								 mOnfiCode.at(chanNum) = ONFI_WRITE_COMMAND;
								 mLongQueueReport->writeFile("LONG", sc_time_stamp().to_double() + delay.to_double(), chanNum, mLongQueueSize.at(chanNum), " ");
								 mLongQueueReportCsv->writeFile("LONG", sc_time_stamp().to_double() + delay.to_double(), chanNum, mLongQueueSize.at(chanNum), ",");
								 sc_time currentTime = sc_time_stamp();
								 RRAMInterfaceMethod(cmd, chanNum, mWriteData.at(chanNum), delay, ONFI_WRITE_COMMAND);
								 currentTime = sc_time_stamp();
								 uint32_t pendingCmd;
								 createPendingCmdEntry(cmd, pendingCmd);
								 mPendingCmdQueue.at(chanNum)->notify(pendingCmd, mProgramTime);
							 }
							 else {
					
								 mLogFileHandler << "ChannelManager::LONG COMMAND:READ "
									 << " @Time: " << dec << sc_time_stamp().to_double()<< " ns"
									 << " Active LBA =0x" << hex << (uint64_t)lba
									 << " Channel Number= " << dec << (uint32_t)chanNum
									 << " Command Payload= 0x" << hex << cmd
									 << " Command Type= " << dec << (uint32_t)ctype
									 << " Slot Number= " << dec << (uint32_t)slotNum
									 << " Queue Number= " << dec << (uint32_t)queueNum
									 << " CodeWord Count= " << dec << cwCnt
									 << endl;

								 mOnfiCode.at(chanNum) = ONFI_BANK_SELECT_COMMAND;
								 sc_core::sc_time mCurrTime = sc_time_stamp();
								 delay = sc_time(0, SC_NS);
								 RRAMInterfaceMethod(cmd, chanNum, mReadData.at(chanNum), delay, ONFI_BANK_SELECT_COMMAND);
								
								 mOnfiChanTime.at(chanNum) = (double)((double)(mCodeWordSize * 1000) / mOnfiSpeed);
								 
								 mOnfiChanUtilReport->writeFile(sc_time_stamp().to_double(), chanNum, 0, " "/* + delay.to_double()*/);
								 mOnfiChanUtilReportCsv->writeFile(sc_time_stamp().to_double(), chanNum, 0, ","/* + delay.to_double()*/);
								 	 
								 mCurrTime = sc_time_stamp();
								 uint8_t cwBankIndex;
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
									 cwBankIndex = cwBankIndex + mCodeWordNum/2;

								 mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::FREE;
								 mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);

								 uint32_t pendingCmd;
								 createPendingCmdEntry(cmd, pendingCmd);
								 mIntermediateQueue.update(pendingCmd, mPendingLBAMaskBits);
								 uint16_t cnt = mIntermediateQueue.getCount();
								
								 //Check if running count goes down to zero
								 if (!cnt)
								 {
									 mSlotNumQueue.push(slotNum);
									 mCmdCompleteEvent.notify(1000, SC_NS);//ECC Delay
								 }
							 }
							 break;
				}
				case NONE:
				{
						
				}
					break;

				}
			}//else
		}//while(1)
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
								if (trans.is_write())
								{
									mEndReqEvent.at(id)->notify(SC_ZERO_TIME);
								}
								else
								{
									mEndReqEvent.at(id)->notify(SC_ZERO_TIME);
								}
								returnStatus = tlm::TLM_ACCEPTED;
								break;
		}
		}//end switch
		return returnStatus;
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
			}
			else if (payloadPtr->is_read()){

				if ( (mOnfiCode.at(chanNum) == ONFI_BANK_SELECT_COMMAND))
				{
					mOnfiCode.at(chanNum) = 0x0;
					/*if (mAvailableCredit.at(chanNum) == 0)
					{*/
						mAvailableCredit.at(chanNum)++;
						mCreditPositiveEvent.at(chanNum)->notify(SC_ZERO_TIME);
					/*}*/
					mEndReadEvent.at(chanNum)->notify(SC_ZERO_TIME);
				}
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
		uint8_t cwBank;
		uint32_t page;
		cmdType ctype;
		tlm::tlm_generic_payload *payloadPtr;
		sc_time tDelay = SC_ZERO_TIME;
		
		double writeCmdDataTransSpeed = (double)( mCmdTransferTime + (double)((mCodeWordSize) * 1000) / mOnfiSpeed); //NS
		//double writeCmdTransSpeed = (double)((double)((ONFI_COMMAND_LENGTH) * 1000) / mOnfiSpeed);
		double readBankCmdTransSpeed = (double)((double)(ONFI_BANK_SELECT_CMD_LENGTH * 1000 * 2) / mOnfiSpeed); //NS
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
		mReqInProgressPtr = payloadPtr;
		tlm::tlm_phase phase = tlm::BEGIN_REQ;

		sc_time currTime;
		if (code == ONFI_WRITE_COMMAND)
		{
			tDelay = sc_time(writeCmdDataTransSpeed, SC_NS) + delay ;
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
			tDelay = sc_time(readBankCmdTransSpeed, SC_NS) ;
			mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());
			double readDataTransSpeed = (double)((double)(mCodeWordSize * 1000) / mOnfiSpeed);
			
			currTime = sc_time_stamp() + delay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
			currTime = sc_time_stamp() + delay + tDelay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, " ");
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, ",");
		}
		
		returnStatus = (*pOnfiBus[chanNum])->nb_transport_fw(*payloadPtr, phase, tDelay);

		switch (returnStatus)
		{
		case tlm::TLM_ACCEPTED:
			if (code == ONFI_BANK_SELECT_COMMAND)
			{
				
				wait(*(mEndReadEvent.at(chanNum)));
			}
          	
			else {
				wait(*(mEndReqEvent.at(chanNum)));
				
			}
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
		uint64_t activeLba = (uint64_t) ((lba >> mChanMaskBits) & getActiveLBAMask());
		uint16_t cwCnt = (uint16_t) (cmd & 0x1ff);
		uint8_t cmdVal = (uint8_t) ((cmd >> (mLBAMaskBits + 9)) & 0x3);
		uint8_t slotNum = (uint8_t) ((cmd >> (mLBAMaskBits + 11)) & 0xff);
		uint8_t queueNum = (uint8_t)(queueAddr - ((uint32_t) (slotNum) << mAddrShifter));
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
		const cmdType& ctype, const uint8_t& lba, const uint16_t& cwCnt, \
		const uint32_t& pageNum, uint64_t& cmdVal)
	{
		cmdVal = (((uint64_t)slotNum & 0xff)<< (mActiveLBAMaskBits + 19)) |\
			(((uint64_t)queueAddr & 0xff) << (mActiveLBAMaskBits + 11)) | \
			(((uint64_t)ctype & 0x3) << (mActiveLBAMaskBits + 9)) | \
			(((uint64_t)lba | 0x0000000000000000) << 9) | (cwCnt & 0x1ff);
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
	void TeraSController::createPendingCmdEntry(const uint64_t cmd, uint32_t& cmdVal)
	{
		uint64_t lba = (uint64_t)((cmd >> 9) & getActiveLBAMask());
		uint8_t tempLba = (uint8_t)(lba & getPendingLBAMask());
		uint8_t CmdVal = (uint8_t) ((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
		uint8_t slotNum = (uint8_t) ((cmd >> (mActiveLBAMaskBits + 19)) & 0xff);
		uint8_t queueAddr = (uint8_t) ((cmd >> (mActiveLBAMaskBits + 11)) & 0xff);
		uint16_t cwCnt = (uint16_t)(cmd & 0x1ff);
		cmdVal = ((uint32_t) (slotNum) << (mPendingLBAMaskBits + 19)) | ((uint32_t) (queueAddr) << (mPendingLBAMaskBits + 11)) \
			| ((uint32_t) (CmdVal) << (mPendingLBAMaskBits + 9)) | ((uint32_t) (tempLba) << 9) |(uint32_t) cwCnt;
	}

	/** pending Write Command process
	* @param chanNum channel Number
	* @return void
	**/
	void TeraSController::pendingCmdProcess(uint8_t chanNum)
	{
		uint32_t cmd;
		cmdType ctype;
		uint8_t lba;
		uint16_t cwCnt;
		uint16_t slotNum;
		uint8_t queueAddr;
		
		std::ostringstream msg;
	
			next_trigger(mPendingCmdQueue.at(chanNum)->get_event());
			while ((cmd = mPendingCmdQueue.at(chanNum)->popCommand()) != 0)
			{
				uint32_t cwBankMask = getCwBankMask();
				uint8_t cwBankIndex = mPendingCmdQueue.at(chanNum)->getCwBankNum(cwBankMask);
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
						cwBankIndex = cwBankIndex + mCodeWordNum / 2;


					mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::FREE;
					mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);
					mIntermediateQueue.update(cmd, mPendingLBAMaskBits);
					uint16_t cnt = mIntermediateQueue.getCount();
					//uint8_t slotNum = mIntermediateQueue.getSlotNum();

					msg.str("");
					msg << "PENDING QUEUE PUSH: WRITE COMMAND: "
						<< "  @Time: " << dec << sc_time_stamp().to_double()
						<< " LBA =0x" << hex << (uint32_t)lba
						<< " Slot Number: " << dec << (uint32_t)slotNum
						<< " Command Payload: " << hex << (uint32_t)cmd;

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
	void TeraSController::pendingCmdReadProcess(uint8_t chanNum)
	{
		uint32_t cmd;
		cmdType ctype;
		uint8_t lba;
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
						mLongQueueSize.at(chanNum) = (uint16_t)mAlcq.at(chanNum).getSize();
						mLongQueueReport->writeFile("LONG", sc_time_stamp().to_double(), chanNum, mLongQueueSize.at(chanNum), " ");
						mLongQueueReportCsv->writeFile("LONG", sc_time_stamp().to_double(), chanNum, mLongQueueSize.at(chanNum), ",");
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
		value = ((uint16_t)((bool)cmd) << 15) | ((uint16_t)(slotNum) << 7) | ((uint16_t)(cwCnt) & 0x7f);
	}

	/** decode command payload recieved
	* @param cmd command payload
	* @param slotNum slot index
	* @param cwCnt code word count
	* @param value completion entry value
	* @return void
	**/
	void TeraSController::decodeCommand(const uint64_t cmd, cmdType& ctype, \
		uint8_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (uint8_t) ((cmd >> 9) & getCwBankMask());
		chipSelect = (bool) ((cmd >> (mCwBankMaskBits + 9)) & 0x1);
		
		page = (uint32_t)((cmd >> (mCwBankMaskBits + 10)) & getActivePageMask());
		ctype = (cmdType)((cmd >> (mActiveLBAMaskBits + 9)) & 0x3);
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
	void TeraSController::createPayload(bool& chipSelect, const cmdType ctype, uint8_t cwBank, \
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
		memcpy(dataPtr + sizeof(uint8_t), (uint8_t*)data, length-2);
	}

	void TeraSController::createPayload(uint8_t data, const uint32_t& length, uint8_t* dataPtr)
	{
		//memcpy(dataPtr, &count, sizeof(uint8_t));
		memcpy(dataPtr, (uint8_t*)&data, length);
	}
	/** Get  address
	* @param uint64_t page
	* @param uint8_t bank
	* @return uint64_t
	**/
	uint64_t TeraSController::getAddress(uint32_t& page,uint8_t& bank, bool& chipSelect)
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

		for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
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

		for (uint8_t bitIndex = 0; bitIndex < mCwBankMaskBits; bitIndex++)
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
				return SHORT;
			}
			else if (longCmd != -1)
			{
				return LONG;
			}
			
		}
		else
		{		
			if (longCmd != -1)
			{
				return LONG;
			}
			else {
			
				wait(*mAlcqNotEmptyEvent.at(chanNum));
				return LONG;
			}
			
		}
	
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
	void TeraSController::decodePendingCmdEntry(const uint32_t cmd, cmdType& ctype, \
		uint8_t& lba, uint16_t& cwCnt, uint16_t& slotNum, uint8_t& queueAddr)
	{
		slotNum = (uint8_t)((cmd >> (mPendingLBAMaskBits + 19)) & 0xff);
		queueAddr = (uint8_t)((cmd >> (mPendingLBAMaskBits + 11)) & 0xff);
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
	void TeraSController::decodeLBA(uint64_t cmd, uint8_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (uint8_t) ((cmd >> 9) & getCwBankMask());
		chipSelect = (bool)((cmd >> (mCwBankMaskBits + 9)) & 0x1);
		page = (uint32_t) ((cmd >> (mCwBankMaskBits + 10)) & getActivePageMask());
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
}
