//Common

#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

#include <cstdint>
#define DDR_BUS_WIDTH 64
#define DDR_XFER_RATE 1600 
#define COMMAND_PAYLOAD_SIZE 8
#define COMMAND_BL 4
#define MAX_VALID_COMPLETION_ENTRIES 21
#define MAX_COMPLETION_BYTES_READ 64
#define MAX_BLOCK_SIZE 8192
#define MAX_SLOT 256
#define SIZE_CMD_PAYLOAD 8
#define BUS_WIDTH 4

enum burstLength{
	BL4 = 0,
	BL8 = 1
};
enum status{
	BUSY = 0,
	FREE = 1
};
enum Flag{
	SET = 1,
	RESET = 0
};
enum chipSelect{
	PCMDQUEUE = 0x0,
	COMPLETIONQUEUE = 0x1,
	DATABUFFER0 = 0x2,
	DATABUFFER1 = 0x3
};
struct SlotStatusTable{
	bool status;
	uint64_t payload;

	SlotStatusTable()
	{
		reset();
	}
	void reset()
	{
		status = status::FREE;
		payload = 0;
	}
};
typedef SlotStatusTable mSlotStatusTable;

/** append parameter in multi simulation
* @param name name to be modified
* @param mBlockSize block size
* @param mQueueDepth queue depth of the host 
* @return void
**/
string appendParameters(string name,uint32_t mBlockSize,uint32_t mQueueDepth)
{

	char temp[32];
	name.append("_iosize");
	_itoa(mBlockSize, temp, 10);
	name.append("_");
	name.append(temp);
	name.append("_qd");
	_itoa(mQueueDepth, temp, 10);
	name.append("_");
	name.append(temp);
	name.append(".log");
	//string temp = name;
	return name;
}
#endif