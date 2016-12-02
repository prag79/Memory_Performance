/*
 * tg.cpp
 *
 *  Created on: Mar 31, 2016
 *      Author: ruchier.shah
 *  Crossbar Inc. Confidential
 */

#include <iostream>
#include <chrono>
#include "trafficGenerator.h"

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
	
	if (ioMin > ioMax) {
		std::cout << "IO Range not supported. Setting both to " << ioMax <<
				std::endl;
		ioMin = ioMax;
	}
	if (seqPct !=0 && seqPct != 100) {
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
	rndLba.seed((unsigned long)seed);
	rndDir.seed((unsigned long)seed);
	nextLba = 0;
	readCnt = 0;
	cmdCnt = 0;
}

/**
 * Main interface function to get list of commands from traffic generator
 *
 * It is assumed that cmdPayload is big enough to hold all the commands
 */
int trafficGenerator::writeCommands(int numCmds)
{
	int i;
	cmdField cmdPayload;
	for (i = 0; i < numCmds; ++i) {
		uint8_t cmdDir = (uint8_t)distDir(rndDir);
		if (cmdDir < rdPct) {
			cmdPayload.d = ioDir::read;
		} else {
			cmdPayload.d = ioDir::write;
		}
		cmdPayload.cwCnt = ioMax/cwSize;
		if (seqPct == 100) {
			cmdPayload.lba = nextLba;
			nextLba += ioMax / cwSize;
			if (nextLba >= maxLba) {
				nextLba = 0;
			}
		} else {
			uint64_t lba = distLba(rndLba);
			if (align == alignment::io) {
				lba -= lba % ioMax;
			}
			cmdPayload.lba = lba;
		}
		mCmdWrFile << (bool)cmdPayload.d << " " << cmdPayload.lba << " " << cmdPayload.cwCnt << std::endl;
	}
	return 0;
}

/**
* File Open API
*
*/
bool trafficGenerator::openWriteModeFile()
{
	mCmdWrFile.open("CmdFile.txt",  std::fstream::out | std::fstream::trunc);
	if (mCmdWrFile.is_open())
	{
		return true;
	}
	else {
		//TODO: Report Error
		return false;
	}
}

bool trafficGenerator::openReadModeFile()
{
	if (!mCmdRdFile.is_open())
	{
		mCmdRdFile.open("CmdFile.txt", std::fstream::in);
	}
	if (mCmdRdFile.is_open())
	{
		return true;
	}
	else {
		//TODO: Report Error
		return false;
	}
}
/**
* File Close API
*
*/
bool trafficGenerator::closeWriteModeFile()
{
	if (mCmdWrFile.is_open())
	{
		mCmdWrFile.close();
		return true;
	}
	else {
		//TODO: Report Error
		return false;
	}
}

bool trafficGenerator::closeReadModeFile()
{
	if (mCmdRdFile.is_open())
	{
		mCmdRdFile.close();
		return true;
	}
	else {
		//TODO: Report Error
		return false;
	}
}

/**
* Read Commands from File
*
*/
void trafficGenerator::getCommands(cmdField& payload)
{
	bool ioDir;
	mCmdRdFile >> ioDir >>  payload.lba >>  payload.cwCnt;
	if (ioDir)
	{
		payload.d = ioDir::write;
	}
	else
	{
		payload.d = ioDir::read;
	}
}

int trafficGenerator::getCommands(cmdField *cmdPayload, int numCmds)
{
	int i;
		for (i = 0; i < numCmds; ++i) {
		uint8_t cmdDir = (uint8_t)distDir(rndDir);
		if (cmdDir < rdPct) {
			cmdPayload[i].d = ioDir::read;
		}
		else {
			cmdPayload[i].d = ioDir::write;
		}
		cmdPayload[i].cwCnt = ioMax / cwSize;
		if (seqPct == 100) {
			cmdPayload[i].lba = nextLba;
			nextLba += ioMax / cwSize;
			if (nextLba >= maxLba) {
				nextLba = 0;
			}
		}
		else {
			uint64_t lba = distLba(rndLba);
			if (align == alignment::io) {
				lba -= lba % ioMax;
			}
			cmdPayload[i].lba = lba;
		}
	}
	return 0;
}

void trafficGenerator::getCommands(cmdField *cmdPayload, int& numCmds, int cpuNum)
{
	//setFilePtr(numCmds * cpuNum);
	bool ioDir;
	for (int i = 0; i < numCmds; i++)
	{
		mCmdRdFile >> ioDir >> cmdPayload[i].lba >> cmdPayload[i].cwCnt;
		if (ioDir)
		{
			cmdPayload[i].d = ioDir::write;
		}
		else
		{
			cmdPayload[i].d = ioDir::read;
		}
	}
}
/**
* Set the file pointer to the 
* beginning of the file
*/
void trafficGenerator::setFilePtrToBeg()
{
	mCmdWrFile.seekp(0, mCmdWrFile.beg);
}
void trafficGenerator::setFilePtr(uint64_t fileIndex)
{
	mCmdRdFile.seekg(fileIndex * 5, mCmdRdFile.beg);
}