/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/



#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include "PrintMacros.h"

#ifdef __APPLE__
#include <sys/types.h>
#endif

//SystemConfiguration.h
//


#define LEN_DEF 8
#define MS_BUFFER

#define RETURN_TRANSACTIONS
//#define DATA_STORAGE
//#define DATA_STORAGE_SSA
//#define DATA_RELIABILITY_ECC
//#define DATA_RELIABILITY_CHIPKILL

#ifdef DATA_STORAGE_SSA
	#define DATA_STORAGE
#endif

#ifdef DATA_RELIABILITY_CHIPKILL
	#define DATA_RELIABILITY_ECC
#endif

#ifdef DATA_RELIABILITY_ECC
	#define DATA_STORAGE
#endif


namespace DRAMSim
{

	typedef unsigned char byte;

	extern bool SHOW_SIM_OUTPUT;

	extern bool VERIFICATION_OUTPUT; // output suitable to feed to modelsim

	extern bool DEBUG_INI_READER;

	extern bool DEBUG_TRANS_Q;
	extern bool DEBUG_CMD_Q;
	extern bool DEBUG_ADDR_MAP;
	extern bool DEBUG_BANKSTATE;
	extern bool DEBUG_BUS;
	extern bool DEBUG_BANKS;
	extern bool DEBUG_POWER;
	extern bool USE_LOW_POWER;
	extern bool VIS_FILE_OUTPUT;

	extern uint64_t TOTAL_STORAGE;
	extern unsigned NUM_BANKS;
	extern unsigned NUM_RANKS;
	extern unsigned NUM_CHANS;
	extern unsigned NUM_ROWS;
	extern unsigned NUM_COLS;
	extern unsigned DEVICE_WIDTH;

	extern unsigned SUBARRAY_DATA_BYTES;
	extern unsigned TRANS_DATA_BYTES;

	//in nanoseconds
	extern unsigned REFRESH_PERIOD;
	extern float tCK;

	extern unsigned CL;
	extern unsigned AL;
	#define RL (CL+AL)
	#define WL (RL-1)
	extern unsigned BL;
	extern unsigned tRAS;
	extern unsigned tRCD;
	extern unsigned tRRD;
	extern unsigned tRC;
	extern unsigned tRP;
	extern unsigned tCCD;
	extern unsigned tRTP;
	extern unsigned tWTR;
	extern unsigned tWR;
	extern unsigned tRTRS;
	extern unsigned tRFC;
	extern unsigned tFAW;
	extern unsigned tCKE;
	extern unsigned tXP;

	extern unsigned tCMD;

	extern unsigned IDD0;
	extern unsigned IDD1;
	extern unsigned IDD2P;
	extern unsigned IDD2Q;
	extern unsigned IDD2N;
	extern unsigned IDD3Pf;
	extern unsigned IDD3Ps;
	extern unsigned IDD3N;
	extern unsigned IDD4W;
	extern unsigned IDD4R;
	extern unsigned IDD5;
	extern unsigned IDD6;
	extern unsigned IDD6L;
	extern unsigned IDD7;
	extern float Vdd;

	extern unsigned NUM_DEVICES;

	//same bank
	#define READ_TO_PRE_DELAY (AL+BL/2+max(((int)tRTP),2)-2)
	#define WRITE_TO_PRE_DELAY (WL+BL/2+tWR)
	#define READ_TO_WRITE_DELAY (RL+BL/2+tRTRS-WL)
	#define READ_AUTOPRE_DELAY (AL+tRTP+tRP)
	#define WRITE_AUTOPRE_DELAY (WL+BL/2+tWR+tRP)
	#define WRITE_TO_READ_DELAY_B (WL+BL/2+tWTR) //interbank
	#define WRITE_TO_READ_DELAY_R (WL+BL/2+tRTRS-RL) //interrank

	extern unsigned ECC_DATA_BUS_BITS;
	extern unsigned JEDEC_DATA_BUS_BITS;

	//Memory Controller related parameters
	extern unsigned TRANS_QUEUE_DEPTH;
	extern unsigned CMD_QUEUE_DEPTH;

	extern uint64_t EPOCH_LENGTH;
	extern unsigned HISTOGRAM_BIN_SIZE;

	extern unsigned TOTAL_ROW_ACCESSES;


	typedef enum
	{
		k6,
		k7,
		mase,
		pin,
		DGpin
	} TraceType;

	typedef enum
	{
		Scheme1,
		Scheme2,
		Scheme3,
		Scheme4,
		Scheme5,
		Scheme6,
		Scheme7
	} AddressMappingScheme;

	// used in MemoryController and CommandQueue
	typedef enum
	{
		OpenPage,
		ClosePage
	} RowBufferPolicy;

	// Only used in CommandQueue
	typedef enum
	{
		PerRank,
		PerRankPerBank
	} QueuingStructure;

	typedef enum
	{
		RankThenBankRoundRobin,
		BankThenRankRoundRobin
	} SchedulingPolicy;


	extern std::string ROW_BUFFER_POLICY;
	extern std::string SCHEDULING_POLICY;
	extern std::string ADDRESS_MAPPING_SCHEME;
	extern std::string QUEUING_STRUCTURE;

	extern RowBufferPolicy rowBufferPolicy;
	extern SchedulingPolicy schedulingPolicy;
	extern AddressMappingScheme addressMappingScheme;
	extern QueuingStructure queuingStructure;

	//
	//FUNCTIONS
	//

	unsigned inline dramsim_log2(unsigned value)
	{
		unsigned logbase2 = 0;
		unsigned orig = value;
		value>>=1;
		while (value>0)
		{
			value >>= 1;
			logbase2++;
		}
		if ((unsigned)1<<logbase2<orig)logbase2++;
		return logbase2;
	}
	inline bool isPowerOfTwo(unsigned long x)
	{
		return (1UL<<dramsim_log2(x)) == x;
	}

};


#endif

