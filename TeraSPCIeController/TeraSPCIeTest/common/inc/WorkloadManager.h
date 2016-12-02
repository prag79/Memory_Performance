/*******************************************************************
 * File : WorkloadManager.h
 *
 * Copyright(C) crossbar-inc. 2016 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains detail of Workload Management used in the
 * Test Bench.
 * This file reads workload files and generate command payload.
 * Related Specifications: 
 * TeraS_Controller_Test Plan_ver0.2.doc
 ********************************************************************/
#ifndef __WORKLOAD_MANAGER_H__
#define __WORKLOAD_MANAGER_H__
#include<iostream>
#include<fstream>
#include <string>
#include<cstdint>

class WorkLoadManager
{
public:
	WorkLoadManager(std::string fileName)
		:mFile(fileName)
	{

	}
	bool  openReadModeFile();
	void readFile(std::string& cmdType, uint64_t& lba, uint32_t& cwCnt);
	bool closeFile();
	bool isEOF();
	void setBeginOfFile();
	void readLine(std::string& text/*char* s, streamsize n*/);
	std::streamoff tellPos();
private:
	std::string mFile;
	std::ifstream mFileHandler;
};

#endif