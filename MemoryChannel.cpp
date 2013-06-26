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




//MemorySystem.cpp
//
//Class file for JEDEC memory system wrapper
//

#include "SimulatorIO.h"
#include "MemoryChannel.h"
#include "IniReader.h"
#include <unistd.h>
#include "Simulator.h"




namespace DRAMSim
{

	using namespace std;

	PowerCB MemoryChannel::ReportPower = NULL;

	MemoryChannel::MemoryChannel(unsigned id) : systemID(id), ReadDataDone(NULL),WriteDataDone(NULL)
	{
		PRINTN("MemoryChannel "<<systemID<<" :");

		//calculate the total storage based on the devices the user selected and the number of

		//calculate number of devices
		/************************
		  This code has always been problematic even though it's pretty simple. I'll try to explain it
		  for my own sanity.

		  There are two main variables here that we could let the user choose:
		  NUM_RANKS or TOTAL_STORAGE.  Since the density and width of the part is
		  fixed by the device ini file, the only variable that is really
		  controllable is the number of ranks. Users care more about choosing the
		  total amount of storage, but with a fixed device they might choose a total
		  storage that isn't possible. In that sense it's not as good to allow them
		  to choose TOTAL_STORAGE (because any NUM_RANKS value >1 will be valid).

		  However, users don't care (or know) about ranks, they care about total
		  storage, so maybe it's better to let them choose and just throw an error
		  if they choose something invalid.

		  A bit of background:

		  Each column contains DEVICE_WIDTH bits. A row contains NUM_COLS columns.
		  Each bank contains NUM_ROWS rows. Therefore, the total storage per DRAM device is:
				PER_DEVICE_STORAGE = NUM_ROWS*NUM_COLS*DEVICE_WIDTH*NUM_BANKS (in bits)

		 A rank *must* have a 64 bit output bus (JEDEC standard), so each rank must have:
				NUM_DEVICES_PER_RANK = 64/DEVICE_WIDTH
				(note: if you have multiple channels ganged together, the bus width is
				effectively NUM_CHANS * 64/DEVICE_WIDTH)

		If we multiply these two numbers to get the storage per rank (in bits), we get:
				PER_RANK_STORAGE = PER_DEVICE_STORAGE*NUM_DEVICES_PER_RANK = NUM_ROWS*NUM_COLS*NUM_BANKS*64

		Finally, to get TOTAL_STORAGE, we need to multiply by NUM_RANKS
				TOTAL_STORAGE = PER_RANK_STORAGE*NUM_RANKS (total storage in bits)

		So one could compute this in reverse -- compute NUM_DEVICES,
		PER_DEVICE_STORAGE, and PER_RANK_STORAGE first since all these parameters
		are set by the device ini. Then, TOTAL_STORAGE/PER_RANK_STORAGE = NUM_RANKS

		The only way this could run into problems is if TOTAL_STORAGE < PER_RANK_STORAGE,
		which could happen for very dense parts.
		*********************/

		// number of bytes per rank

#ifdef DATA_RELIABILITY
		NUM_DEVICES = ECC_DATA_BUS_BITS/DEVICE_WIDTH;
#else
		NUM_DEVICES = JEDEC_DATA_BUS_BITS/DEVICE_WIDTH;
#endif

		unsigned long megsOfStoragePerRank = ((((long long)NUM_ROWS * (NUM_COLS * DEVICE_WIDTH) * NUM_BANKS) * NUM_DEVICES) / 8) >> 20;
		TOTAL_STORAGE = (NUM_RANKS * megsOfStoragePerRank);

		PRINT("CH. " <<systemID<<" TOTAL_STORAGE : "<< TOTAL_STORAGE << "MB | "<<NUM_RANKS<<" Ranks | "<< NUM_DEVICES <<" Devices per rank");


		// TODO: change to other vector constructor?
		ranks = new vector<Rank *>();
		memoryController = new MemoryController(this,ranks);

		for (int i=0; i<NUM_RANKS; i++)
		{
			Rank *r = new Rank(i,memoryController);
			ranks->push_back(r);
		}
	}


	MemoryChannel::~MemoryChannel()
	{
		/* the MemorySystem should exist for all time, nothing should be destroying it */
	//	ERROR("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
	//	abort();

		delete(memoryController);

		for (size_t i=0; i<NUM_RANKS; i++)
		{
			delete (*ranks)[i];
		}
		ranks->clear();
		delete(ranks);

		if (VERIFICATION_OUTPUT)
		{
			SimulatorIO::verifyFile.flush();
			SimulatorIO::verifyFile.close();
		}
	}

	bool MemoryChannel::willAcceptTransaction()
	{
		return memoryController->willAcceptTransaction();
	}

	bool MemoryChannel::addTransaction(bool isWrite, uint64_t addr)
	{
		Transaction::TransactionType type = isWrite ? Transaction::DATA_WRITE : Transaction::DATA_READ;
		Transaction *trans = new Transaction(type,addr,NULL,Simulator::clockDomainCPU->clockcycle);

		// push_back in memoryController will make a copy of this during
		// addTransaction so it's kosher for the reference to be local

		if (memoryController->willAcceptTransaction())
		{
			return memoryController->addTransaction(trans);
		}
		else
		{
			pendingTransactions.push_back(trans);
			return true;
		}
	}

	bool MemoryChannel::addTransaction(Transaction *trans)
	{
#ifdef CHANNEL_BUFFER
		if (memoryController->willAcceptTransaction())
		{
			return memoryController->addTransaction(trans);
		}
		else
		{
			pendingTransactions.push_back(trans);
			return true;
		}
#else
		return memoryController->addTransaction(trans);
#endif
	}

	//prints statistics
	void MemoryChannel::printStats()
	{
		memoryController->printStats(true);
	}

	void MemoryChannel::printStats(bool)
	{
		printStats();
	}


	//update the memory systems state
	void MemoryChannel::update()
	{

		//PRINT(" ----------------- Memory System Update ------------------");

		//updates the state of each of the objects
		// NOTE - do not change order
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			(*ranks)[i]->update();
		}

		//pendingTransactions will only have stuff in it if MARSS is adding stuff or set CHANNEL_BUFFER mode
		if (pendingTransactions.size() > 0 && memoryController->willAcceptTransaction())
		{
			memoryController->addTransaction(pendingTransactions.front());
			pendingTransactions.pop_front();
		}
		memoryController->update();

		//PRINT("\n"); // two new lines
	}

	void MemoryChannel::registerCallbacks( TransactionCompleteCB* readCB, TransactionCompleteCB* writeCB,
										  void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower))
	{
		ReadDataDone = readCB;
		WriteDataDone = writeCB;
		ReportPower = reportPower;
	}




} /*namespace DRAMSim */


// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
	void libdramsim_is_present(void)
	{
		;
	}
}

