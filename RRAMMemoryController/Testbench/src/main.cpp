/*
 * main.cpp
 *
 *  Created on: Mar 31, 2016
 *      Author: ruchier.shah
 *  Crossbar Inc. Confidential
 * @file main.cpp
 * 	Main file showing usage example of traffic generator class
 */

#include <iostream>
#include "tg.h"

int main(void)
{
	trafficGenerator tg(1024 * 1024 * 4, 512, 4096, 4096, 90, 100, alignment::io);
	cmd payload[40];
	tg.getCommands(payload, 20);
	for (int i = 0; i < 20; ++i) {
		std::cout << (payload[i].d == ioDir::read ? "READ": "WRITE") << ", " <<
				payload[i].cwCnt << ", " <<
				payload[i].lba << std::endl;
	}
	return 0;
}
