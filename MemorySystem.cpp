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
#include <errno.h> 
#include <unistd.h>

//#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "IniReader.h"
#include "SimulatorIO.h"
#include "Simulator.h"
#include "Callback.h"


namespace DRAMSim
{
	using namespace std;
	PowerCB MemorySystem::ReportPower=NULL;

	MemorySystem::MemorySystem(): ReadDataDone(NULL),WriteDataDone(NULL)
	{

#ifdef DATA_RELIABILITY_ECC
		//ECC BUS BITS
		JEDEC_DATA_BUS_BITS = JEDEC_DATA_BITS / BL;
		ECC_DATA_BUS_BITS = ECC_DATA_BITS / BL;
		NUM_DEVICES = ECC_DATA_BUS_BITS/DEVICE_WIDTH;
#else
		NUM_DEVICES = JEDEC_DATA_BUS_BITS / DEVICE_WIDTH;
#endif

#ifdef DATA_STORAGE_SSA
		TRANS_DATA_BYTES = SUBARRAY_DATA_BYTES * NUM_DEVICES;
#else
	#ifdef DATA_RELIABILITY_ECC
		TRANS_DATA_BYTES = ECC_DATA_BUS_BITS * BL / 8;
	#else
		TRANS_DATA_BYTES = JEDEC_DATA_BUS_BITS * BL / 8;
	#endif
#endif

		for (size_t iChannel=0; iChannel<NUM_CHANS; iChannel++)
		{
			unsigned long megsOfStoragePerRank = ( (long long)DEVICE_WIDTH * NUM_COLS * NUM_ROWS * NUM_BANKS * NUM_DEVICES / 8) >> 20;
			TOTAL_STORAGE = (NUM_RANKS * megsOfStoragePerRank);

			ranks.push_back(new vector<Rank *>());
			memoryControllers.push_back(new MemoryController(this,ranks[iChannel],iChannel));

			for (size_t iRank=0; iRank< NUM_RANKS; iRank++)
			{
				ranks[iChannel]->push_back(new Rank(iRank,memoryControllers[iChannel]));
			}

			PRINTN("MemoryChannel "<<iChannel<<" :");
			PRINT("CH. " <<iChannel<<" TOTAL_STORAGE : "<< TOTAL_STORAGE << "MB | "<<NUM_RANKS<<" Ranks | "<< NUM_DEVICES <<" Devices per rank");
		}
	}

	MemorySystem::~MemorySystem()
	{
		for (size_t iChannel=0; iChannel<NUM_CHANS; iChannel++)
		{
			delete(memoryControllers[iChannel]);

			vector<Rank *> *channelRank = ranks[iChannel];
			for (size_t iRank=0; iRank<NUM_RANKS; iRank++)
			{
				delete (*channelRank)[iRank];
			}
			channelRank->clear();
		}

		if (VERIFICATION_OUTPUT)
		{
			SimulatorIO::verifyFile.flush();
			SimulatorIO::verifyFile.close();
		}
	}

	void MemorySystem::update()
	{
		for (size_t iChannel=0; iChannel<NUM_CHANS; iChannel++)
		{
			for (size_t iRank=0;iRank<NUM_RANKS;iRank++)
			{
				(*ranks[iChannel])[iRank]->update();
			}

			if (pendingTransactions.size() > 0)
			{
				unsigned channelNum = findChannelNumber(pendingTransactions.front()->address);
				if (channelNum == iChannel && memoryControllers[iChannel]->addTransaction(pendingTransactions.front()))
				{
					pendingTransactions.pop_front();
				}
			}
			memoryControllers[iChannel]->update();
		}
	}

	unsigned MemorySystem::findChannelNumber(uint64_t addr)
	{
		// Single channel case is a trivial shortcut case
		if (NUM_CHANS == 1)
		{
			return 0;
		}

		if (!isPowerOfTwo(NUM_CHANS))
		{
			ERROR("We can only support power of two # of channels.\n" <<
					"I don't know what Intel was thinking, but trying to address map half a bit is a neat trick that we're not sure how to do");
			abort();
		}

		// only chan is used from this set
		unsigned iChannel,rank,bank,row,column;
		addressMapping(addr,iChannel,rank,bank,row,column);
		if (iChannel >= NUM_CHANS)
		{
			ERROR("Got channel index "<<iChannel<<" but only "<<NUM_CHANS<<" exist");
			abort();
		}
		//DEBUG("Channel idx = "<<channelNumber<<" totalbits="<<totalBits<<" channelbits="<<channelBits);

		return iChannel;
	}

	bool MemorySystem::addTransaction(Transaction *trans)
	{
		unsigned iChannel = findChannelNumber(trans->address);

#ifdef MS_BUFFER
		if (memoryControllers[iChannel]->addTransaction(trans))
		{
			return true;
		}
		else
		{
			pendingTransactions.push_back(trans);
			return true;
		}
#else
		return memoryControllers[iChannel]->addTransaction(trans);
#endif
	}


	bool MemorySystem::addTransaction(bool isWrite, uint64_t addr)
	{
		unsigned iChannel = findChannelNumber(addr);
		Transaction::TransactionType type = isWrite ? Transaction::DATA_WRITE : Transaction::DATA_READ;
		Transaction *trans = new Transaction(type,addr,NULL,LEN_DEF,Simulator::clockDomainCPU->clockcycle);

		// push_back in memoryController will make a copy of this during
		// addTransaction so it's kosher for the reference to be local

#ifdef MS_BUFFER
		if (memoryControllers[iChannel]->addTransaction(trans))
		{
			return true;
		}
		else
		{
			pendingTransactions.push_back(trans);
			return true;
		}
#else
		return memoryControllers[iChannel]->addTransaction(trans);
#endif
	}


	void MemorySystem::printStats()
	{
		for (size_t iChannel=0; iChannel<NUM_CHANS; iChannel++)
		{
			PRINT("==== Channel ["<<iChannel<<"] ====");
			memoryControllers[iChannel]->printStats(true);
			PRINT("//// Channel ["<<iChannel<<"] ////");
		}
	}


	void MemorySystem::registerCallbacks( TransactionCompleteCB* readCB, TransactionCompleteCB* writeCB,
										  void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower))
	{
		ReadDataDone = readCB;
		WriteDataDone = writeCB;
		ReportPower = reportPower;
	}

	void MemorySystem::addressMapping(
			uint64_t physicalAddress,
			unsigned &chan,
			unsigned &rank,
			unsigned &bank,
			unsigned &row,
			unsigned &col)
	{
		uint64_t tempA, tempB;

		uint64_t transactionMask =  TRANS_DATA_BYTES - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
		unsigned  channelBitWidth = dramsim_log2(NUM_CHANS);
		unsigned	 rankBitWidth = dramsim_log2(NUM_RANKS);
		unsigned	 bankBitWidth = dramsim_log2(NUM_BANKS);
		unsigned	  rowBitWidth = dramsim_log2(NUM_ROWS);
		unsigned	  colBitWidth = dramsim_log2(NUM_COLS);
		// this forces the alignment to the width of a single burst (64 bits = 8 bytes = 3 address bits for DDR parts)
		unsigned	byteOffsetWidth = dramsim_log2((JEDEC_DATA_BUS_BITS/8));
		// Since we're assuming that a request is for BL*BUS_WIDTH, the bottom bits
		// of this address *should* be all zeros if it's not, issue a warning

		if ((physicalAddress & transactionMask) != 0)
		{
			DEBUG("WARNING: address 0x"<<std::hex<<physicalAddress<<std::dec<<" is not aligned to the request size of "<<TRANS_DATA_BYTES);
		}

		// each burst will contain JEDEC_DATA_BUS_BITS/8 bytes of data, so the bottom bits (3 bits for a single channel DDR system) are
		// 	thrown away before mapping the other bits
		physicalAddress >>= byteOffsetWidth;

		// The next thing we have to consider is that when a request is made for a
		// we've taken into account the granulaity of a single burst by shifting
		// off the bottom 3 bits, but a transaction has to take into account the
		// burst length (i.e. the requests will be aligned to cache line sizes which
		// should be equal to transactionSize above).
		//
		// Since the column address increments internally on bursts, the bottom n
		// bits of the column (colLow) have to be zero in order to account for the
		// total size of the transaction. These n bits should be shifted off the
		// address and also subtracted from the total column width.
		//
		// I am having a hard time explaining the reasoning here, but it comes down
		// this: for a 64 byte transaction, the bottom 6 bits of the address must be
		// zero. These zero bits must be made up of the byte offset (3 bits) and also
		// from the bottom bits of the column
		//
		// For example: cowLowBits = log2(64bytes) - 3 bits = 3 bits
		unsigned colLowBitWidth = dramsim_log2(TRANS_DATA_BYTES) - byteOffsetWidth;

		physicalAddress >>= colLowBitWidth;
		unsigned colHighBitWidth = colBitWidth - colLowBitWidth;
		/*if (DEBUG_ADDR_MAP)
		{
			DEBUG("Bit widths: channel:"<<channelBitWidth<<
							 " rank:"<<rankBitWidth<<
							 " bank:"<<bankBitWidth<<
							 " row:"<<rowBitWidth<<
							 " colLow:"<<colLowBitWidth<<
							 " colHigh:"<<colHighBitWidth<<
							 " off:"<<byteOffsetWidth<<
							 " Total:"<< (channelBitWidth + rankBitWidth + bankBitWidth + rowBitWidth + colLowBitWidth + colHighBitWidth + byteOffsetWidth));
		}*///commented by libing 2013-4-22

		//perform various address mapping schemes
		if (addressMappingScheme == Scheme1)
		{
			//chan:rank:row:col:bank
			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;
		}
		else if (addressMappingScheme == Scheme2)
		{
			//chan:row:col:bank:rank
			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;

		}
		else if (addressMappingScheme == Scheme3)
		{
			//chan:rank:bank:col:row
			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;

		}
		else if (addressMappingScheme == Scheme4)
		{
			//chan:rank:bank:row:col
			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;

		}
		else if (addressMappingScheme == Scheme5)
		{
			//chan:row:col:rank:bank

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;


		}
		else if (addressMappingScheme == Scheme6)
		{
			//chan:row:bank:rank:col

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;


		}
		// clone of scheme 5, but channel moved to lower bits
		else if (addressMappingScheme == Scheme7)
		{
			//row:col:rank:bank:chan
			tempA = physicalAddress;
			physicalAddress = physicalAddress >> channelBitWidth;
			tempB = physicalAddress << channelBitWidth;
			chan = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> bankBitWidth;
			tempB = physicalAddress << bankBitWidth;
			bank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rankBitWidth;
			tempB = physicalAddress << rankBitWidth;
			rank = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> colHighBitWidth;
			tempB = physicalAddress << colHighBitWidth;
			col = tempA ^ tempB;

			tempA = physicalAddress;
			physicalAddress = physicalAddress >> rowBitWidth;
			tempB = physicalAddress << rowBitWidth;
			row = tempA ^ tempB;

		}

		else
		{
			ERROR("== Error - Unknown Address Mapping Scheme");
			exit(-1);
		}
		/*if (DEBUG_ADDR_MAP)
		{
			DEBUG("Mapped Ch="<<chan<<" Rank="<<rank<<
				  " Bank="<<bank<<" Row="<<row<<" Col="<<col<<"\n");
		}*///commented by libing 2013-4-22
	}


} // end of DRAMSim


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
