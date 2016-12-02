#include "FileManager.h"
#include "reporting.h"
const char* mfilename = "file.log";

/** readMemory()
* Reads data from file
* @param bankNum - Number of Banks
* @param pageAddress - Page Address
* @param dataPtr     - Data Ptr
* @return void
**/
void FileManager::readMemory(const uint8_t& bankNum, \
	const uint32_t& pageAddress, uint8_t* dataPtr, const uint8_t& numDie, \
	const uint8_t& chanNum)
{
	if (!mMemFile.is_open()){
		cout << "FileManager:readMemory: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint64_t fileIndex = (pageAddress * mBankNum * mNumDie + \
		chanNum * mBankNum * mNumDie * mPageNum + numDie * mBankNum + bankNum) * mPageSize;
	mMemFile.seekg(fileIndex, mMemFile.beg);
	mMemFile.read((char*)dataPtr, mPageSize);
}

/** writeMemory()
* Reads data from file
* @param bankNum - Number of Banks
* @param pageAddress - Page Address
* @param dataPtr     - Data Ptr
* @return void
**/
void FileManager::writeMemory(const uint8_t& bankNum, const uint32_t& pageAddress, \
	uint8_t* dataPtr, const uint8_t& numDie, const uint8_t& chanNum)
{
	if (!mMemFile.is_open()){
		cout << "FileManager:writeMemory: File open error" << endl;
		std::exit(EXIT_FAILURE);
	}
	uint64_t startFileIndex = (pageAddress * mBankNum * mNumDie + \
		chanNum * mBankNum * mNumDie * mPageNum + numDie * mBankNum + bankNum)* mPageSize;
	mMemFile.seekp(startFileIndex, mMemFile.beg);
	mMemFile.write((char*)dataPtr, mPageSize);
}

void FileManager::initFile()
{
	mMemFile.open("MemoryFile.dat", std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::binary);
	std::ostringstream msg;

	uint8_t* data = new uint8_t[mBlockSize];
	
	memset(data, 0x00, mBlockSize);

	if (mMemFile)
	{
		for (uint64_t i = 0; i < (mSize); i += mBlockSize)
		{
			mMemFile.seekp(i, mMemFile.beg);
			mMemFile.write((char*)data, mBlockSize);
			
		}
	}
	delete[] data;
	
}

/** closeFile()
* closes opened file
* @return void
**/
void FileManager::closeFile()
{
	if (mMemFile.is_open())
	{
		mMemFile.close();
	}
}
