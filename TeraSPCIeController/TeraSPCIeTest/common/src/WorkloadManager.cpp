#include "WorkLoadManager.h"

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