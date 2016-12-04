// MemoryQueueProject.cpp : Defines the entry point for the console application.
//

#include "systemc.h"
#include<iostream>
#include<cstdint>
#include "memoryQueue.h"

typedef struct
{
	uint32_t data;
	uint8_t cmd;
	bool parity;
} transaction;

void createPayload(transaction *pd)
{
	pd->data = (uint32_t)rand() % 100;
	pd->cmd = (uint8_t)rand() % 10;
	pd->parity = true;

}

bool mypredicate(transaction* i, transaction* j) {

	if ((i->cmd == j->cmd) && (i->data == j->data) && (i->parity == j->parity))
		return true;
	else
		return false;
}

int sc_main(int argc, char* argv[])
{
	MemoryQueue<transaction> txCommandQueue(10);
	std::vector<transaction *>payload(10);
	//std::vector<transaction *>::iterator it1, it2;

	for (int i = 0; i < 10; i++) {
		payload[i] = new transaction;
		createPayload(payload[i]);
	}
	for (int i = 0; i < 10; i++)
		txCommandQueue.pushToQueue(payload[i]);

	std::vector<transaction *>gotData(10);
	for (int i = 0; i < 10; i++)
	{
		gotData[i] = txCommandQueue.popQueue();
		std::cout << "Data in the queue " << i << " is" << std::hex << (uint32_t)gotData[i]->data << " " << std::endl;
		std::cout << "command in the queue " << i << " is" << std::hex << (uint32_t)gotData[i]->cmd << " " << std::endl;
		std::cout << "command in the queue " << i << " is" << std::hex << (bool)gotData[i]->parity << " " << std::endl;
		std::cout << std::endl << std::endl;

	}
	if (std::equal(payload.begin(), payload.end(), gotData.begin(), mypredicate))
	{
		std::cout << "Match" << std::endl;
	}
	else
		std::cout << "ERROR::MisMatch" << std::endl;

	payload.clear();
	gotData.clear();

	return 0;
}

