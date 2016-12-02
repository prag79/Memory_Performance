/*
 * tg.cpp
 *
 *  Created on: Mar 31, 2016
 *      Author: ruchier.shah
 *  Crossbar Inc. Confidential
 */

#include <iostream>
#include <chrono>
#include "tg.h"

/**
 * Constructor
 * Initializes all the member variables and setups up random number generator
 */
trafficGenerator::trafficGenerator(uint64_t m, uint32_t c,
		uint32_t imn, uint32_t imx, uint8_t r, uint8_t s,
		alignment a, uint64_t sd) : distLba(0, m + 1), distDir(0, 99),
				maxLba(m), cwSize(c), ioMin(imn), ioMax(imx), rdPct(r),
				seqPct(s), align(a), seed(sd)
{
	if (ioMin != ioMax) {
		std::cout << "IO Range not supported. Setting both to " << ioMax <<
				std::endl;
		ioMin = ioMax;
	}
	if (seqPct != 0 && seqPct != 100) {
		std::cout << "Seq/Rnd Percentage Not Supported. Setting to 100% Seq" <<
				std::endl;
		seqPct = 100;
	}
	if (seed == 0) {
		using namespace std::chrono;
		milliseconds ms = duration_cast< milliseconds >(
		    system_clock::now().time_since_epoch());
		seed = ms.count();
	}
	rndLba.seed(seed);
	rndDir.seed(seed);
	nextLba = 0;
	readCnt = 0;
	cmdCnt = 0;
}

/**
 * Main interface function to get list of commands from traffic generator
 *
 * It is assumed that cmdPayload is big enough to hold all the commands
 */
int trafficGenerator::getCommands(cmd *cmdPayload, int numCmds)
{
	int i;
	for (i = 0; i < numCmds; ++i) {
		uint8_t cmdDir = distDir(rndDir);
		if (cmdDir < rdPct) {
			cmdPayload[i].d = ioDir::read;
		} else {
			cmdPayload[i].d = ioDir::write;
		}
		cmdPayload[i].cwCnt = ioMax/cwSize;
		if (seqPct == 100) {
			cmdPayload[i].lba = nextLba;
			nextLba += ioMax / LBA_SIZE;
			if (nextLba >= maxLba) {
				nextLba = 0;
			}
		} else {
			uint64_t lba = distLba(rndLba);
			if (align == alignment::io) {
				lba -= lba % ioMax;
			}
			cmdPayload[i].lba = lba;
		}
	}
	return 0;
}
