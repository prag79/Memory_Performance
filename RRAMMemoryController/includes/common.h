#ifndef COMMON_H
#define COMMON_H
#include<cstdint>
#include "systemc.h"
namespace CrossbarTeraSLib {
//#ifdef PCI_LANE_16
//#define PCI_BUS_WIDTH 128
//#else
//#define PCI_BUS_WIDTH 32
//#endif
//#define MEM_RD_CMD_WIDTH 12
//#define MEM_RD_DATA_WIDTH 12
//#define MEM_WR_DATA_WIDTH 12
//#define MEM_WR_COMPLETION_WIDTH 16
//#define SQ_DATA_WIDTH 64
//#define HCMD_DATA_WIDTH 20
	//#define SET_CAP_MPSMAX(reg, value) ((reg & 0xFF0FFFFFFFFFFFFFull)| ((value << 52) & 0x00F0000000000000ull))  
#define GET_TLP_TYPE(reg)        ((reg >> 24) & 0x1F)
#define GET_TLP_FORMAT(reg)        ((reg >> 29) & 0x3)
#define GET_TLP_LENGTH(reg) ((reg) & 0x3FF)

	enum cmdType {
		READ = 0,
		WRITE = 1,
		NO_CMD = 2
	};

	enum tagType {
		SQ0_CMDTAG = 0x0,
		SQ1_CMDTAG = 0x1,
		BUFFPTR_TAG = 0x2,
	};

	struct HCMDQueueEntry
	{
		uint8_t hCmd;
		uint64_t lba;
		uint16_t cnt;
		uint16_t cid : 16;
		uint64_t hAddr0 : 64;
		uint64_t hAddr1 : 64;
		sc_core::sc_time time;
		HCMDQueueEntry()
		{
			reset();
		}
		inline void reset()
		{
			hCmd = 0;
			lba = 0;
			cnt = 0;
			cid = 0;
			hAddr0 = 0;
			hAddr1 = 0;
			time = sc_core::SC_ZERO_TIME;
		}
	};
	typedef struct HCMDQueueEntry HCmdQueueData;
	struct PCMDQueueEntry
	{
		cmdType cmd : 2;
		uint64_t plba : 64;
		uint32_t cmdOffset : 32;
		uint32_t iTag : 32;
		int32_t nextAddr : 32;
		sc_core::sc_time time;
		PCMDQueueEntry()
		{
			reset();
		}
		inline void reset()
		{
			cmd = READ;
			plba = 0;
			cmdOffset = 0;
			iTag = 0;
			nextAddr = -1;
			time = sc_core::SC_ZERO_TIME;
		}
	};
	typedef struct PCMDQueueEntry CmdQueueData;


	enum status
	{
		FREE,
		BUSY
	};

	struct CmdParamTblEntry
	{
		CmdParamTblEntry()
		{
			reset();
		}
		void reset()
		{
			cid = 0;
			cmd = READ;
			lba = 0;
			blockCnt = 0;
			//	nextAddr = 0;
		}
		uint16_t cid : 16;
		cmdType cmd : 2;
		uint64_t lba;
		uint16_t blockCnt : 9;
		uint64_t hAddr0 : 64;
		uint64_t hAddr1 : 64;

	};

	typedef struct CmdParamTblEntry cmdTableField;

	struct DataBufferStatus
	{
		DataBufferStatus()
		{
			buffStatus = status::FREE;
			nextAddr = 0;
		}
		status buffStatus;
		uint32_t nextAddr;

	};

	typedef struct DataBufferStatus BufferStatus;
	enum TLPQueueType
	{
		MEM_READ_CMD_Q,
		MEM_READ_DATA_Q,
		MEM_WRITE_DATA_Q,
		MEM_WRITE_COMP_Q,
		NONE
	};

	enum hostQueueType
	{
		SUBMISSION,
		COMPLETION,
		DATA,
		//NONE
	};

	enum tlpType
	{
		MEM_READ_REQ,
		MEM_WRITE_REQ,
		COMPLETION_TLP,
		CFG_READ,
		CFG_WRITE
	};
	struct ActiveCmdQueueEntry
	{
		cmdType cmd;
		uint64_t plba;
		uint32_t cmdOffset;
		uint32_t iTag;
		uint32_t queueNum;
		sc_core::sc_time time;

		ActiveCmdQueueEntry()
		{
			reset();
		}
		inline void reset()
		{
			cmd = READ;
			plba = 0;
			cmdOffset = 0;
			iTag = 0;
			queueNum = 0;
			time = sc_core::SC_ZERO_TIME;
		}
	};
	typedef struct ActiveCmdQueueEntry ActiveCmdQueueData;

	struct ActiveDMACmdQueueEntry
	{
		cmdType cmd;
		uint64_t plba;
		uint32_t cmdOffset;
		uint32_t iTag;
		uint32_t queueNum;
		uint8_t buffPtr;
		sc_core::sc_time time;

		ActiveDMACmdQueueEntry()
		{
			reset();
		}
		inline void reset()
		{
			cmd = READ;
			plba = 0;
			cmdOffset = 0;
			iTag = 0;
			queueNum = 0;
			buffPtr = 0;
			time = sc_core::SC_ZERO_TIME;
		}
	};
	typedef struct ActiveDMACmdQueueEntry ActiveDMACmdQueueData;

	struct CmdLatency
	{
		double startDelay;
		double endDelay;
		double latency;
		CmdLatency()
		{
			startDelay = 0;
			endDelay = 0;
			latency = 0;
		}
	};
	typedef struct CmdLatency CmdLatencyData;

	enum CONTROLLER_STATES
	{
		MEMORY_READ_REQUEST_CMD,
		MEMORY_READ_REQUEST_DATA,
		MEMORY_WRITE_REQUEST_CMD,
		MEMORY_WRITE_REQUEST_COMPLETION,
		ADMIN_COMPLETION_READ_REQ,
		COMPLETION_READ_REQ,
		COMPLETION_REQ_W_DATA
	};

	enum bankStatus
	{
		BANK_FREE = 0,
		BANK_BUSY = 1
	};
	typedef bankStatus cwBankStatus;

	enum activeQueueType
	{
		SHORT = 0,
		LONG = 1,
		DMA = 2,
	};
	typedef enum activeQueueType aQType;

#define TAG_MASK 0xC0
#define PAGE_BITS 4
#define CW_BANK_MASK 0xF
#define PAGE_MASK_BITS 0x1FFFFFFFFF
#define PAGE_BOUNDARY_MASK 0xFF
#define NUM_PAGE_BITS 28
#define NUM_BANK_BITS 3
#define BANK_MASK_BITS 0x7
#define minima(a,b) ((a)<(b)? (a):(b))
#define MAX_ADDRESS_RANGE 0xFFFFFFFFFF
#define ONFI_BUS_WIDTH 8
#define ONFI_SPEED 800
#define ECZ_FACTOR 8
#define MAX_NUM_OF_BANKS 8
#define MAX_NUM_OF_PAGES 4
#define NEXT_ADDR_MASK 0xff00000000000000
#define LBA_MASK  0x000003fffffffe00 
#define DDR_BUS_WIDTH 64
#define ONFI_WRITE_COMMAND 0x80
#define ONFI_READ_COMMAND 0x00
#define ONFI_BANK_SELECT_COMMAND 0x05
#define ONFI_COMMAND_LENGTH 7
#define ONFI_BANK_SELECT_CMD_LENGTH 4
#define DATA_BUFFER_WIDTH 64
#define DATA_BUFFER_BIT_SHIFT 6
#define COMPLETION_WORD_SIZE 2
}
#endif