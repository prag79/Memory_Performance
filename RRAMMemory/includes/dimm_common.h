#ifndef _DIMM_COMMON_H_
#define _DIMM_COMMON_H_
#include<cstdint>
#include<string>
#include "tlm.h"

enum implementation_type {
	ARRAY = 0,
	FILEIO = 1
};

typedef implementation_type type;

enum cmdType {
	READ = 0,
	WRITE = 1
};
////string appendParameters(string name, uint32_t mBlockSize, uint32_t mQueueDepth);
//struct CmdExtension : public tlm::tlm_extension<CmdExtension>
//{ // user defined extension class
//	CmdExtension()
//	: queueNum(0) {}
//
//	virtual tlm_extension_base* clone() const { // Must override pure virtual clone method
//		CmdExtension* t = new CmdExtension();
//		t->queueNum = this->queueNum;
//		return t;
//	}
//	// Must override pure virtual copy_from method
//	virtual void copy_from(tlm_extension_base const &ext) {
//		queueNum = static_cast<CmdExtension const &>(ext).queueNum;
//	}
//		uint32_t queueNum;
//};

struct PCMDQueueEntry
{
	uint64_t cmd;
	int32_t nextAddr : 32;
	sc_core::sc_time time;
	PCMDQueueEntry()
	{
		reset();
	}
	inline void reset()
	{
		cmd = 0;
		nextAddr = -1;
		time = sc_core::SC_ZERO_TIME;
	}
};
typedef struct PCMDQueueEntry CmdQueueData;

struct ActiveCmdQueueEntry
{
	uint64_t cmd;
	sc_core::sc_time time;
	
	ActiveCmdQueueEntry()
	{
		reset();
	}
	inline void reset()
	{
		cmd = 0;
		time = sc_core::SC_ZERO_TIME;
	}
};

typedef struct ActiveCmdQueueEntry ActiveCmdQueueData;

struct CmdLatency
{
	double startDelay;
	double endDelay;
	double latency;
	CmdLatency()
	{
		startDelay = 0;
		endDelay   = 0;
		latency    = 0;
	}
};
typedef struct CmdLatency CmdLatencyData;

enum bankStatus
{
	BANK_FREE = 0,
	BANK_BUSY = 1
};
typedef bankStatus cwBankStatus;

enum activeQueueType
{
	SHORT_QUEUE = 0,
	LONG_QUEUE = 1,
	NONE= 2
};

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

struct PhysicalDLStatus
{
	PhysicalDLStatus()
	{
		mStatus = status::FREE;
	}

	status getStatus()
	{
		return mStatus;
	}

	void setStatus(status& val)
	{
		mStatus = val;
	}

private:
	status mStatus;
};
#endif