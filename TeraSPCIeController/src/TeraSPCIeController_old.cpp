// TeraSPCIeController.cpp : Defines the entry point for the console application.
//

#include "TeraSPCIeController.h"
namespace  CrossbarTeraSLib {

	TeraSPCIeController::~TeraSPCIeController()
	{
		delete mMemReadTLP;
		for (uint32_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			delete[] mReadData.at(chanIndex);
			delete[] mWriteData.at(chanIndex);
			delete mRespQueue.at(chanIndex);
			delete pChipSelect.at(chanIndex);

			delete mPendingCmdProcOpt.at(chanIndex);
			delete mPendingReadCmdProcOpt.at(chanIndex);
			delete mCheckRespProcOpt.at(chanIndex);
			delete mLongQThreadOpt.at(chanIndex);
			delete mChanMgrProcOpt.at(chanIndex);
			delete mCmdDispatcherProcOpt.at(chanIndex);

			delete pOnfiBus.at(chanIndex);
			delete mEndReadEvent.at(chanIndex);
			delete mCmdDispatchEvent.at(chanIndex);
			delete mAlcqNotFullEvent.at(chanIndex);
			delete mAscqNotFullEvent.at(chanIndex);
			delete mAdcqNotFullEvent.at(chanIndex);
			delete mAlcqNotEmptyEvent.at(chanIndex);
			delete mAdcqNotEmptyEvent.at(chanIndex);
			delete mAscqNotEmptyEvent.at(chanIndex);
			delete mTrigCmdDispEvent.at(chanIndex);
			delete mCmplTLPDataRxEvent.at(chanIndex);
			delete mEndReqEvent.at(chanIndex);

		}
		//mLongQueueReport->closeFile();
		//delete mLongQueueReport;
		//mShortQueueReport->closeFile();
		//delete mShortQueueReport;
		mLatencyReport->closeFile();
		delete mLatencyReport;
		mOnfiChanUtilReport->closeFile();
		delete mOnfiChanUtilReport;
		mOnfiChanActivityReport->closeFile();
		delete mOnfiChanActivityReport;
		//LogFileHandler.close();
	}
#pragma region STATIC_THREADS

	void TeraSPCIeController::arbiterThread()
	{
		tlm::tlm_generic_payload transaction;
		std:ostringstream msg;
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
				case TLPQueueType::MEM_READ_CMD_Q:
				{
													 msg.str("");
													 msg << "SEND MEM READ REQ TLP"
														 << "  @Time= " << dec << sc_time_stamp().to_double() << endl;
													 REPORT_INFO(filename, __FUNCTION__, msg.str());

													 mLogFileHandler << "TeraSPCIeController::ARBITER: "
														 << "SEND MEM READ REQ TLP: "
														 << " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
														 << endl;
													 /*Acquire data pointer from memory manager*/
													 uint8_t* data = mMemMgr.getPtr(MEM_RD_CMD_WIDTH);

													 /* Get  CMD TLP from the queue*/
													 mTLPQueueMgr.popTLP(TLPQueueType::MEM_READ_CMD_Q, data);

													 /*Notify other methods waiting on queue to have space*/
													 mMemReadCmdQNotFullEvent.notify(SC_ZERO_TIME);
													 /*create TLM payload*/
													 transaction.set_data_ptr(data);
													 transaction.set_address(0);
													 transaction.set_data_length(MEM_RD_CMD_WIDTH);
													 transaction.set_read();
													 sc_time txTime = sc_time((double)((EFF_MEM_RD_CMD_WIDTH * 1000) / mPcieSpeed), SC_NS);
													 pcieTxInterface->b_transport(transaction, txTime);
													 /*release memory once host processes the data*/
													 mMemMgr.freePtr(data);
													 break;
				}
				case TLPQueueType::MEM_READ_DATA_Q:
				{
													  msg.str("");
													  msg << "SEND MEM READ REQ TLP WITH DATA "
														  << "  @Time= " << dec << sc_time_stamp().to_double() << endl;
													  REPORT_INFO(filename, __FUNCTION__, msg.str());

													  mLogFileHandler << "TeraSPCIeController::ARBITER: "
														  << "SEND MEM READ REQ TLP WITH DATA "
														  << " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
														  << endl;
													  /*Acquire data pointer from memory manager*/
													  uint8_t* data = mMemMgr.getPtr(MEM_RD_DATA_WIDTH);

													  /* Get  READ DATA TLP from the queue*/
													  mTLPQueueMgr.popTLP(TLPQueueType::MEM_READ_DATA_Q, data);
													 
													  /*Notify other methods waiting on queue to have space*/
													  mMemReadDataQNotFullEvent.notify(SC_ZERO_TIME);

													  /*create TLM payload*/
													  transaction.set_data_ptr(data);
													  transaction.set_address(0);
													  //transaction.set_data_length(MEM_RD_CMD_WIDTH);
													  transaction.set_data_length(MEM_RD_DATA_WIDTH);
													  transaction.set_read();
													  sc_time txTime = sc_time((double)(((MEM_RD_DATA_WIDTH + mCwSize) * 1000) / mPcieSpeed), SC_NS);
													  pcieTxInterface->b_transport(transaction, txTime);

													  /*release memory once host processes the data*/
													  mMemMgr.freePtr(data);
													  break;
				}
				case TLPQueueType::MEM_WRITE_DATA_Q:
				{
													   msg.str("");
													   msg << "SEND MEM WRITE WITH DATA TLP "
														   << "  @Time= " << dec << sc_time_stamp().to_double() << endl;
													   REPORT_INFO(filename, __FUNCTION__, msg.str());

													   mLogFileHandler << "TeraSPCIeController::ARBITER: "
														   << "SEND MEM WRITE WITH DATA TLP "
														   << " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
														   << endl;
													   /*Acquire data pointer from memory manager*/
													   uint8_t* data = mMemMgr.getPtr(MEM_WR_DATA_WIDTH);
													   //uint8_t* data = mMemMgr.getPtr(MEM_WR_DATA_WIDTH + mCwSize);

													   /* Get  WRITE DATA TLP from the queue*/
													   mTLPQueueMgr.popTLP(TLPQueueType::MEM_WRITE_DATA_Q, data);

													   /*Notify other methods waiting on queue to have space*/
													   mMemWriteDataQNotFullEvent.notify(SC_ZERO_TIME);

													   /*create TLM payload*/
													   transaction.set_data_ptr(data);
													   transaction.set_address(0);
													   //transaction.set_data_length(MEM_WR_DATA_WIDTH + mCwSize);
													   transaction.set_data_length(EFF_MEM_WR_DATA_WIDTH + mCwSize);
													   transaction.set_write();

													   //sc_time txTime = sc_time((((MEM_WR_DATA_WIDTH + mCwSize) * 1000) / mPcieSpeed), SC_NS);
													   sc_time txTime = sc_time(((double)((EFF_MEM_WR_DATA_WIDTH + mCwSize) * 1000) / mPcieSpeed), SC_NS);
													   pcieTxInterface->b_transport(transaction, txTime);


													   /*release memory once host processes the data*/
													   tMemoryWriteReqTLP* memWriteReqTLP;// = new tMemoryWriteReqTLP();
													   //memcpy(memWriteReqTLP, reinterpret_cast<tMemoryWriteReqTLP*> (data), sizeof(tMemoryWriteReqTLP));
													   memWriteReqTLP = reinterpret_cast<tMemoryWriteReqTLP*> (data);
													   mMemMgr.freePtr(memWriteReqTLP->data);
													   mMemMgr.freePtr(data);
													   //delete memWriteReqTLP;
													   break;
				}
				case TLPQueueType::MEM_WRITE_COMP_Q:
				{
													   msg.str("");
													   msg << "SEND MEM WRITE REQ TO COMPLETION "
														   << "  @Time= " << dec << sc_time_stamp().to_double() << endl;
													   REPORT_INFO(filename, __FUNCTION__, msg.str());

													   mLogFileHandler << "TeraSPCIeController::ARBITER: "
														   << "MEM WRITE COMPLETION TLP SENT: "
														   << " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
														   << endl;
													   /*Acquire data pointer from memory manager*/
													   uint8_t* data = mMemMgr.getPtr(MEM_WR_COMPLETION_WIDTH);

													   /* Get  WRITE COMPLETION QUEUE TLP from the queue*/
													   mTLPQueueMgr.popTLP(TLPQueueType::MEM_WRITE_COMP_Q, data);

													   /*Notify other methods waiting on queue to have space*/
													   mMemWriteCompQNotFullEvent.notify(SC_ZERO_TIME);

													   /*create TLM payload*/
													   transaction.set_data_ptr(data);
													   transaction.set_address(0);
													   transaction.set_data_length(MEM_WR_COMPLETION_WIDTH);
													   transaction.set_write();

													   sc_time txTime = sc_time((double)(EFF_MEM_WR_COMPLETION_WIDTH * 1000 / mPcieSpeed), SC_NS);
													   pcieTxInterface->b_transport(transaction, txTime);

													   tMemoryWriteReqTLP* memWriteReqTLP;
													   memWriteReqTLP = reinterpret_cast<tMemoryWriteReqTLP*> (data);

													   /*release memory once host processes the data*/
													   mMemMgr.freePtr(memWriteReqTLP->data);
													   mMemMgr.freePtr(data);
													   break;

				}
				default: break;
				}
			}
		}//while
	}

	void TeraSPCIeController::startCntrlr()
	{
		std::ostringstream msg;
		while (1)
		{
			/*msg.str("");
			msg << "TeraSPCIeController::startCntrlr Thread() called "
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
			REPORT_INFO(filename, __FUNCTION__, msg.str());
			mLogFileHandler << "TeraSPCIeController::startCntrlr Thread() called "
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;*/

			bool enable = GET_CC_EN(regCC);
			if (enable)
			{
				msg.str(" ");
				msg<< "PCIE ENABLED: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "TeraSPCIeController::START PCIE: "
					<< "PCIE ENABLED: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;

				wait(mTimeOut * 500, SC_MS);

				regCSTS=SET_CSTS_RDY(regCSTS, 1);

				msg << "PCIE READY: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "TeraSPCIeController::START PCIE: "
					<< "PCIE READY: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
			}
			else
			{
				wait(mTimeOut * 500, SC_MS);
				regCSTS=SET_CSTS_RDY(regCSTS, 0);

				msg << "PCIE RESET: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "TeraSPCIeController::START PCIE "
					<< "PCIE RESET: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
			}
			wait();
		}
	}

	void TeraSPCIeController::writeCmdDoneThread()
	{
		std::ostringstream msg;
		while (1)
		{
			if (mTagQueue.empty())
			{
				wait();
			}

			if (mCQ1Queue.isFull())
			{
				wait(mCQ1HdblEvent);
				//mCQ1Queue.setHead(regCQ1HDBL);
				//mCQ1Queue.push();
			}

			/*If MEM_WRITE COMPLETION queue is full, then wait*/
			if (mTLPQueueMgr.isFull(TLPQueueType::MEM_WRITE_COMP_Q))
			{
				wait(mMemWriteCompQNotFullEvent);
			}

			if ((mCQ1Queue.getTail() == (BA_CQ0)))// && (!mCQ1Queue.isEmpty()))
			{
				msg.str("");
				msg << "INVERT PHASE TAG "
					<< "  @Time= " << dec << sc_time_stamp().to_double();
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "TeraSPCIeController::INVERT PHASE TAG "
					<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< endl;
				mPhaseTagCQ1 = !mPhaseTagCQ1;
			}

			/*Get the tag from the tag queue*/
			uint32_t iTag = mTagQueue.front();
			mTagQueue.pop();

			/*Set the Host command queue to free state*/
			mHostCmdQueue.setQueueFree(iTag);
			mHostCmdQueue.pop(iTag);
			/* Tag is set free to be reused*/
			mTagMgr.resetTag(iTag);

			uint16_t cid;
			cmdType cmd;
			uint64_t lba, addr0, addr1;
			uint16_t blkCnt;

			/*Get parameter from the parameter table*/
			mParamTable.getParam(iTag, cid, cmd, lba, blkCnt, addr0, addr1);
			mParamTable.resetParam(iTag);
			uint8_t* data = mMemMgr.getPtr(16);
			createCQ1Entry(cid, cmd, mPhaseTagCQ1, 0x0, mSQ1Queue.getHead(), 0x1, data);

			/* Create Completion Queue data structure and
			send memory write TLP*/
			uint64_t address;
			uint32_t qAddr = 0;
			address = mCQ1Queue.getTail();
			mCQ1Queue.push();
			createMemWriteTLP(address, hostQueueType::COMPLETION, qAddr, data);
									
			mArbitrateEvent.notify(SC_ZERO_TIME);
			mHCmdQNotFullEvent.notify(SC_ZERO_TIME);
			/*if (regSQ1TDBL != mSQ1Queue.getTail())
			{
				mTrigMemReadCmdEvent.notify(SC_ZERO_TIME);
			}*/
			msg.str("");
			msg << "COMPLETION QUEUE TLP PUSH::WRITE COMMAND "
				<< "  @Time= " << dec << sc_time_stamp().to_double();
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << "TeraSPCIeController::WRITE COMMAND COMPLETE "
				<< "COMPLETION QUEUE TLP PUSH::WRITE COMMAND "
				<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
				<< endl;

		}
	}
	

	void TeraSPCIeController::pcmdQInsertThread()
	{
		std::ostringstream msg;
		while (1)
		{
			if (mHCmdQueue.empty())
			{
				wait();
			}
			else {
				uint32_t indexSize = mCmdSize.front();
				HCmdQueueData hostData = mHCmdQueue.front();
				uint32_t tag = mFreeTagQueue.front();
				sc_time delay = mDelayQueue.front();
				mDelayQueue.pop();
				mFreeTagQueue.pop();
				mHCmdQueue.pop();
				mCmdSize.pop();
				uint32_t qAddr;
				uint64_t lba = hostData.lba;
				uint32_t cmdOffset = 0;
				int32_t nextAddr = -1;

				//Stripe HCmd at cWSize granularity and save the cmds in PCMDQ
				for (uint16_t qIndex = 0; qIndex < indexSize; qIndex++)
				{
					/*If Physical Cmd Queue is full*/
					if (mCmdQueue.isFull())
					{
						wait(mCmdQNotFullEvent);
						cmdOffset = 0;
					}

					/*Get free command queue address from the pool */
					if (mCmdQueue.getFreeQueueAddr(qAddr))
					{
						//delay = sc_time_stamp();

						/*Set the Command queue address busy */
						mCmdQueue.setQueueBusy(qAddr);
						mNumPcmdCmd++;
						/*insert the data into the command queue address*/
						mCmdQueue.insert(qAddr, hostData.hCmd, lba, cmdOffset, tag, nextAddr, delay);//save striped cmds into PCMDQ
						msg.str("");
						msg << "PCMD QUEUE PUSH: "
							<< " COMMAND: 0x" << hex << (uint32_t)hostData.hCmd
							<< " LBA: 0x" << hex << (uint64_t)lba
							<< " OFFSET: 0x" << hex << (uint32_t)cmdOffset
							<< " TAG: " << hex << tag
							<< " NEXT ADDR: 0x" << hex << (int32_t)nextAddr
							<< " PCMDQ ADDR: 0x" << hex << (uint32_t)qAddr
							<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
						REPORT_INFO(filename, __FUNCTION__, msg.str());

						mLogFileHandler << "PCMD QUEUE PUSH: "
							<< " COMMAND: 0x" << hex << (uint32_t)hostData.hCmd
							<< " LBA: 0x" << hex << lba
							<< " OFFSET: 0x" << hex << cmdOffset
							<< " TAG: " << hex << tag
							<< " NEXT ADDR: 0x" << hex << nextAddr
							<< " PCMDQ ADDR: 0x" << hex << qAddr
							<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
							<< endl;
						lba++;
						cmdOffset++;
						
						/*Create Link list of banks for each channel*/
						TeraSBankLLMethod(qAddr);
					}
					else
					{
						qIndex--;
					}
				}
			}//else
		}//while
	}

	

	void TeraSPCIeController::memReadReqCmdThread()
	{
		std::ostringstream msg;
		while (1)
		{
			
			if (mSQ1Queue.getTail() != mSQ1Queue.getHead())
			{

				/*Check if TLP Queue is full*/
				if (mTLPQueueMgr.isFull(MEM_READ_CMD_Q))
				{
					wait(mMemReadCmdQNotFullEvent);
				}


				/*Calculate the space available in Host Command Queue*/
				uint32_t spaceLeft = mHostCmdQueue.getQSpaceLeft();//Check for space left in host cmd queue
				uint32_t numCommands;
				/*calculate the number of commands that needs to be processed*/

				if (mSQ1Queue.getTail() > mSQ1Queue.getHead()){
					numCommands = (mSQ1Queue.getTail() - mSQ1Queue.getHead()) / 64;
				}
				else
				{
					//div_t result = std::div((int)(mSQ1Queue.getHead() - mSQ1Queue.getTail()) / 64, (int)mSizeOfSQ);

					//numCommands = result.quot + result.rem;
					numCommands = mSizeOfSQ - (mSQ1Queue.getHead() - mSQ1Queue.getTail()) / 64 + 1;
				}
				
				msg.str("");
				msg << "TeraSPCIeController::Mem Thread() called "
					<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "MEM READ REQ CMD PUSH: "
					<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
					<< endl;
				/* If Space available is more of equal to
				number of commands to be processed then
				fetch all the commands*/
				if ((numCommands != 0) && (spaceLeft != 0))
				{

					if (spaceLeft >= numCommands)
					{
						uint8_t chanNum = 0x0;
						uint64_t address = mSQ1Queue.getHead();
						createMemReadTLP(address, numCommands * 16, tagType::SQ1_CMDTAG, chanNum);
						mArbitrateEvent.notify(SC_ZERO_TIME);
						for (uint32_t i = 0; i < numCommands; i++)
							mSQ1Queue.pop();

						//mNumCmd += numCommands;
						/*if there is still command left in the SQ1 */
						//uint32_t SQ1CmdLeft = numCommands - spaceLeft;
						//if (SQ1CmdLeft == 0)
						//{
						if (regSQ1TDBL != mSQ1Queue.getTail())
						{
							mSQ1Queue.setTail(regSQ1TDBL);
							mTrigMemReadCmdEvent.notify(SC_ZERO_TIME);
						}
						else
						{
							wait(mCompletionTLPRxEvent);
							wait(mHCmdQNotFullEvent);
						}
						//}
					}
					/* If space left on the host command queue is less than
					the number of commands to be processed then keep on
					notifying this whenever there is space available
					*/
					else
					{
						/*if (isDoorBellSet == false)
						{
						mSQ1TDBL = mSQ1Queue.getTail();
						isDoorBellSet = true;
						}*/
						uint8_t chanNum = 0x0;
						uint64_t address = mSQ1Queue.getHead();
						createMemReadTLP(address, spaceLeft * 16, tagType::SQ1_CMDTAG, chanNum);
						mArbitrateEvent.notify(SC_ZERO_TIME);
						for (uint32_t i = 0; i < spaceLeft; i++)
							mSQ1Queue.pop();

						/*if there is still command left in the SQ1 */
						uint32_t SQ1CmdLeft = numCommands - spaceLeft; // regSQ1TDBL - mSQ1Queue.getTail();
						//mNumCmd += spaceLeft;
						/*Notify Memory read thread again, the idea is to keep the Host Command queue
						fully utilized all the time*/
						if (SQ1CmdLeft)
						{
							wait(mCompletionTLPRxEvent);
							wait(mHCmdQNotFullEvent);
						}
						else
						{
							if (regSQ1TDBL != mSQ1Queue.getTail())
							{
								mSQ1Queue.setTail(regSQ1TDBL);
								mTrigMemReadCmdEvent.notify(SC_ZERO_TIME);
							}
						}
					}
					//mArbiterToken.push(0);
					


				}//if ((numCommands != 0) && (spaceLeft != 0))
			}//if
			else
			{
				wait();
			}
		}//while(1)
	}

#pragma endregion STATIC_THREADS

#pragma region CMD_DISPATCHEBR_HELPER
	void TeraSPCIeController::pushLongQueueCmd(uint8_t chanNum, uint8_t cwBankIndex, ActiveCmdQueueData queueVal)
	{
		/*Check if long queue is full*/
		if (mAlcq.at(chanNum).isFull())
		{
			wait(*(mAlcqNotFullEvent.at(chanNum)));
		}
		std::ostringstream msg;
		msg.str("");
		msg << "WRITE COMMAND: "
			<< " @Time: " << dec << (sc_time_stamp().to_double() + queueVal.time.to_double())
			<< " Active LBA =0x" << hex << (uint64_t)queueVal.plba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Queue Number: " << dec << (uint32_t)queueVal.queueNum
			<< " Bank Index " << hex << (uint32_t)cwBankIndex
			<< " Address: " << hex << (uint32_t)queueHead.at(chanNum).at(cwBankIndex);
		REPORT_INFO(filename, __FUNCTION__, msg.str());
		mLogFileHandler << "CommandDispatcher::WRITE COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + queueVal.time.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)queueVal.plba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Queue Number= " << dec << (uint32_t)queueVal.queueNum
			<< " Bank Index= " << dec << (uint32_t)cwBankIndex
			<< " Address= 0x" << hex << (uint32_t)queueHead.at(chanNum).at(cwBankIndex)
			<< endl;

		/*Push the WRITE command into the long queue */
		mAlcq.at(chanNum).pushQueue(queueVal);

		/* Notify presence of command in the long queue*/
		mAlcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);

		/*Set the corresponding bank to busy*/
		mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY;
	}

	void TeraSPCIeController::pushShortQueueCmd(uint8_t chanNum, uint8_t cwBankIndex, ActiveCmdQueueData queueVal)
	{
		/*Check if short command queue is full*/
		if (mAscq.at(chanNum).isFull())
		{
			wait(*(mAscqNotFullEvent.at(chanNum)));
		}
		std::ostringstream msg;
		msg.str("");
		msg << "READ COMMAND: "
			<< " @Time= " << dec << sc_time_stamp().to_double() + queueVal.time.to_double()
			<< " Active LBA =0x" << hex << (uint64_t)queueVal.plba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Queue Number: " << dec << (uint32_t)queueVal.queueNum
			<< " CW Bank " << hex << (uint32_t)cwBankIndex
			<< " Address: " << dec << queueHead.at(chanNum).at(cwBankIndex);
		REPORT_INFO(filename, __FUNCTION__, msg.str());
		mLogFileHandler << "CommandDispatcher::READ COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + queueVal.time.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)queueVal.plba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Queue Number= " << dec << (uint32_t)queueVal.queueNum
			<< " CW Bank= " << dec << (uint32_t)cwBankIndex
			<< " Address: 0x" << dec << queueHead.at(chanNum).at(cwBankIndex)
			<< endl;

		/*Push the READ command into short queue*/
		mAscq.at(chanNum).pushQueue(queueVal);

		/*Notify presence of command in the short queue */
		mAscqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);

		/*Set the corresponding bank busy*/
		mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_BUSY; //Make it busy
	}
#pragma endregion

#pragma region CHAN_MANAGER_HELPER

	void TeraSPCIeController::dispatchShortCmd(uint8_t chanNum)
	{
		ActiveCmdQueueData queueData;
		std::ostringstream msg;

		if (mAscq.at(chanNum).isFull())
		{
			mAscqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
		}

		/* Get the data from the short queue*/
		queueData = mAscq.at(chanNum).popQueue();


		sc_core::sc_time delay = queueData.time;

		/*Short Queue utilization log*/
		/*  mShortQueueSize.at(chanNum) = (uint16_t)mAscq.at(chanNum).getSize();
		mShortQueueReport->writeFile("SHORT", sc_time_stamp().to_double() + delay.to_double(), chanNum, mShortQueueSize.at(chanNum), " ");
		mShortQueueReportCsv->writeFile("SHORT", sc_time_stamp().to_double() + delay.to_double(), chanNum, mShortQueueSize.at(chanNum), ",");*/

		msg.str("");
		msg << "SHORT COMMAND: "
			<< " @Time " << dec << (sc_time_stamp().to_double() + delay.to_double())
			<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
			<< " Channel Number: " << dec << (uint32_t)chanNum
			<< " Command Type: " << hex << queueData.cmd
			<< " Queue Number: " << dec << (uint32_t)queueData.queueNum;

		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << "CHANNEL MANAGER::SHORT COMMAND: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Command Type: 0x" << hex << (uint32_t)queueData.cmd
			<< " Queue Number= " << dec << (uint32_t)queueData.queueNum
			<< endl;

		// mOnfiCode.at(chanNum) = ONFI_READ_COMMAND;

		/*Send READ command to the ONFI channel*/
		RRAMInterfaceMethod(queueData, chanNum, mReadData.at(chanNum), delay, ONFI_READ_COMMAND);
		
		/*Push the cmd data into the pending queue*/
		mPendingReadCmdQueue.at(chanNum)->notify(queueData, mReadTime);
	}
	void TeraSPCIeController::dispatchWriteCmd(uint8_t chanNum, ActiveDMACmdQueueData& queueData)
	{
		mLogFileHandler << "CHANNEL MANAGER::LONG COMMAND:WRITE "
			<< " @Time: " << dec << (sc_time_stamp().to_double() + queueData.time.to_double()) << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Command Type= " << dec << (uint32_t)queueData.cmd
			<< " Queue Number= " << dec << (uint32_t)queueData.queueNum
			<< endl;

		
		/* Read the data to be written from the write data buffer*/
		mWriteDataBuff.readData(queueData.buffPtr, mWriteData.at(chanNum));

		/* Send the write command to the ONFI channel*/
		RRAMInterfaceMethod(queueData, chanNum, mWriteData.at(chanNum), queueData.time, ONFI_WRITE_COMMAND);
		ActiveCmdQueueData pendingCmd;
		
		/*Push the WRITE command to the pending queue*/
		createPendingCmdQEntry(queueData, pendingCmd);
		mPendingCmdQueue.at(chanNum)->notify(pendingCmd, mProgramTime);
	}

	void TeraSPCIeController::dispatchReadCmd(uint8_t chanNum, ActiveDMACmdQueueData& queueData)
	{
		/*If the DMA command is */
		mLogFileHandler << "CHANNEL MANAGER::LONG COMMAND:READ "
			<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
			<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " Command Type= " << dec << (uint32_t)queueData.cmd
			<< " Queue Number= " << dec << (uint32_t)queueData.queueNum
			<< endl;

		//mOnfiCode.at(chanNum) = ONFI_BANK_SELECT_COMMAND;
		sc_time delay = sc_time(0, SC_NS);
		/*Fetch the READ data from the ONFI*/
		RRAMInterfaceMethod(queueData, chanNum, mReadData.at(chanNum), delay, ONFI_BANK_SELECT_COMMAND);

		mOnfiChanTime.at(chanNum) = (double)((double)(mCwSize * 1000) / mOnfiSpeed);

		mOnfiChanUtilReport->writeFile(sc_time_stamp().to_double(), chanNum, 0, " "/* + delay.to_double()*/);
		mOnfiChanUtilReportCsv->writeFile(sc_time_stamp().to_double(), chanNum, 0, ","/* + delay.to_double()*/);
	}
#pragma endregion

#pragma region DYNAMIC_THREADS

	void TeraSPCIeController::cmdDispatcherThread(uint8_t chanNum)
	{
		uint16_t offset = 0;

		std::ostringstream msg;
		uint8_t numBanksBusy = 0;

		while (1)
		{
			/*The command dispatcher only enters into arbitration if there is any bank free
			else it waits for any of the bank to be free*/
			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
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

			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
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

			/*Command dispatcher goes over every logical bank link list to find out
			which one is free and then dispatches command from that queue address that corresponds to that
			particular bank */
			for (uint8_t cwBankIndex = 0; cwBankIndex < mCodeWordNum; cwBankIndex++)
			{
				mBankLinkList.getTail(chanNum, cwBankIndex, queueTail.at(chanNum).at(cwBankIndex));
				queueHead.at(chanNum).at(cwBankIndex) = mBankLinkList.getHead(chanNum, cwBankIndex);
				if (queueHead.at(chanNum).at(cwBankIndex) != -1)
				{
					// if Bank is busy then it goes to the next link list
					if (mCwBankStatus.at(chanNum).at(cwBankIndex) == cwBankStatus::BANK_FREE)//if not busy
					{
						cmdType cmd;
						int32_t nextAddr;
						ActiveCmdQueueData queueVal;
						uint64_t lba;
						uint32_t cmdOffset;
						uint32_t iTag;
						sc_core::sc_time timeVal;

						int32_t tailAddr;

						/*get tail address from the bank link list*/
						mBankLinkList.getTail(chanNum, cwBankIndex, tailAddr);

						/*Fetch command from the address pointed to by the head of the link list*/
						mCmdQueue.fetchCommand(queueHead.at(chanNum).at(cwBankIndex), cmd, lba, cmdOffset, iTag, nextAddr, timeVal);
						//mCmdQueue.fetchNextAddress(queueHead.at(chanNum).at(cwBankIndex), nextAddr);
						/*If we come to the end of LL i.e head = tail the reset the
						head and tail linked list to -1*/
						if (queueHead.at(chanNum).at(cwBankIndex) == tailAddr)
						{
							mBankLinkList.updateHead(chanNum, cwBankIndex, -1);
							mBankLinkList.updateTail(chanNum, cwBankIndex, -1);
						}
						/*else update the head with the next address of the command queue */
						else {
							mBankLinkList.updateHead(chanNum, cwBankIndex, nextAddr);
						}

						/*Ready to dispatch command to the active queue, create active queue command entry*/
						createActiveCmdEntry(cmd, lba, cmdOffset, iTag, queueHead.at(chanNum).at(cwBankIndex), timeVal, queueVal);

						if (cmd == READ) // In case of Read
						{
							pushShortQueueCmd(chanNum, cwBankIndex, queueVal);
						}
						else {  //In case of Write
							
							pushLongQueueCmd(chanNum, cwBankIndex, queueVal);
						}
					}//if (mCwBankStatus.at(chanNum).at(cwBankIndex) == false)
					
				}//if(!-1)
				
			}//for

		}//while
	}

	void TeraSPCIeController::longQueueThread(uint8_t chanNum)
	{
		std::ostringstream msg;
		ActiveCmdQueueData queueData;
		
		while (1)
		{
			if (mAlcq.at(chanNum).isEmpty())
			{
				wait(*mAlcqNotEmptyEvent.at(chanNum));
			}
			/*If long queue is full, then notify that long is has space as
			we are about to pop data from the long queue*/
			if (mAlcq.at(chanNum).isFull())
			{
				mAlcqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
			}

			/*pop cmd out of the long queue*/
			queueData = mAlcq.at(chanNum).popQueue();

			
			uint8_t qAddr;

			/* Get the first free write buffer address*/
			mWriteDataBuff.getFreeBuffPtr(qAddr);
			/*Set it to busy*/
			mWriteDataBuff.setBuffBusy(qAddr);
			/*Save the adddress of the Write buffer in a queue*/
			mWriteBuffAddr.at(chanNum).push(qAddr);
			//mMemWriteDataQEvent.notify(SC_ZERO_TIME);

			/*Check if MEM_READ_DATA Queue is full*/
			if (mTLPQueueMgr.isFull(MEM_READ_DATA_Q))
			{
				wait(mMemReadDataQNotFullEvent);
			}

			/* Create Memory Read Request Data TLP and push it into the
			TLP Buffer*/
			uint32_t length = mCwSize / 4;
			uint32_t offset;
			uint64_t dataAddress;
			uint64_t hostAddress;
			
			/* Get Sub Command offset*/
			mCmdQueue.getCmdOffset(queueData.queueNum, offset);

			hostAddress = mHostCmdQueue.getHostAddress(queueData.iTag);
			/*Calculate the sub command memory address*/
			dataAddress = hostAddress + offset * mCwSize;

			createMemReadTLP(dataAddress, length, BUFFPTR_TAG, chanNum);
			
			/*Notify to arbitrate the TLP command into the bus*/
			mArbitrateEvent.notify(SC_ZERO_TIME);
			
			/* Find if the write data is available in the Data
			buffer else wait*/
			/* Wait for completion TLP to write to the write buffer*/
			wait(*mCmplTLPDataRxEvent.at(chanNum));
			
			qAddr = mWriteBuffAddr.at(chanNum).front();
			mWriteBuffAddr.at(chanNum).pop();
			/* Create Active DMA Queue Enty and push it into the DMA queue*/

			ActiveDMACmdQueueData qData;

			/*Push the command into the DMA queue*/
			createDMAQueueCmdEntry(queueData, qAddr, qData);
			if (mAdcq.at(chanNum).isFull())
			{
				wait(*(mAdcqNotFullEvent.at(chanNum)));
			}

			mAdcq.at(chanNum).pushQueue(qData);

			/*Notify presence of command in the DMA queue*/
			mAdcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);

			sc_core::sc_time delay = queueData.time;
			msg.str("");
			msg << "LONG COMMAND: "
				<< " @Time: " << dec << sc_time_stamp().to_double() + delay.to_double() << " ns"
				<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
				<< " Channel Number: " << dec << (uint32_t)chanNum
				<< " Command Type: " << hex << queueData.cmd
				<< " Queue Number: " << dec << (uint32_t)queueData.queueNum;

			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << "ChannelManager::LONG COMMAND:WRITE "
				<< " @Time: " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
				<< " Channel Number= " << dec << (uint32_t)chanNum
				<< " Command Type= 0x" << hex << queueData.cmd
				<< " Queue Number= " << dec << (uint32_t)queueData.queueNum
				<< endl;

		}
	}
	
	void TeraSPCIeController::chanManagerThread(uint8_t chanNum)
	{
		std::ostringstream msg;
		while (1)
		{
			/* wait till there is a command in any of the queues */
			if (mAscq.at(chanNum).isEmpty() && mAdcq.at(chanNum).isEmpty())
			{
				wait(*mAdcqNotEmptyEvent.at(chanNum) | *mAscqNotEmptyEvent.at(chanNum));
			}

			/* If Command is present, go for channel arbitration between short queue and DMA queue*/
			else {
				switch (queueArbitration(chanNum))
				{
				case SHORT_QUEUE:
				{
									dispatchShortCmd(chanNum);
							  break;
				}
				case DMA_QUEUE:
				{
							ActiveDMACmdQueueData queueData;

							/*Check if ADCQ is full*/
							if (mAdcq.at(chanNum).isFull())
							{
								mAdcqNotFullEvent.at(chanNum)->notify(SC_ZERO_TIME);
							}

							/*Get the data from DMA queue*/
							queueData = mAdcq.at(chanNum).popQueue();

							sc_core::sc_time delay = queueData.time;
							msg.str("");
							msg << "DMA COMMAND: "
								<< " @Time: " << dec << sc_time_stamp().to_double() + delay.to_double() << " ns"
								<< " Active LBA =0x" << hex << (uint64_t)queueData.plba
								<< " Channel Number: " << dec << (uint32_t)chanNum
								<< " Command Type: " << hex << queueData.cmd
								<< " Queue Number: " << dec << (uint32_t)queueData.queueNum;

							REPORT_INFO(filename, __FUNCTION__, msg.str());

							if (queueData.cmd == WRITE)
							{
								dispatchWriteCmd(chanNum, queueData);
							}
							else if (queueData.cmd == READ){

								dispatchReadCmd(chanNum, queueData);

								

								/*Save the read data in the Read Data Buffer*/
								mReadDataBuff.writeData(queueData.buffPtr, mReadData.at(chanNum));

								/*Check if MEM_WRITE_DATA queue is full*/
								if (mTLPQueueMgr.isFull(TLPQueueType::MEM_WRITE_DATA_Q))
								{
									wait(mMemWriteDataQNotFullEvent);
								}
								uint32_t offset;
								uint64_t hAddr0;
								uint64_t hAddr1;
								uint64_t subCmdAddress;

								/* Get Sub Command offset*/
								mCmdQueue.getCmdOffset(queueData.queueNum, offset);

								/*Get Host Memory base address*/
								//mParamTable.getHostAddr(queueData.iTag, hAddr0, hAddr1);
								uint64_t hostAddress = mHostCmdQueue.getHostAddress(queueData.iTag);
								/*Calculate the sub command memory address*/
								uint64_t dataAddress = hostAddress + offset * mCwSize;

								/* Fetch data from the data buffer*/
								uint8_t* data = mMemMgr.getPtr(mCwSize);

								/* ECC Delay*/
								wait(1000, SC_NS);

								mReadDataBuff.readData(queueData.buffPtr, data);

								/*Create Memory Write TLP*/
								createMemWriteTLP(dataAddress, hostQueueType::DATA, queueData.queueNum, data);

								/* Set the PCMDQ entry to free status*/
								mCmdQueue.setQueueFree(queueData.queueNum);

								/*notify presence of space in the PCMD queue*/
								mCmdQNotFullEvent.notify(SC_ZERO_TIME);
								//mArbiterToken.push(3);
								/* Notify TLP Manager to arbitrate on the bus */
								mArbitrateEvent.notify(SC_ZERO_TIME);
								
								uint8_t cwBankIndex;
								uint32_t page;
								bool chipSelect;
								decodeLBA(queueData.plba, cwBankIndex, chipSelect, page);
								if ((cwBankIndex == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCwSize / mPageSize)))
								{
									cwBankIndex = 0;
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
								if (chipSelect && (mNumDie > 1))
									cwBankIndex = cwBankIndex + mCodeWordNum / mNumDie;

								/*Set the bank status to free*/
								mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;

								/*Notify command dispatcher to send command to this bank*/
								mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);

								/* Decrement the running count*/
								mRunningCntTable.updateCnt(queueData.iTag);

								/*Get the running count*/
								uint16_t cnt = mRunningCntTable.getRunningCnt(queueData.iTag);

								//Check if running count goes down to zero
								if (!cnt)
								{
									/*Host command queue is free*/
									mHostCmdQueue.setQueueFree(queueData.iTag);
									/*Set the Host command queue to free state*/
									mHostCmdQueue.pop(queueData.iTag);
									/*Set Tag in the Tag manager to free status*/
									mTagMgr.resetTag(queueData.iTag);

									/*if (mCQ1Queue.isFull())
									{
										mPhaseTagCQ1 = !mPhaseTagCQ1;
									}*/
									/*If CQ1 is full then wait*/
									if (mCQ1Queue.isFull())
									{
										wait(mCQ1HdblEvent);
									}
									/* CQ1 is full then invert the phase tag to let the host
									know that the queue has gone through one cycle.*/
									if ((mCQ1Queue.getTail() == BA_CQ0))// && (!mCQ1Queue.isEmpty()))
									{
										mPhaseTagCQ1 = !mPhaseTagCQ1;
									}
									/*If MEM_WRITE COMPLETION queue is full, then wait*/
									if (mTLPQueueMgr.isFull(TLPQueueType::MEM_WRITE_COMP_Q))
									{
										wait(mMemWriteCompQNotFullEvent);
									}

									uint16_t cid;
									cmdType cmd;
									uint64_t lba, addr0, addr1;
									uint16_t blkCnt;

									/*Get parameters from the parameter table*/
									mParamTable.getParam(queueData.iTag, cid, cmd, lba, blkCnt, addr0, addr1);

									/*Acquire data pointer 16 bytes wide*/
									uint8_t* data = mMemMgr.getPtr(16);
									sc_time localTime = sc_time_stamp();
									/*Create CQ1 entry*/
									createCQ1Entry(cid, cmd, mPhaseTagCQ1, 0x0, mSQ1Queue.getHead(), 0x1, data);

									
									/* Create Completion Queue data structure and
									send memory write TLP*/
									uint64_t address;
									uint32_t qAddr = 0;

									/*Get the address of CQ1 queue entry to write to*/
									address = mCQ1Queue.getTail();
									mCQ1Queue.push();
									/*Create Memory Write TLP for Completion Queue*/
									createMemWriteTLP(address, hostQueueType::COMPLETION, qAddr, data);
									
									//mArbiterToken.push(2);
									/*Notify to arbitrate the TLP on the bus*/
									mArbitrateEvent.notify(SC_ZERO_TIME);
									mHCmdQNotFullEvent.notify(SC_ZERO_TIME);

									mNumCmd++;
									msg.str("");
									msg << "TeraSPCIeController::READ CMD COMPLETION QUEUE PUSH"
										<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
									REPORT_INFO(filename, __FUNCTION__, msg.str());

									mLogFileHandler << "READ CMD COMPLETION QUEUE PUSH: "
										<< " @Time= " << dec << (sc_time_stamp().to_double()) << " ns"
										<< endl;
								}
							}
							break;
				}
				default :
					break;
				}
			}//else
		}//while(1)
	}

#pragma endregion DYNAMIC_THREADS

#pragma region DYNAMIC_METHODS

	void TeraSPCIeController::pendingWriteCmdMethod(uint8_t chanNum)
	{
		ActiveCmdQueueData cmd;
		cmdType ctype;
		uint8_t lba;
		uint8_t queueAddr;
		uint32_t iTag;
		bool chipSelect;
		std::ostringstream msg;

		next_trigger(mPendingCmdQueue.at(chanNum)->get_event());
		while ((mPendingCmdQueue.at(chanNum)->popCommand(cmd)) != false)
		{
			uint32_t cwBankMask = getCwBankMask();
			uint8_t cwBankIndex = mPendingCmdQueue.at(chanNum)->getCwBankNum(cwBankMask);
			mPendingCmdQueue.at(chanNum)->erase();

			/*Decode the pending command queue entry field*/
			decodePendingCmdEntry(cmd, ctype, lba, iTag, chipSelect, queueAddr);

			if (ctype == WRITE)
			{
				/*Find out the cwBankIndex to make the bank status free*/
				if ((cwBankIndex == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCwSize / mPageSize)))
				{
					cwBankIndex = 0;
					chipSelect = 1;

				}
				if ((chipSelect == 1) && (mNumDie == 1))
				{
					chipSelect = 0;
				}
				if (chipSelect && (mNumDie > 1))
					cwBankIndex = cwBankIndex + mCodeWordNum / mNumDie;

				/* Free the corresponding bank status*/
				mCwBankStatus.at(chanNum).at(cwBankIndex) = cwBankStatus::BANK_FREE;

				/*Notify Command Dispatcher of the free bank status*/
				mTrigCmdDispEvent.at(chanNum)->notify(SC_ZERO_TIME);

				/*Set PCMD Queue status to free*/
				mCmdQueue.setQueueFree(cmd.queueNum);

				/*Notify the presence of space in the PCMD queue*/
				mCmdQNotFullEvent.notify(SC_ZERO_TIME);

				msg.str("");
				msg << "PENDING QUEUE PUSH: WRITE COMMAND: "
					<< "  @Time: " << dec << sc_time_stamp().to_double()
					<< " LBA =0x" << hex << (uint32_t)cmd.plba
					<< " Command Type: " << hex << (uint32_t)cmd.cmd;

				REPORT_INFO(filename, __FUNCTION__, msg.str());

				mLogFileHandler << "PENDING QUEUE PUSH::WRITE COMMAND: "
					<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
					<< " LBA =0x" << hex << (uint32_t)cmd.plba
					<< " Command Type= 0x" << hex << (uint32_t)cmd.cmd
					<< endl;

				/*Decrement running count*/
				mRunningCntTable.updateCnt(iTag);
				
				/*Get the running count*/
				uint16_t cnt = mRunningCntTable.getRunningCnt(iTag);
				mNumCmdDispatched++;
				if (!cnt)
				{
					/*If the cnt goes to zero, Command has finished*/
					mTagQueue.push(cmd.iTag);
					
					/*Notify the Write Cmd Done thread to process the write command and
					send it to completion queue*/
					mWriteCmdDoneEvent.notify(SC_ZERO_TIME);

				}
			}//if (ctype == WRITE)

		}//while

	}

	void TeraSPCIeController::pendingReadCmdMethod(uint8_t chanNum)
	{
		ActiveCmdQueueData cmd;
		cmdType ctype;
		uint8_t lba;
		uint32_t iTag;
		bool chipSelect;
		uint8_t queueAddr;
				
		std::ostringstream msg;

		next_trigger(mPendingReadCmdQueue.at(chanNum)->get_event());
		while (((mPendingReadCmdQueue.at(chanNum)->popCommand(cmd)) != false))
		{
			if (!mAdcq.at(chanNum).isFull())
			{

				mPendingReadCmdQueue.at(chanNum)->erase();

				/*Decode pending queue entry*/
				decodePendingCmdEntry(cmd, ctype, lba, iTag, chipSelect, queueAddr);

				if (ctype == READ)
				{
					uint8_t qAddr;
					mReadDataBuff.getFreeBuffPtr(qAddr);
					mReadDataBuff.setBuffBusy(qAddr);
					sc_core::sc_time delay = sc_time_stamp();
					ActiveDMACmdQueueData value;
					value.cmd = cmd.cmd;
					value.time = cmd.time + delay;
					value.cmdOffset = cmd.cmdOffset;
					value.buffPtr = qAddr;
					value.iTag = cmd.iTag;
					value.plba = cmd.plba;
					value.queueNum = cmd.queueNum;

					/*Push READ Command into the DMA queue*/
					mAdcq.at(chanNum).pushQueue(value);
					
					//plba |= ((uint64_t)chanNum & 0x0F);
					msg.str("");
					msg << "PENDING QUEUE PUSH:: READ COMMAND: "
						<< "  @Time: " << dec << sc_time_stamp().to_double()
						<< " LBA =0x" << hex << (uint32_t)cmd.plba
						<< " Command Type: " << hex << (uint32_t)cmd.cmd
						<< " Channel Number: " << dec << (uint32_t)chanNum;

					REPORT_INFO(filename, __FUNCTION__, msg.str());

					mLogFileHandler << "PENDING QUEUE PUSH::READ COMMAND: "
						<< "  @Time= " << dec << sc_time_stamp().to_double() << " ns"
						<< " LBA =0x" << hex << (uint32_t)cmd.plba
						<< " Command Type: " << hex << (uint32_t)cmd.cmd
						<< " Channel Number= " << dec << (uint32_t)chanNum
						<< endl;

					/*Notify that there is command in the ADCQ*/
					mAdcqNotEmptyEvent.at(chanNum)->notify(SC_ZERO_TIME);

				}//if(READ)
			}
			else
			{
				next_trigger(*mAdcqNotFullEvent.at(chanNum));
				break;
			}

		}//while

	}

	void TeraSPCIeController::checkRespMethod(uint8_t chanNum)
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
				mLogFileHandler << "WRITE COMPLETE: "
					<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
					<< " Transaction Number: " << dec << mTotalWrites;
			}
			else if (payloadPtr->is_read()){

				mAvailableCredit.at(chanNum)++;
				mEndReadEvent.at(chanNum)->notify(SC_ZERO_TIME);
				mTotalReads++;
				msg.str("");
				msg << "READ COMPLETE: "
					<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
					<< " Transaction Number: " << dec << mTotalReads;
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				mLogFileHandler << "READ COMPLETE: "
					<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
					<< " Transaction Number: " << dec << mTotalReads;
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
#pragma endregion DYNAMIC_METHODS

#pragma region INTERFACE_METHODS_HELPER
	void TeraSPCIeController::writeCCReg(uint64_t payload)
	{
		mLogFileHandler << "TeraSPCIeController::CC REGISTER WRITE: "
			<< " CC REGISTER VALUE: 0x" << hex << payload
			<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
		regCC = SET_CC_IOCQES(regCC, payload);
		regCC = SET_CC_EN(regCC, payload);
		regCC = SET_CC_IOSQES(regCC, payload);
		regCC = SET_CC_MPS(regCC, payload);
		regCC = SET_CC_CSS(regCC, payload);
		mCC_EnEvent.notify(SC_ZERO_TIME);
	}
	void TeraSPCIeController::writeAQAReg(uint64_t payload)
	{
		mLogFileHandler << "TeraSPCIeController::AQA REGISTER WRITE: "
			<< " AQA REGISTER VALUE: 0x" << hex << payload
			<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
		regAQA = SET_AQA_ASQS(regAQA, payload);
		regAQA = SET_AQA_ACQS(regAQA, payload);
	}
	void TeraSPCIeController::writeASQReg(uint64_t payload)
	{
		mLogFileHandler << "TeraSPCIeController::ASQ REGISTER WRITE: "
			<< " ASQ REGISTER VALUE: 0x" << hex << payload
			<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
		regASQ = SET_ASQB(regASQ, payload);
	}
	void TeraSPCIeController::writeACQReg(uint64_t payload)
	{
		mLogFileHandler << "TeraSPCIeController::ACQ REGISTER WRITE: "
			<< " ACQ REGISTER VALUE: 0x" << hex << payload
			<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
		regACQ = SET_ACQB(regACQ, payload);
	}
	void TeraSPCIeController::writeSQ0TDBLReg(uint64_t payload, sc_time delay)
	{
		std::ostringstream msg;
		bool value = (bool)GET_CSTS_RDY(regCSTS);
		if (value)
		{
			regSQ0TDBL = payload;

			msg.str(" ");
			msg << "SQ0 TAIL DOORBELL RING: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regSQ0TDBL;

			REPORT_INFO(filename, __FUNCTION__, msg.str());
			mLogFileHandler << "SQ0 TAIL DOORBELL RING: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regSQ0TDBL
				<< endl;
			mSQ0TdblEvent.notify(SC_ZERO_TIME);
		}
	}
	void TeraSPCIeController::writeCQ0HDBLReg(uint64_t payload, sc_time delay)
	{
		std::ostringstream msg;
		regCQ0HDBL = payload;

		msg.str(" ");
		msg << "CQ0 HEAD DOORBELL RING: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regCQ0HDBL
			<< endl;
		REPORT_INFO(filename, __FUNCTION__, msg.str());
		mLogFileHandler << "CQ0 HEAD DOORBELL RING: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regCQ0HDBL
			<< endl;
		mCQ0HdblEvent.notify(SC_ZERO_TIME);
	}
	void TeraSPCIeController::writeSQ1TDBLReg(uint64_t payload, sc_time delay)
	{
		std::ostringstream msg;
		bool value = (bool)GET_CSTS_RDY(regCSTS);
		if (value)
		{
			regSQ1TDBL = payload;
			/*if (!isDoorBellSet)
			{*/
				mSQ1Queue.setTail(regSQ1TDBL);
			/*	isDoorBellSet = true;
			}*/
			mTrigMemReadCmdEvent.notify(SC_ZERO_TIME);
			msg.str(" ");
			msg << "SQ1 TAIL DOORBELL RING: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regSQ1TDBL
				<< endl;
			REPORT_INFO(filename, __FUNCTION__, msg.str());
			mLogFileHandler << "SQ1 TAIL DOORBELL RING: "
				<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
				<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regSQ1TDBL
				<< endl;
		}
	}
	void TeraSPCIeController::writeCQ1HDBLReg(uint64_t payload, sc_time delay)
	{
		std::ostringstream msg;
		regCQ1HDBL = payload;
		mCQ1Queue.setHead(regCQ1HDBL);
		mCQ1HdblEvent.notify(SC_ZERO_TIME);
		msg.str(" ");
		msg << "CQ1 HEAD DOORBELL RING: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regCQ1HDBL
			<< endl;
		mLogFileHandler << "CQ1 HEAD DOORBELL RING: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
			<< " DOORBELL REG VALUE=0x" << hex << (uint32_t)regCQ1HDBL
			<< endl;
	}

	void TeraSPCIeController::processCompletionTLPCmd(uint8_t* dataPtr, sc_time delay)
	{
		std::ostringstream msg;
		/* Check the TAG */
		tCompletionTLP* completionTLP = new tCompletionTLP();
		memcpy(completionTLP, reinterpret_cast<tCompletionTLP*> (dataPtr), sizeof(tCompletionTLP));

		HCmdQueueData* hCmdEntry = mHostCmdQueue.acquire();
		tSubQueue* sqEntry = new tSubQueue();
		uint32_t tag = 0;
		uint32_t totalCmd = mLength / 16;
		
		/*Save the SQ entry in Host command queue and also replicate the command
		in the physical command queue, */
		for (uint32_t numCmdIndex = 0; numCmdIndex < totalCmd; numCmdIndex++)
		{
			if (!mHostCmdQueue.isFull())
			{
				hCmdEntry->reset();

				/* Parse the SQ entry read from completion TLP payload
				and store the value in Host command Field*/
				parseSQFromCompletionTLP(sqEntry, hCmdEntry, numCmdIndex, completionTLP);

				uint16_t cid = hCmdEntry->cid;
				uint8_t cmd = hCmdEntry->hCmd;
				uint64_t lba = hCmdEntry->lba;
				uint16_t blkCnt = (hCmdEntry->cnt);
				uint64_t hAddr0 = hCmdEntry->hAddr0;
				uint64_t hAddr1 = hCmdEntry->hAddr1;
				uint32_t cmdOffset = 0;
				msg.str(" ");
				msg << "HOST COMMANDS: "
					<< " CID: 0x" << hex << cid
					<< " CMD: 0x" << hex << cmd
					<< " LBA: 0x" << hex << lba
					<< " BLOCK COUNT: 0x" << hex << blkCnt
					<< " LOWER ADDRESS: 0x" << hex << hAddr0
					<< " HIGHER ADDRESS: 0x" << hex << hAddr1
					<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns";

				REPORT_INFO(filename, __FUNCTION__, msg.str());
				mLogFileHandler << "HOST COMMANDS: "
					<< " CID: 0x" << hex << cid
					<< " CMD: 0x " << hex << cmd
					<< " LBA: 0x " << hex << lba
					<< " BLOCK COUNT: 0x" << hex << blkCnt
					<< " LOWER ADDRESS: 0x" << hex << hAddr0
					<< " HIGHER ADDRESS: 0x" << hex << hAddr1
					<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
					<< endl;


				/*get free iTag from Tag Manager*/
				mTagMgr.getFreeTag(tag);
				/*set the iTag to busy state*/
				mTagMgr.setTagBusy(tag);

				//mNumCmd++;
				/*Push the command in host command queue*/
				mHostCmdQueue.push(hCmdEntry, tag);

				/*Set the host command queue busy*/
				mHostCmdQueue.setQueueBusy(tag);

				/*Insert parameters in the parameter table indexed by iTag*/
				mParamTable.insertParam(tag, cid, (cmdType&)cmd, lba, blkCnt, hAddr0, hAddr1); //Save the cmd parameters to param table

				/* Calculate the number of sub commands to be added in the physical command queue*/
				uint32_t indexSize = (uint32_t)(hCmdEntry->cnt);

				/*Insert the count in the running table*/
				mRunningCntTable.insert(tag, indexSize);

				/*Push the tag, count and host command entry to be used by
				Physical command queue parser thread to parse the commands
				into sub commands*/
				mHCmdQueue.push(*hCmdEntry);
				mCmdSize.push(indexSize);
				mFreeTagQueue.push(tag);
				mDelayQueue.push(delay);

				/*Notify physical command queue parser thread*/
				mTrigPcmdQEvent.notify(SC_ZERO_TIME);

			}
			else {
				wait(mHCmdQNotFullEvent);
				numCmdIndex--;
			}

		}
		mCompletionTLPRxEvent.notify(SC_ZERO_TIME);
		delete sqEntry;
		delete completionTLP;
	}
#pragma endregion

#pragma region INTERFACE_METHODS

	void TeraSPCIeController::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
	{
		uint8_t* dataPtr = trans.get_data_ptr();
		uint64_t addr = trans.get_address();
		//mLength = trans.get_data_length();
		uint32_t address;
		tlpType tlpType;
		uint16_t length;
		uint16_t byteCnt;
		uint16_t compID;
		uint16_t reqID;
		uint8_t lwrAddr;
		uint8_t tag = 0;
		bool isValidTLP;
		std::ostringstream msg;

		bool isValidType = decodeTLPType(reinterpret_cast<tDw0MemoryTLP*>(dataPtr), length, tlpType);

		if (tlpType == tlpType::CFG_WRITE)
		{
			
			uint64_t payload;
			getCfgWrPayload(reinterpret_cast<tConfigWriteReq*>(dataPtr), address, payload);
			address += BA_CNTL;
			msg.str("");
			msg << "REGISTER WRITE::ADDRESS:0x" << hex << address
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
			REPORT_INFO(filename, __FUNCTION__, msg.str());
			mLogFileHandler << "TeraSPCIeController::REGISTER WRITE: "
				<< "ADDRESS: 0x" << hex << address
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
			
			switch (address)
			{

			case INTMS_ADD:
			{
							  break;
			}
			case INTMC_ADD:
			{
							  break;
			}
			case CC_ADD:
			{
						   writeCCReg(payload);
						   break;
			}
			case RES_ADD:
			{
							break;
			}

			case NSSR_ADD:
			{
							 break;
			}
			case AQA_ADD:
			{
							writeAQAReg(payload);
							break;
			}
			case ASQ_ADD:
			{
							writeASQReg(payload);
							break;
			}
			case ACQ_ADD:
			{
							writeACQReg(payload);
							break;
			}
			case SQ0TDBL_ADD:
			{
							writeSQ0TDBLReg(payload, delay);
							break;
			}
			case CQ0HDBL_ADD:
			{
							writeCQ0HDBLReg(payload, delay);
							break;
			}
			case SQ1TDBL_ADD:
			{
							writeSQ1TDBLReg(payload, delay);
							break;
			}
			case CQ1HDBL_ADD:
			{
						    writeCQ1HDBLReg(payload, delay);
						    break;
			}
			default:
			{
					   break;
			}
			}
		}

		else if (tlpType == COMPLETION_TLP)
		{
			/* Parse Completion TLP*/
			getCompletionTLPFields(reinterpret_cast<tCompletionTLP*> (dataPtr), byteCnt, compID, reqID, \
				lwrAddr, tag);

			msg.str("");
			msg << "RECEIVED COMPLETION TLP: "
				<< " BYTE COUNT: 0x"<< hex << byteCnt
				<< " COMPLETER ID: 0x" << hex << compID
				<< " REQUESTOR ID: 0x" << hex << reqID
				<< " ADDRESS: 0x" << hex << lwrAddr
				<< " TAG: 0x" << hex << (uint32_t)tag
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
			REPORT_INFO(filename, __FUNCTION__, msg.str());

			mLogFileHandler << "RECEIVED COMPLETION TLP: "
				<< " BYTE COUNT: 0x" << hex << byteCnt
				<< " COMPLETER ID: 0x" << hex << compID
				<< " REQUESTOR ID: 0x" << hex << reqID
				<< " ADDRESS: 0x" << hex << (uint32_t)lwrAddr
				<< " TAG: 0x" << hex << (uint32_t)tag
				<< "  @Time= " << dec << sc_time_stamp().to_double() << endl;
			/*Check if completion TLP is valid*/
			if (compID != 0x100 && reqID != 0x0)
			{
				isValidTLP = false;
			}
			else
				isValidTLP = true;

			/*Set TLP response*/
			if (isValidType && isValidTLP)
			{
				trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);
			}
			else
			{
				trans.set_response_status(tlm::tlm_response_status::TLM_COMMAND_ERROR_RESPONSE);
				return;
			}


			if (((tag & TAG_MASK) >> 6) == 0x1)
			{
				
				processCompletionTLPCmd(dataPtr, delay);

			}
			else if (((tag & TAG_MASK) >> 6) == 0x0)
			{
				//mCntrlState = CONTROLLER_STATES::ADMIN_COMPLETION_READ_REQ;
			}
			else if (((tag & TAG_MASK) >> 6) == 0x2 || ((tag & TAG_MASK) >> 6) == 0x3)
			{
				/*Completion TLP with Data*/
				tCompletionTLP* completionTLP = new tCompletionTLP();
				memcpy(completionTLP, reinterpret_cast<tCompletionTLP*> (dataPtr), sizeof(tCompletionTLP));
				
				//mNumCmdDispatched++;
				/*Parse the data and store in the appropriate Write Data Buffer*/
				parseData(completionTLP);
				delete completionTLP;
				msg.str(" ");
				msg << "COMPLETION TLP WITH DATA: "
					<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns";
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				
				mLogFileHandler << "COMPLETION TLP WITH DATA: "
					<< " @Time= " << dec << (sc_time_stamp().to_double() + delay.to_double()) << " ns"
					<< endl;
			}
			else
			{
				trans.set_response_status(tlm::tlm_response_status::TLM_COMMAND_ERROR_RESPONSE);
				return;
			}
		}

	}
	
	tlm::tlm_sync_enum TeraSPCIeController::nb_transport_bw(int id, tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_time& delay)
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
								  mLogFileHandler << " BEGIN_RESP: "
									  << " Channel Number: " << dec << (uint32_t)id
									  << " @Time: " << dec << sc_time_stamp().to_double() << " ns"
								      << endl;
								  returnStatus = tlm::TLM_ACCEPTED;
								  break;
		}//end case
		case (tlm::END_REQ) :
		{
								msg.str("");
								msg << " END_REQ: "
									<< " Channel Number: " << dec << (uint32_t)id
									<< " @Time: " << dec << sc_time_stamp().to_double() << " ns";
								REPORT_INFO(filename, __FUNCTION__, msg.str());
								mLogFileHandler << " END_REQ: "
									<< " Channel Number: " << dec << (uint32_t)id
									<< " @Time: " << dec << sc_time_stamp().to_double() << " ns"
									<< endl;
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

#pragma endregion INTERFACE_METHODS

#pragma region RRAM_IF_METHODS

	void TeraSPCIeController::setCmdTransferTime(const uint8_t& code, sc_core::sc_time& tDelay, sc_core::sc_time& delay, uint8_t chanNum)
	{
		sc_time currTime;
		double writeCmdDataTransSpeed = (double)(mCmdTransferTime + (double)((mCwSize)* 1000) / mOnfiSpeed); //NS
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
			double readDataTransSpeed = (double)((double)(mCwSize * 1000) / mOnfiSpeed);

			currTime = sc_time_stamp() + delay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
			currTime = sc_time_stamp() + delay + tDelay;
			mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, " ");
			mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, ",");
		}
	}


	void TeraSPCIeController::RRAMInterfaceMethod(ActiveCmdQueueData& cmd, const uint8_t& chanNum, uint8_t* data, sc_core::sc_time& delay,
		const uint8_t& code)
	{
		bool chipSelect;
		uint8_t cwBank;
		uint32_t page;
		cmdType ctype;
		tlm::tlm_generic_payload *payloadPtr;
		sc_time tDelay = SC_ZERO_TIME;

		/* Calculate Command Transfer Speed*/
		double writeCmdDataTransSpeed = (double)(mCmdTransferTime + (double)((mCwSize)* 1000) / mOnfiSpeed); //NS
		//double writeCmdTransSpeed = (double)((double)((ONFI_COMMAND_LENGTH) * 1000) / mOnfiSpeed);
		double readBankCmdTransSpeed = (double)((double)(ONFI_BANK_SELECT_CMD_LENGTH * 1000 * 2) / mOnfiSpeed); //NS
		decodeCommand(cmd, ctype, cwBank, chipSelect, page);

		/* Transaction Manager*/
		payloadPtr = mTransMgr.allocate();
		payloadPtr->acquire();

		/*Create ONFI payload*/
		createPayload(chipSelect, ctype, cwBank, page, code, data, payloadPtr);

		double_t startTime = 0;

		/*Log ONFI Activity to calculate Channel utilization*/
		if (mOnfiChanActivityStart.at(chanNum) == false)
		{
			mOnfiStartActivity.at(chanNum) = sc_time_stamp().to_double() + delay.to_double();
			mOnfiChanActivityStart.at(chanNum) = true;
			startTime = delay.to_double();
		}

		tlm::tlm_sync_enum returnStatus;

		/*Enable chip Select*/
		pChipSelect.at(chanNum)->write(chipSelect);
		wait(SC_ZERO_TIME);

		tlm::tlm_phase phase = tlm::BEGIN_REQ;
		sc_time currTime;

		/*Log ONFI command transfer time*/
		setCmdTransferTime(code, tDelay, delay, chanNum);
		//if (code == ONFI_WRITE_COMMAND)
		//{
		//	/*ONFI WRITE*/
		//	tDelay = sc_time(writeCmdDataTransSpeed, SC_NS) + delay;
		//	mOnfiChanTime.at(chanNum) = (mCmdTransferTime - delay.to_double());
		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanTime.at(chanNum) = ((writeCmdDataTransSpeed - mCmdTransferTime) - delay.to_double());
		//	mOnfiChanUtilReport->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//}
		//else if (code == ONFI_READ_COMMAND) {

		//	/*ONFI READ*/
		//	tDelay = sc_time(mCmdTransferTime, SC_NS) + delay;
		//	mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());

		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//}
		//else if (code == ONFI_BANK_SELECT_COMMAND) {

		//	/*ONFI BANK SELECT*/
		//	tDelay = sc_time(readBankCmdTransSpeed, SC_NS);
		//	mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());
		//	double readDataTransSpeed = (double)((double)(mCwSize * 1000) / mOnfiSpeed);

		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//	currTime = sc_time_stamp() + delay + tDelay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, " ");
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, ",");
		//}

		/* Send nb_transport*/
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

	void TeraSPCIeController::RRAMInterfaceMethod(ActiveDMACmdQueueData& cmd, const uint8_t& chanNum, uint8_t* data, sc_core::sc_time& delay,
		const uint8_t& code)
	{
		bool chipSelect;
		uint8_t cwBank;
		uint32_t page;
		cmdType ctype;
		tlm::tlm_generic_payload *payloadPtr;
		sc_time tDelay = SC_ZERO_TIME;

		/* Calculate Command Transfer Speed*/
		double writeCmdDataTransSpeed = (double)(mCmdTransferTime + (double)((mCwSize)* 1000) / mOnfiSpeed); //NS
		//double writeCmdTransSpeed = (double)((double)((ONFI_COMMAND_LENGTH) * 1000) / mOnfiSpeed);
		double readBankCmdTransSpeed = (double)((double)(ONFI_BANK_SELECT_CMD_LENGTH * 1000 * 2) / mOnfiSpeed); //NS
		decodeCommand(cmd, ctype, cwBank, chipSelect, page);

		/* Transaction Manager*/
		payloadPtr = mTransMgr.allocate();
		payloadPtr->acquire();

		/*Create ONFI payload*/
		createPayload(chipSelect, ctype, cwBank, page, code, data, payloadPtr);

		double_t startTime = 0;

		/*Log ONFI Activity to calculate Channel utilization*/
		if (mOnfiChanActivityStart.at(chanNum) == false)
		{
			mOnfiStartActivity.at(chanNum) = sc_time_stamp().to_double() + delay.to_double();
			mOnfiChanActivityStart.at(chanNum) = true;
			startTime = delay.to_double();
		}
		tlm::tlm_sync_enum returnStatus;

		/*Enable chip Select*/
		pChipSelect.at(chanNum)->write(chipSelect);
		wait(SC_ZERO_TIME);
		//mReqInProgressPtr = payloadPtr;
		tlm::tlm_phase phase = tlm::BEGIN_REQ;

		sc_time currTime;

		/*Log ONFI command transfer time*/
		setCmdTransferTime(code, tDelay, delay, chanNum);
		//if (code == ONFI_WRITE_COMMAND)
		//{
		//	tDelay = sc_time(writeCmdDataTransSpeed, SC_NS) + delay;
		//	mOnfiChanTime.at(chanNum) = (mCmdTransferTime - delay.to_double());
		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanTime.at(chanNum) = ((writeCmdDataTransSpeed - mCmdTransferTime) - delay.to_double());
		//	mOnfiChanUtilReport->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double() + mCmdTransferTime, chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//}
		//else if (code == ONFI_READ_COMMAND) {

		//	tDelay = sc_time(mCmdTransferTime, SC_NS) + delay;
		//	mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());

		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//}
		//else if (code == ONFI_BANK_SELECT_COMMAND) {

		//	tDelay = sc_time(readBankCmdTransSpeed, SC_NS);
		//	mOnfiChanTime.at(chanNum) = (tDelay.to_double() - delay.to_double());
		//	double readDataTransSpeed = (double)((double)(mCwSize * 1000) / mOnfiSpeed);

		//	currTime = sc_time_stamp() + delay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), " " /*+ delay.to_double()*/);
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, mOnfiChanTime.at(chanNum), "," /*+ delay.to_double()*/);
		//	currTime = sc_time_stamp() + delay + tDelay;
		//	mOnfiChanUtilReport->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, " ");
		//	mOnfiChanUtilReportCsv->writeFile(currTime.to_double(), chanNum, readDataTransSpeed, ",");
		//}

		/* Send nb_transport*/
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

#pragma endregion RRAM_IF_METHODS

#pragma region MEMBER_FUNCTIONS

	uint32_t TeraSPCIeController::getMemPageSize()
	{
		uint8_t mps = GET_CC_MPS(regCC);
		return (uint32_t)pow(2, (12 + mps));
	}

	void TeraSPCIeController::PCIeAdminCntrlrMethod()
	{

	}

	bool TeraSPCIeController::decodeTLPType(tDw0MemoryTLP* pldType, uint16_t& length, \
		tlpType& tlpType)
	{
		uint8_t type = pldType->type;
		uint8_t format = pldType->format;
		length = pldType->length;
		if (type == 0x0 && format == 0x0)
		{
			tlpType = tlpType::MEM_READ_REQ;
			return true;
		}
		else if (type == 0x0 && format == 0x2)
		{
			tlpType = tlpType::MEM_WRITE_REQ;
			return true;
		}
		else if (type == 0xa && format == 0x2)
		{
			tlpType = tlpType::COMPLETION_TLP;
			return true;
		}
		else if (type == 0x4 && format == 0x2)
		{
			tlpType = tlpType::CFG_WRITE;
			return true;
		}
		else
			return false;
	}

	void TeraSPCIeController::getCfgWrPayload(tConfigWriteReq* data, uint32_t& address, uint64_t& payload)
	{
		address = data->address;
		payload = data->data;
	}

	void TeraSPCIeController::parseSQFromCompletionTLP(tSubQueue* sqEntry, HCmdQueueData* hCmdEntry, uint32_t index, tCompletionTLP * completionTLP)
	{

		memcpy(sqEntry, reinterpret_cast<tSubQueue*> (completionTLP->data + 64 * index), 64);
		if ((sqEntry->DW0.opcode & 0xff) == 0x02)
		{
			hCmdEntry->hCmd = cmdType::READ;
		}
		else
		{
			hCmdEntry->hCmd = cmdType::WRITE;
		}
		hCmdEntry->cid = sqEntry->DW0.CID & 0xffff;
		hCmdEntry->lba = ((uint64_t)sqEntry->Dw10 | ((uint64_t)sqEntry->Dw11 << 32)) & 0xffffffffffffffffll;
		hCmdEntry->cnt = sqEntry->Dw12 & 0xffff;
		if (sqEntry->DW0.PSDT == 0x00)
		{
			hCmdEntry->hAddr0 = sqEntry->dptr0;
			hCmdEntry->hAddr1 = 0x0;
		}
		else if (sqEntry->DW0.PSDT == 0x01 || sqEntry->DW0.PSDT == 0x10)
		{
			hCmdEntry->hAddr0 = sqEntry->dptr0;
			hCmdEntry->hAddr1 = sqEntry->dptr1;
		}
		//dataOut->cnt = sqData->get

	}

	void TeraSPCIeController::parseData(tCompletionTLP* completionTLP)
	{
		//uint8_t* data = mWriteDataBuff.acquire();
		//memcpy(data, completionTLP->data, mLength * 4);
		uint8_t qAddr = 0;
		for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
		{
			//mohit edit
			if (!mWriteBuffAddr.at(chanIndex).empty()){
				/*Do not pop this queue , it is popped at longQueuethread*/
				qAddr = mWriteBuffAddr.at(chanIndex).front();
			}
			else{
				continue;
			}
			//mohit edit ends
			/*Do not pop this queue , it is popped at longQueuethread*/
			//qAddr = mWriteBuffAddr.at(chanIndex).front();
			//mWriteBuffAddr.at(chanIndex).pop();
			if (qAddr == (completionTLP->Dw2CompletionTLP.tag & 0x3f))
			{
				mWriteDataBuff.writeData(qAddr, completionTLP->data);
				mCmplTLPDataRxEvent.at(chanIndex)->notify(SC_ZERO_TIME);
				break; 
			}
		}

	}

	void TeraSPCIeController::TeraSBankLLMethod(int32_t qAddr)
	{
		uint8_t chanNum, cwBank, chipSelect;
		uint32_t pageNum;
		uint64_t genLba = 0;
		std::ostringstream msg;

		/*Get the LBA fields: channel number, CW Bank, chip select, page number*/
		mCmdQueue.getLBAField(qAddr, chanNum, cwBank, chipSelect, pageNum);
		if ((mCwBankMaskBits == 1) && (cwBank == 1) && (mBankNum <= (mCwSize / mPageSize)))
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
		msg << " LBA =0x" << hex << (uint64_t)genLba
			<< " Channel Number= " << hex << (uint32_t)chanNum
			<< " CW Bank= " << hex << (uint32_t)cwBank
			<< " Chip Select= " << hex << (uint32_t)chipSelect
			<< " Page Address= " << hex << (uint32_t)pageNum;
		REPORT_INFO(filename, __FUNCTION__, msg.str());

		mLogFileHandler << " BANK LL Manager:: "
			<< " @Time= " << dec << (sc_time_stamp().to_double() + mDelay.to_double()) << " ns"
			<< " LBA =0x" << hex << (uint64_t)genLba
			<< " Channel Number= " << dec << (uint32_t)chanNum
			<< " CW Bank= " << dec << (uint32_t)cwBank
			<< " Chip Select= " << dec << (uint32_t)chipSelect
			<< " Page Address= 0x" << hex << (uint32_t)pageNum
			<< endl;

		if (chipSelect && (mNumDie > 1))
			cwBank = cwBank + mCodeWordNum / mNumDie;

		/*If link list is empty for the parsed channel then
		update both head and tail*/
		if (mBankLinkList.getHead(chanNum, cwBank) == -1) //Link List Empty
		{
			mBankLinkList.updateHead(chanNum, cwBank, qAddr);
			mBankLinkList.updateTail(chanNum, cwBank, qAddr);

		}
		/* else read the tail and update the tail with the current address of the
		physical command queue*/
		else { //Read Tail and update the tail with the current address
			int32_t tailAddr;
			mBankLinkList.getTail(chanNum, cwBank, tailAddr);
			mCmdQueue.updateNextAddress(tailAddr, qAddr);
			mBankLinkList.updateTail(chanNum, cwBank, qAddr);
		}

		/*Notify corresponding channel's Command Dispatcher thread*/
		mCmdDispatchEvent.at(chanNum)->notify(SC_ZERO_TIME);
	}

	void TeraSPCIeController::createCQ1Entry(uint16_t cid, cmdType cmd, bool phaseTag, uint16_t status, uint16_t sq1Head, uint16_t sqID, uint8_t* data)
	{
		tCompletionQueue cq1;
		cq1.Dw0.CID = cid;
		if (cmd == cmdType::READ)
		cq1.Dw0.opcode = (uint8_t)0x2;
		else
			cq1.Dw0.opcode = (uint8_t)0x1;
		cq1.Dw0.PSDT = 0x0;
		cq1.Dw0.fuse = 0x0;

		cq1.Dw2.SQHPTR = sq1Head;
		cq1.Dw2.SQID = sqID;
		cq1.Dw3.CID = cid;
		cq1.Dw3.PT = phaseTag;
		cq1.Dw3.SF = status;

		/*check if this is right while debugging*/
		memcpy(data, reinterpret_cast<uint8_t*>(&cq1), 16);

	}

	void TeraSPCIeController::createPayload(bool& chipSelect, const cmdType ctype, uint8_t cwBank, \
		uint32_t page, const uint8_t& code, uint8_t* dataPtr, tlm::tlm_generic_payload* payloadPtr)
	{
		uint64_t addr = getAddress(page, cwBank, chipSelect);
		addr = ((uint64_t)code << 56) | addr;

		payloadPtr->set_address(addr);
		payloadPtr->set_data_ptr(dataPtr);
		payloadPtr->set_data_length(mCwSize);
		if (ctype == WRITE)
			payloadPtr->set_command(tlm::TLM_WRITE_COMMAND);
		else
			payloadPtr->set_command(tlm::TLM_READ_COMMAND);
	}

	
	uint64_t TeraSPCIeController::getAddress(uint32_t& page, uint8_t& bank, bool& chipSelect)
	{
		if ((bank == 1) && (mCwBankMaskBits == 1) && (mBankNum <= (mCwSize / mPageSize)))
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
		if (mCwSize != mPageSize)
		{
			bankBitShifter = (uint32_t)(log2(mCwSize / mPageSize));
		}
		else {
			bankBitShifter = 0;
		}

		uint8_t memBank = (uint8_t)((bank << bankBitShifter) & mBankMask);
		uint32_t memPage = (uint32_t)((page << ((uint32_t)(log2(mPageSize)))) & mPageMask);

		uint64_t address = ((uint64_t)(memBank) | ((uint64_t)(memPage) << mBankMaskBits));
		return address;
	}

	void TeraSPCIeController::getCompletionTLPFields(tCompletionTLP* data, uint16_t& byteCnt, uint16_t& compID, uint16_t& reqID, \
		uint8_t& lwrAddr, uint8_t& tag)
	{
		mLength = data->Dw0CompletionTLP.length;
		byteCnt = data->Dw1CompletionTLP.byteCount;
		compID = data->Dw1CompletionTLP.completerID;
		reqID = data->Dw2CompletionTLP.requesterID;
		lwrAddr = data->Dw2CompletionTLP.lowerAddress;
		tag = data->Dw2CompletionTLP.tag;
	}

	void TeraSPCIeController::createDMAQueueCmdEntry(ActiveCmdQueueData& activeQueue, uint8_t& qAddr, ActiveDMACmdQueueData& qData)
	{
		qData.buffPtr = qAddr;
		qData.cmd = activeQueue.cmd;
		qData.cmdOffset = activeQueue.cmdOffset;
		qData.iTag = activeQueue.iTag;
		qData.plba = activeQueue.plba;
		qData.queueNum = activeQueue.queueNum;
		qData.time = activeQueue.time;
	}
#pragma endregion MEMBER_FUNCTIONS

#pragma region TLP_CREATION_METHODS

void TeraSPCIeController::createMemReadTLP(uint64_t& address, uint32_t length, const tagType tag, uint8_t& chanNum)
{

	tMemoryReadReqTLP memReadTLP = {0};
	//memReadTLP.address = mSQ1Queue.getHead();
	memReadTLP.address = address;
	memReadTLP.Dw0MemReadReqTLP.length = length;
	/*memReadTLP.Dw0MemReadReqTLP.attribute = 0x00;
	memReadTLP.Dw0MemReadReqTLP.EP = 0x0;*/
	memReadTLP.Dw0MemReadReqTLP.format = 0x00;
	/*memReadTLP.Dw0MemReadReqTLP.TD = 0x0;
	memReadTLP.Dw0MemReadReqTLP.trafficClass = 0x00;*/
	memReadTLP.Dw0MemReadReqTLP.dw0UnusedField = 0;
	memReadTLP.Dw0MemReadReqTLP.type = 0x00;

	/*memReadTLP.Dw1MemoReadReqTLP.firstByteEnable = 0x0f;
	memReadTLP.Dw1MemoReadReqTLP.lastByteEnable = 0x00;
	memReadTLP.Dw1MemoReadReqTLP.requesterID = 0x0000;*/
	memReadTLP.Dw1MemReadReqTLP.byteEnable = 0x0f;
	memReadTLP.Dw1MemReadReqTLP.requesterID = 0;

	/*while (mTLPQueueMgr.isFull(MEM_READ_CMD_Q))
	{
	wait(mMemReadCmdQNotFullEvent);
	}*/
	if (tag == SQ1_CMDTAG)
	{
		memReadTLP.Dw1MemReadReqTLP.tag = ((tag & 0x3) << 6) | (mTagCounter & 0x3f);
		mTLPQueueMgr.pushTLP(MEM_READ_CMD_Q, reinterpret_cast<uint8_t*> (&memReadTLP));
		mTagCounter++;
		if (mTagCounter >= 128)
			mTagCounter = 0;
	}
	else if (tag == BUFFPTR_TAG)
	{
		uint8_t qAddr;
		//mWriteDataBuff.getFreeBuffPtr(qAddr);
		qAddr = mWriteBuffAddr.at(chanNum).front();
		memReadTLP.Dw1MemReadReqTLP.tag = ((tag & 0x3) << 6) | (qAddr & 0x3f);
		mTLPQueueMgr.pushTLP(MEM_READ_DATA_Q, reinterpret_cast<uint8_t*> (&memReadTLP));
	}

}

void TeraSPCIeController::createMemWriteTLP(uint64_t& address, hostQueueType qType, uint32_t& qAddr, uint8_t* data)
{
	//uint8_t* data = mMemMgr.acquire(mCwSize);
	tMemoryWriteReqTLP mMemWriteReqTLP = {0};
	mMemWriteReqTLP.address = (uint32_t)address;
	mMemWriteReqTLP.Dw0MemWriteReqTLP.format = 0x02;
	mMemWriteReqTLP.Dw0MemWriteReqTLP.dw0UnusedField = 0x0;
	mMemWriteReqTLP.Dw1MemWriteReqTLP.byteEnable = 0x0f;
	mMemWriteReqTLP.Dw1MemWriteReqTLP.requesterID = 0x0;
	mMemWriteReqTLP.data = data;

	if (qType == hostQueueType::DATA)
	{
		mMemWriteReqTLP.Dw0MemWriteReqTLP.length = mCwSize / 4;
		//mReadDataBuff.readData((uint8_t&)qAddr, data);
		mMemWriteReqTLP.Dw1MemWriteReqTLP.tag = ((tagType::BUFFPTR_TAG) & 0x3 << 6) | qAddr & 0x3f;

		mTLPQueueMgr.pushTLP(MEM_WRITE_DATA_Q, reinterpret_cast<uint8_t*> (&mMemWriteReqTLP));
	}
	else if (qType == hostQueueType::COMPLETION)
	{

		mMemWriteReqTLP.Dw0MemWriteReqTLP.length = 4;
		mTLPQueueMgr.pushTLP(MEM_WRITE_COMP_Q, reinterpret_cast<uint8_t*> (&mMemWriteReqTLP));
	}

}

#pragma endregion TLP_CREATION_METHODS

#pragma region ARBITRATION

TLPQueueType TeraSPCIeController::queueArbiter()
{
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
	else if(mArbiterToken == 2)
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

aQType TeraSPCIeController::queueArbitration(const uint8_t& chanNum)
{
	int64_t shortCmd = mAscq.at(chanNum).peekQueue();
	int64_t dmaCmd = mAdcq.at(chanNum).peekQueue();

	if (mAvailableCredit.at(chanNum) != 0)
	{
		if (shortCmd != -1)
		{
			mAvailableCredit.at(chanNum)--;
			return SHORT_QUEUE;
		}
		else if (dmaCmd != -1)
		{
			return DMA_QUEUE;
		}

	}
	else
	{
		if (dmaCmd != -1)
		{
			return DMA_QUEUE;
		}

		else {

			wait(*mAdcqNotEmptyEvent.at(chanNum));
			return DMA_QUEUE;
		}

	}

}
#pragma endregion ARBITRATION

/** end of simulation
* @return void
**/
void TeraSPCIeController::end_of_simulation()
{
	for (uint8_t chanIndex = 0; chanIndex < mChanNum; chanIndex++)
	{
		mOnfiChanActivityReport->writeFile(chanIndex, mOnfiStartActivity.at(chanIndex), mOnfiEndActivity.at(chanIndex), " ");
	}
}
}