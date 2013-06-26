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


#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

//MemoryController.h
//
//Header file for memory controller object
//
#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "Transaction.h"
#include "CommandQueue.h"
#include "BusPacket.h"
#include "BankState.h"
#include "Rank.h"
#include "CSVWriter.h"
#include <map>



namespace DRAMSim
{
	using namespace std;

	class MemorySystem;
	class MemoryController
	{
	public:
		//functions
		MemoryController(MemorySystem* ms,vector<Rank *> *ranks,unsigned channel);
		virtual ~MemoryController();

		bool addTransaction(Transaction *trans);
		void receiveFromBus(BusPacket *bpacket);
		void update();
		void printStats(bool finalStats = false);


		//fields
		vector<Transaction *> transactionQueue;

		// energy values are per rank -- SST uses these directly, so make these public
		vector< uint64_t > backgroundEnergy;
		vector< uint64_t > burstEnergy;
		vector< uint64_t > actpreEnergy;
		vector< uint64_t > refreshEnergy;
	private:
		MemorySystem *parentMemorySystem;
		vector<Rank *> *ranks;
		vector< vector <BankState> > bankStates;
		unsigned channelID;

		//output file
		CSVWriter csvOut;

		// params
		unsigned channelBitWidth;
		unsigned rankBitWidth;
		unsigned bankBitWidth;
		unsigned rowBitWidth;
		unsigned colBitWidth;
		unsigned byteOffsetWidth;

		unsigned refreshRank;

		// counters
		CommandQueue commandQueue;
		BusPacket *poppedBusPacket;
		vector<unsigned>refreshCountdown;
		vector<BusPacket *> writeDataToSend;
		vector<unsigned> writeDataCountdown;
		vector<Transaction *> returnTransaction;
		vector<Transaction *> pendingReadTransactions;
		map<unsigned,unsigned> latencies; // latencyValue -> latencyCount
		vector<bool> powerDown;

		// these packets are counting down waiting to be transmitted on the "bus"
		BusPacket *outgoingCmdPacket;
		unsigned cmdCyclesLeft;
		BusPacket *outgoingDataPacket;
		unsigned dataCyclesLeft;

		// statistics
		uint64_t totalTransactions;
		vector<uint64_t> grandTotalBankAccesses;
		vector<uint64_t> totalReadsPerBank;
		vector<uint64_t> totalWritesPerBank;
		vector<uint64_t> totalReadsPerRank;
		vector<uint64_t> totalWritesPerRank;
		vector<uint64_t> totalEpochLatency;

		struct CmdStat
		{
			CmdStat()
			{
				readCounter=0;
				readpCounter=0;
				writeCounter=0;
				writepCounter=0;
				activateCounter=0;
				prechangeCounter=0;
				refreshCounter=0;

				readSum=0;
				readpSum=0;
				writeSum=0;
				writepSum=0;
				activateSum=0;
				prechangeSum=0;
				refreshSum=0;
			}

			unsigned readCounter;
			unsigned readpCounter;
			unsigned writeCounter;
			unsigned writepCounter;
			unsigned activateCounter;
			unsigned prechangeCounter;
			unsigned refreshCounter;

			unsigned readSum;
			unsigned readpSum;
			unsigned writeSum;
			unsigned writepSum;
			unsigned activateSum;
			unsigned prechangeSum;
			unsigned refreshSum;

		} cmdStat;


		//functions
		void insertHistogram(unsigned latencyValue, unsigned rank, unsigned bank);
		void updateBankState();
		void updateCounter();
		void updateCmdQueue();
		void updateTransQueue();
		void updatePower();
		void updateReturnTrans();
		void updatePrint();
	};
}

#endif

