/*******************************************************************
 * File : pciTbCommon.h
 *
 * Copyright(C) crossbar-inc. 
 * 
 * ALL RIGHTS RESERVED.
 *
 * Description: This file contains all the common defines and functions 
 * Related Specifications: 
 *
 ********************************************************************/
#ifndef _PCI_TB_COMMON_
#define _PCI_TB_COMMON_

#include <cstdint>
#include <string>

#define		BUS_WIDTH		32
#define		REG_32			4
#define		REG_64			8
#define     MEMSIZE			393216
#define		CAP_ADD			(BA_CNTL + 0x0000)
#define		VS_ADD			(BA_CNTL + 0x0008)
#define		INTMS_ADD		(BA_CNTL + 0x000C)
#define		INTMC_ADD		(BA_CNTL + 0x0010)
#define		CC_ADD			(BA_CNTL + 0x0014)
#define		RES_ADD			(BA_CNTL + 0x0018)
#define		CSTS_ADD		(BA_CNTL + 0x001C)
#define		NSSR_ADD		(BA_CNTL + 0x0020)
#define		AQA_ADD			(BA_CNTL + 0x0024)
#define		ASQ_ADD			(BA_CNTL + 0x0028)
#define		ACQ_ADD			(BA_CNTL + 0x0030)
#define		SQ0TDBL_ADD		(BA_CNTL + 0x1000 +  4)
#define		CQ0HDBL_ADD		(BA_CNTL + 0x1000 +  8)
#define		SQ1TDBL_ADD		(BA_CNTL + 0x1000 + 12)
#define		CQ1HDBL_ADD		(BA_CNTL + 0x1000 + 16)
	
#define		SET_FEATURE_COMMAND		0x9
#define		IDENTIFY_COMMAND		0x6
#define		CREATE_IO_CQ_COMMAND	0x5
#define		CREATE_IO_SQ_COMMAND	0x1
#define		WRITE_COMMAND			0x1
#define		READ_COMMAND			0x2
#define		SQ_WIDTH				64
#define		CQ_WIDTH				16
#define	    COMP_TLP_WIDTH			12

struct mMemWriteInfo{
	uint16_t length;
	uint32_t address;
	uint8_t* payload;
};
typedef mMemWriteInfo tMemWrInfo;

#endif