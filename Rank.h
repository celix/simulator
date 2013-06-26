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

#ifndef RANK_H
#define RANK_H

#ifdef DATA_STORAGE_SSA
	#include "Subarray.h"
#endif

#include "BusPacket.h"
#include "SystemConfiguration.h"
#include "Bank.h"
#include "BankState.h"

namespace DRAMSim
{
	using namespace std;

	class MemoryController;  //forward declaration

#ifndef DATA_STORAGE_SSA

	class Rank
	{
	private:
		bool isPowerDown;
		int id;
		unsigned incomingWriteBank;
		unsigned incomingWriteRow;
		unsigned incomingWriteColumn;

	public:
		//functions
		Rank(int id, MemoryController *mc);
		virtual ~Rank();
		void receiveFromBus(BusPacket *packet);
		int getId() const;
		void update();
		void powerUp();
		void powerDown();

		//fields
		bool refreshWaiting;
		unsigned dataCyclesLeft;
		MemoryController *memoryController;
		BusPacket *outgoingDataPacket;

		//these are vectors so that each element is per-bank
		vector<BusPacket *> readReturnPacket;
		vector<unsigned> readReturnCountdown;
		vector<BankState> bankStates;
		vector<Bank> banks;
	};

#else
	class Rank
	{
	private:
		bool isPowerDown;
		int id;
		unsigned incomingWriteBank;
		unsigned incomingWriteRow;
		unsigned incomingWriteColumn;

	public:
		//functions
		Rank(int id, MemoryController *mc);
		virtual ~Rank();
		void receiveFromBus(BusPacket *packet);
		int getId() const;
		void update();
		void powerUp();
		void powerDown();

		//fields
		bool refreshWaiting;
		unsigned dataCyclesLeft;
		MemoryController *memoryController;
		BusPacket *outgoingDataPacket;

		//these are vectors so that each element is per-bank
		vector<BusPacket *> readReturnPacket;
		vector<unsigned> readReturnCountdown;
		vector<BankState> bankStates;
		vector< vector<Subarray> > subarrays;
	};

#endif
} // end of namespace DRAMSim
#endif

