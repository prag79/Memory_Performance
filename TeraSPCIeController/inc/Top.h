#include "TeraSPCIeController.h"
#include "TeraSMemoryChip.h"
//#include "pciCntrlTb.h"
#include "Router.h"
#include "PciTestBench.h"
#include "SystemMemory.h"
#include "pcieRC.h"
#include "pciTbCommon.h"
#include "Config.h"
#include <fstream>

class Top{
public:
   Top(uint32_t NumCmd, uint32_t QueueSize, uint32_t QueueFactor, uint32_t BlockSize, uint32_t CwSize, uint8_t ChanNum\
			, uint32_t PageSize,uint32_t ReadTime, uint32_t ProgramTime, uint8_t Credit, uint32_t OnfiSpeed, uint32_t BankNum\
			, uint8_t NumDie, uint32_t PageNum,  bool EnMultiSim, bool Mode, double CmdTransferTime\
			, uint32_t IoSize, uint32_t PcieSpeed, uint32_t SizeOfSQ, uint32_t SizeOfCQ, uint8_t CmdType, uint32_t PollTime, \
			uint32_t TbQueueDepth, bool EnableWorkLoad, std::string WrkloadFiles, uint16_t SeqLBAPct, uint16_t CmdPct, \
			uint32_t wrBuffSize, uint32_t rdBuffSize)
			:mIoSize(IoSize)
			, mBlockSize(BlockSize)
			, mQueueDepth(TbQueueDepth)
   {
			mChannelNum = ChanNum;
		   mLogFileHandler.open(appendParameters("./Logs_PCIe/TeraSPCIeController",".log"), std::fstream::trunc | std::fstream::out);

		   /*pcie = new CrossbarTeraSLib::TeraSPCIeController(sc_gen_unique_name("PCIE"), 32, 4, 512, BA_MEM + BA_CQ0, BA_MEM + BA_SQ0, SZ_CQ0, SZ_SQ0, 512, 16,
			   256, sc_time(1000, SC_NS), sc_time(2000, SC_NS), 5, 800, 1600, 8, 2, 1024, 1, 0, 0, 100, 512, 985);
			   rram = new CrossbarTeraSLib::TeraSMemoryDevice(sc_gen_unique_name("TeraSRRAM"), 512, 2, 8, 1024, 256, sc_time(1000, SC_NS), sc_time(2000, SC_NS), 800, 16, 512, 1, 0, 512);
			   tb = new CrossbarTeraSLib::PciTestBench("tb", 2, 1, 512, 512);
			   router = new Router<2>("router");
			   memory = new SystemMemory(sc_gen_unique_name("SystemMemory"), sc_core::sc_time(0, SC_NS), sc_core::sc_time(0, SC_NS));
			   iPcieRC = new pcieRC("iPcieRC", 985);*/

	   pcie = new CrossbarTeraSLib::TeraSPCIeController(sc_gen_unique_name("PCIE"), QueueSize, QueueFactor, BlockSize, BA_MEM + BA_CQ0, BA_MEM + BA_SQ0, \
								SizeOfCQ, SizeOfSQ, CwSize, ChanNum, PageSize, sc_time(ReadTime, SC_NS), sc_time(ProgramTime, SC_NS), Credit, OnfiSpeed, \
								BankNum, NumDie, PageNum, EnMultiSim, Mode, CmdTransferTime, IoSize, PcieSpeed, mLogFileHandler, TbQueueDepth, wrBuffSize, rdBuffSize);
			   rram		= new CrossbarTeraSLib::TeraSMemoryDevice(sc_gen_unique_name("TeraSRRAM"), CwSize, NumDie, BankNum, PageNum, PageSize, \
				   sc_time(ReadTime, SC_NS), sc_time(ProgramTime, SC_NS), OnfiSpeed, ChanNum, BlockSize, TbQueueDepth, EnMultiSim, IoSize, true);
			   tb = new CrossbarTeraSLib::PciTestBench("tb", NumCmd, BlockSize, CwSize, SizeOfSQ, SizeOfCQ, CmdType, PollTime, EnMultiSim, IoSize,\
				   TbQueueDepth, EnableWorkLoad, WrkloadFiles,SeqLBAPct,CmdPct);
			   router = new Router<2>("router");
			   memory	= new SystemMemory(sc_gen_unique_name("SystemMemory"), sc_time(0, SC_NS),sc_time(0, SC_NS));//system memory read/write time is 0 ns
			   iPcieRC = new pcieRC("iPcieRC", PcieSpeed, QueueSize, CwSize, mLogFileHandler, SizeOfCQ, SizeOfSQ, EnMultiSim, BlockSize, IoSize, TbQueueDepth);

		for (uint8_t chanIndex = 0; chanIndex < ChanNum; chanIndex++)
		{
			mChipSelect.push_back(new sc_signal<bool>(sc_gen_unique_name("chipSelect")));
			rram->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
			pcie->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
			pcie->pOnfiBus.at(chanIndex)->bind(*(rram->pOnfiBus.at(chanIndex)));
		}

		// Bind sockets
		tb->tbSocket.bind(router->tbSocket);
		router->routerSocket[0].bind(memory->t_socket);
		router->routerSocket[1].bind(iPcieRC->hostIntfRx);
		iPcieRC->hostIntfTx.bind(router->pciRCSocket);
		iPcieRC->cntrlIntfTx.bind(pcie->pcieRxInterface);
		pcie->pcieTxInterface.bind(iPcieRC->cntrlIntfRx);

		
		
	}
	void erase()
	{
		delete pcie;
		delete rram;
		delete router;
		delete memory;
		delete iPcieRC;
		delete tb;
		for (uint8_t chanIndex = 0; chanIndex < mChannelNum; chanIndex++)
		{
		delete mChipSelect.at(chanIndex);
		}
		mLogFileHandler.close();
	}
	
	string appendParameters(string name, string format)
	{

		char temp[64];
		name.append("_iosize");
		_itoa(mIoSize, temp, 10);
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
private:
	std::vector<sc_signal<bool>* >			mChipSelect;
	
	CrossbarTeraSLib::TeraSPCIeController	*pcie;
	CrossbarTeraSLib::TeraSMemoryDevice		*rram;
	
	CrossbarTeraSLib::PciTestBench*	tb;
	Router<2>*		router;
	SystemMemory*   memory;
	pcieRC*			iPcieRC;
	
	std::fstream		mLogFileHandler;
	uint32_t			mChannelNum;
	uint32_t mIoSize;
	uint32_t mBlockSize;
	uint32_t mQueueDepth;

};
