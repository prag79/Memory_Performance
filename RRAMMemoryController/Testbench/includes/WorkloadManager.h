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
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

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
bool WorkLoadManager::openReadModeFile()
{
	if (mFileHandler.is_open())
	{
		return true;
	}
	else {
		//TODO: Report Error
		mFileHandler.open(mFile, std::fstream::in);
		return false;
	}
}

void WorkLoadManager::readFile(std::string& cmdType, uint64_t& lba, uint32_t& cwCnt)
{
	mFileHandler >> cmdType >> lba >> cwCnt;
}

bool WorkLoadManager::closeFile()
{
	if (mFileHandler.is_open())
	{
		mFileHandler.close();
		return true;
	}
	else {
		//TODO: Report Error
		return false;
	}
}
	bool WorkLoadManager::isEOF()
	{
		if (mFileHandler.eof())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

void WorkLoadManager::setBeginOfFile()
{
	mFileHandler.seekg(0, mFileHandler.end);
}

void WorkLoadManager::readLine(std::string& text /*char* s, streamsize n*/)
{
	//mFileHandler.getline(s, n);
	std::getline(mFileHandler, text);
//	uint32_t pos = mFileHandler.tellg();
}

std::streamoff WorkLoadManager::tellPos()
{
	std::streamoff pos = mFileHandler.tellg();
	return pos;
}
#endif