/*
 * tg.h
 *
 *  Created on: Mar 31, 2016
 *      Author: ruchier.shah
 *  Crossbar Inc. Confidential
 * @file tg.h
 * 	Header file for traffic generator class
 * 	Currently not supported:
 * 		- IO Range: Only one IO size supported. ioMin = ioMax
 * 		- Variable seq/rand percentage: only 0% sequential or 100% sequential
 */

#ifndef TG_H_
#define TG_H_
#include<cstdint>
#include <random>
#include <fstream>

#define LBA_SIZE 512

enum class ioDir {read, write};

enum class alignment {none, io};

/**
 * Command structure to describe single IO command
 */
struct cmdField {
	ioDir d;					// Read or Write
	uint64_t lba;				// LBA of the command
	uint16_t cwCnt;// Number of codewords
	cmdField()
	{
		reset();
	}
	void reset()
	{
		d = ioDir::read;
		lba = 0;
		cwCnt = 0;
	}
};

class trafficGenerator {
private:
	std::mt19937_64 rndLba;		// Random number generator and distribution
	std::uniform_int_distribution<uint64_t> distLba;
	std::mt19937_64 rndDir;		// Random number generator and distribution
	std::uniform_int_distribution<uint64_t> distDir;
	uint64_t maxLba;
	uint32_t cwSize;
	uint32_t ioMin;
	uint32_t ioMax;
	uint8_t rdPct;
	uint8_t seqPct;
	alignment align;
	uint64_t seed;
	uint64_t nextLba;			// LBA cursor for sequential traffic
	uint8_t readCnt;
	uint8_t cmdCnt;
	uint8_t mNumDie;
	uint8_t mNumBanks;
	std::ofstream mCmdWrFile;
	std::ifstream mCmdRdFile;
public:
	trafficGenerator(
		uint64_t maxLba,	// Max LBA. LBA Size is 512bytes
		uint32_t cwSize,	// Codeword size in bytes
		uint32_t ioMin,		// Minimum IO size in bytes
		uint32_t ioMax,		// Maximum IO size in bytes
		uint8_t rdPct,		// % of generated command as Read commands
		uint8_t seqPct,		// % of generated command as sequential commands
		alignment align = alignment::none,	// IO alignment. IO or None
		uint64_t seed = 0,	// Seed for random number generator for LBA
		uint8_t numDie,
		uint8_t numBanks
			);

	int writeCommands(int numCmds);
	int writeCommandsToSameBanks(int numCmds);
	void getCommands(cmdField& payload);
	int getCommands(cmdField *cmdPayload, int numCmds);
	void getCommands(cmdField* cmdPayload, int& numCmds, int cpuNum);
	
	void setFilePtrToBeg();
	void setFilePtr(uint64_t fileIndex);
	bool closeWriteModeFile();
	bool closeReadModeFile();
	bool openWriteModeFile();
	bool openReadModeFile();
	void readCommand(cmdField& cmdPayload);
	uint64_t getSeed(void)
	{
		return seed;
	}
};

#endif /* TG_H_ */
