#include "TeraSPCIeController.h"
#include "TeraSMemoryChip.h"
//#include "pciCntrlTb.h"
#include "Router.h"
#include "PciTestBench.h"
#include "SystemMemory.h"
#include "pcieRC.h"
#include "pciTbCommon.h"
#include "Config.h"

#include <stdio.h>

class top{
public:
	std::vector<sc_signal<bool>* > mChipSelect;

	CrossbarTeraSLib::TeraSPCIeController *pcie = new CrossbarTeraSLib::TeraSPCIeController(sc_gen_unique_name("PCIE"), 32, 4, 4096, BA_MEM + BA_CQ0, BA_MEM + BA_SQ0, SZ_CQ0, SZ_SQ0, 512, 16,
		256, sc_time(1000, SC_NS), sc_time(2000, SC_NS), \
		5, 800, 1600, \
		8, 2, 1024, 1, 0, 0, 100, 4096,985);

	CrossbarTeraSLib::TeraSMemoryDevice *rram = new CrossbarTeraSLib::TeraSMemoryDevice(sc_gen_unique_name("TeraSRRAM"), 512, 2, 8, 1024, 256, sc_time(1000, SC_NS), sc_time(2000, SC_NS), 800, 16, 4096, 1, 0, 4096);

	for (uint8_t chanIndex = 0; chanIndex < 16; chanIndex++)
	{
		mChipSelect.push_back(new sc_signal<bool>(sc_gen_unique_name("chipSelect")));
		rram->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
		pcie->pChipSelect.at(chanIndex)->bind(*mChipSelect.at(chanIndex));
		pcie->pOnfiBus.at(chanIndex)->bind(*(rram->pOnfiBus.at(chanIndex)));
	}
	PciTestBench* tb = new PciTestBench("tb", 1);
	Router<2>* router = new Router<2>("router");
	SystemMemory*    memory = new SystemMemory(sc_gen_unique_name("SystemMemory"), sc_core::sc_time(1000, SC_NS), sc_core::sc_time(2000, SC_NS));
	pcieRC*	iPcieRC = new pcieRC("iPcieRC",985);

	// Bind sockets
	tb->tbSocket.bind(router->tbSocket);
	router->routerSocket[0].bind(memory->t_socket);
	router->routerSocket[1].bind(iPcieRC->hostIntfRx);
	iPcieRC->hostIntfTx.bind(router->pciRCSocket);
	iPcieRC->cntrlIntfTx.bind(pcie->pcieRxInterface);
	pcie->pcieTxInterface.bind(iPcieRC->cntrlIntfRx);
}