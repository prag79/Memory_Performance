#include "TeraSPCIeControllerBase.h"
#include "reporting.h"
namespace CrossbarTeraSLib {

	/** Get Bank Mask
	* Generate Masks bits based on number of bits
	* @return uint32_t
	**/
	uint32_t TeraSPCIeControllerBase::getBankMask()
	{
		uint32_t bankMask = 0;

		for (uint8_t bitIndex = 0; bitIndex < mBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** Get Code word Bank Mask
	* Generate code word Masks bits based on number of bits
	* @return uint32_t
	**/
	uint32_t TeraSPCIeControllerBase::getCwBankMask()
	{
		uint32_t bankMask = 0;

		for (uint8_t bitIndex = 0; bitIndex < mCwBankMaskBits; bitIndex++)
			bankMask |= (0x1 << bitIndex);
		return bankMask;

	}

	/** Get page Mask
	* Generate code word Masks bits based on number of bits
	* @return uint32_t
	**/
	uint64_t TeraSPCIeControllerBase::getPageMask()
	{
		uint64_t pageMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < (mPageMaskBits + (uint32_t)(log2(mPageSize))); bitIndex++)
			pageMask |= ((uint64_t)0x1 << bitIndex);
		return pageMask;

	}

	/** Get chan Mask
	* Generate chan Masks bits based on number of bits
	* @return uint64_t
	**/
	uint64_t TeraSPCIeControllerBase::getChanMask()
	{
		uint64_t chanMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < (mChanMaskBits); bitIndex++)
			chanMask |= ((uint64_t)0x1 << bitIndex);
		return chanMask;

	}

	/** Get active page Mask
	* Generate active page Masks bits based on page mask bits
	* @return uint64_t
	**/
	uint64_t TeraSPCIeControllerBase::getActivePageMask()
	{
		uint64_t pageMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mPageMaskBits; bitIndex++)
			pageMask |= ((uint64_t)0x1 << bitIndex);
		return pageMask;

	}

	/** Get LBA Mask
	* Generate lba Masks bits based on number of bits
	* @return uint64_t
	**/
	uint64_t TeraSPCIeControllerBase::getLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}

	/** Get active LBA Mask
	* Generate active LBA Masks bits based on lba mask
	* @return uint64_t
	**/
	uint64_t TeraSPCIeControllerBase::getActiveLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mActiveLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}

	/** Get pending LBA Mask
	* Generate pending LBA Masks bits based on lba mask
	* @return uint64_t
	**/
	uint64_t TeraSPCIeControllerBase::getPendingLBAMask()
	{
		uint64_t lbaMask = 0;

		for (uint64_t bitIndex = 0; bitIndex < mPendingLBAMaskBits; bitIndex++)
			lbaMask |= ((uint64_t)0x1 << bitIndex);
		return lbaMask;

	}


	/** create Active command entry
	* @param cmd command payload
	* @param queueAddr queue address
	* @param queueVal active command value
	* @return void
	**/
	void TeraSPCIeControllerBase::createActiveCmdEntry(const cmdType cmd, const uint64_t plba, \
		const uint32_t cmdOffset, const uint32_t iTag, const uint32_t queueNum, const sc_time delay, ActiveCmdQueueData& queueVal)
	{
		queueVal.cmd = cmd;
		queueVal.cmdOffset = cmdOffset;
		queueVal.iTag = iTag;
		queueVal.plba = (uint64_t)((plba >> mChanMaskBits) & 0xfffffffffffffffful);
		queueVal.queueNum = queueNum;
		queueVal.time = delay;
	}

	/** create Active command entry
	* @param slotNum slot index
	* @param queueAddr queue address
	* @param ctype command type
	* @param lba activelba
	* @param cwCnt code word count
	* @param pageNum num of pages
	* @param cmdVal command value
	* @return void
	**/
	void TeraSPCIeControllerBase::createActiveCmdEntry(const uint16_t& slotNum, const uint8_t& queueAddr, \
		const cmdType& ctype, const uint8_t& lba, const uint16_t& cwCnt, \
		const uint32_t& pageNum, uint64_t& cmdVal)
	{
		cmdVal = (((uint64_t)slotNum & 0xff) << (mActiveLBAMaskBits + 19)) | \
			(((uint64_t)queueAddr & 0xff) << (mActiveLBAMaskBits + 11)) | \
			(((uint64_t)ctype & 0x3) << (mActiveLBAMaskBits + 9)) | \
			(((uint64_t)lba | 0x0000000000000000) << 9) | (cwCnt & 0x1ff);
	}


	/** decode command payload received
	* @param cmd command payload
	* @param slotNum slot index
	* @param cwCnt code word count
	* @param value completion entry value
	* @return void
	**/
	void TeraSPCIeControllerBase::decodeCommand(const ActiveCmdQueueData queueData, cmdType& ctype, \
		uint8_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (queueData.plba & getCwBankMask());
		chipSelect = ((queueData.plba) >> mCwBankMaskBits) & 0x1;
		page = ((queueData.plba) >> (mCwBankMaskBits + 1)) & getActivePageMask();
		ctype = queueData.cmd;
	}

	void TeraSPCIeControllerBase::decodeCommand(const ActiveDMACmdQueueData queueData, cmdType& ctype, \
		uint8_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (queueData.plba & getCwBankMask());
		chipSelect = ((queueData.plba) >> mCwBankMaskBits) & 0x1;
		page = ((queueData.plba) >> (mCwBankMaskBits + 1)) & getActivePageMask();
		ctype = queueData.cmd;
	}


	/** create pending command entry
	* @param cmd  active cmd
	* @param cmdVal pending command value
	* @return void
	**/
	void TeraSPCIeControllerBase::createPendingCmdQEntry(const ActiveDMACmdQueueData cmdInp, ActiveCmdQueueData& cmdOutp)
	{
		cmdOutp.cmd = cmdInp.cmd;
		cmdOutp.cmdOffset = cmdInp.cmdOffset;
		cmdOutp.iTag = cmdInp.iTag;
		cmdOutp.plba = cmdInp.plba;
		cmdOutp.queueNum = cmdInp.queueNum;
		cmdOutp.time = cmdInp.time;
	}



	void TeraSPCIeControllerBase::decodeLBA(const uint64_t plba, uint8_t& cwBank, bool& chipSelect, uint32_t& page)
	{
		cwBank = (uint8_t)((plba)& getCwBankMask());
		chipSelect = (bool)((plba >> (mCwBankMaskBits)) & 0x1);
		page = (uint32_t)((plba >> (mCwBankMaskBits + 1)) & getActivePageMask());
	}


	void TeraSPCIeControllerBase::decodePendingCmdEntry(ActiveCmdQueueData cmd, cmdType& ctype, \
		uint8_t& lba, uint32_t& iTag, bool& chipSelect, uint8_t& queueAddr)
	{
		ctype = cmd.cmd;
		lba = cmd.plba;
		chipSelect = (bool)((lba >> mCwBankMaskBits) & 0x1);
		queueAddr = cmd.queueNum;
		iTag = cmd.iTag;
	}


	/** append parameter in multi simulation
	* @param name name to be modified
	* @param mBlockSize block size
	* @param mQueueDepth queue depth of the host
	* @return void
	**/
	string TeraSPCIeControllerBase::appendParameters(string name, string format)
	{

		char temp[64];
		name.append("_iosize");
		_itoa(mIOSize, temp, 10);
		name.append("_");
		name.append(temp);
		name.append("_blksize");
		_itoa(mBlockSize, temp, 10);
		name.append("_");
		name.append(temp);
		name.append("_qd");
		_itoa(mQueueDepth, temp, 10);
		name.append("_");
		name.append(temp);
		name.append(format);
		return name;
	}

	uint16_t TeraSPCIeControllerBase::getActiveCwBankIndex(const uint64_t& lba)
	{
		uint16_t cwBank = (uint16_t)((lba)& getCwBankMask());
		return cwBank;
	}

	bool TeraSPCIeControllerBase::getActiveChipSelect(const uint64_t& lba)
	{
		return (bool)((lba >> (mCwBankMaskBits)& 0x1));
	}
}