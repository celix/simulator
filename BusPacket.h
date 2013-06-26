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

#ifndef BUSPACKET_H
#define BUSPACKET_H

#include <bitset>
#include "SystemConfiguration.h"
#include "DataPacket.h"

namespace DRAMSim
{
	class BusPacket
	{
		BusPacket();
	public:
		typedef enum
		{
			READ,
			READ_P,
			WRITE,
			WRITE_P,
			ACTIVATE,
			PRECHARGE,
			REFRESH,
			DATA
		} BusPacketType;

		//Fields
		BusPacketType busPacketType;
		uint64_t physicalAddress;

		unsigned rank;
		unsigned bank;
		unsigned row;
		unsigned column;

		size_t len;

		//void *data;
		DataPacket *data;

		//Functions
		BusPacket(BusPacketType packtype, unsigned rk, unsigned bk=0, unsigned rw=0, unsigned col=0, uint64_t physicalAddr=0, DataPacket *dat=NULL, size_t len=LEN_DEF);

		void print();
		void print(uint64_t currentClockCycle, bool dataStart);
		void printData() const;


#ifdef DATA_RELIABILITY_ECC

	typedef enum
	{
		ENCODE,
		DECODE,
		CHECK,
		CORRECTION
	} RELIABLE_OP;


#ifdef DATA_RELIABILITY_CHIPKILL
	void CHIPKILL(RELIABLE_OP op);

	#define ECC_WORD_BITS 72
	#define ECC_CHECK_BITS 8
	#define ECC_DATA_BITS 2304			//(ECC_DATA_BUS_BITS * BL)
	#define JEDEC_DATA_BITS 2048		//(JEDEC_DATA_BUS_BITS * BL)
#else
	#define ECC_CHECK_BITS 8
	#define ECC_DATA_BITS 576		//(ECC_DATA_BUS_BITS * BL)
	#define JEDEC_DATA_BITS 512			//(JEDEC_DATA_BUS_BITS * BL)
#endif


	#define POSITION_REVISE(i) ((int)pow(2.0, i) - i - 2)
	#define CHECKBIT_POSITION(i) ((int)pow(2.0, i) - 1)

	void DATA_ENCODE();
	void DATA_DECODE();

	bool DATA_CHECK();
	bool DATA_CORRECTION();

	bool ECC_HAMMING_SECDED(RELIABLE_OP eccop, int n = 72, int m = 64);



	byte *BitstoByteArray(bitset<JEDEC_DATA_BITS> &bits);
	byte *BitstoByteArray(bitset<ECC_DATA_BITS> &bits);


#define BitsfromByteArray(BITS,LEN)		 		\
		byte *bytes = data->getData();			\
		for (int i=0; i<LEN*8; i++)				\
		{										\
			if ((bytes[i/8]&(1<<(7-i%8))) > 0)	\
			{									\
				BITS.set(i);					\
			}									\
		}

#endif

	};
}

#endif

