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

#ifndef TRANSACTION_H
#define TRANSACTION_H

//Transaction.h
//
//Header file for transaction object

#include "SystemConfiguration.h"
#include "BusPacket.h"
#include "DataPacket.h"

#include <map>
#include <list>




namespace DRAMSim
{

	using namespace std;

	class Transaction
	{
		Transaction();
	public:
		typedef enum
		{
			DATA_READ,
			DATA_WRITE,
			RETURN_DATA
		} TransactionType;

		//fields
		TransactionType transactionType;
		uint64_t address;
		size_t len;
		DataPacket *data;
		uint64_t timeAdded;
		uint64_t timeReturned;
		uint64_t timeTraced;
		//add on 20121030 by libing to record cache access time 
		uint64_t timeIssued ;
		//functions
		Transaction(TransactionType transType, uint64_t addr, DataPacket *data, size_t len=LEN_DEF, uint64_t time = 0);
		Transaction(const Transaction &t);

		void alignAddress();
		BusPacket::BusPacketType getBusPacketType();

		void print();
	};


#ifdef RETURN_TRANSACTIONS
	class TransactionReceiver
	{
		private:
			map<uint64_t, list<uint64_t> > pendingReadRequests;
			map<uint64_t, list<uint64_t> > pendingWriteRequests;
			unsigned counter;

		public:
			TransactionReceiver():counter(0){};

			void addPending(const Transaction *t, uint64_t cycle)
			{
				// C++ lists are ordered, so the list will always push to the back and
				// remove at the front to ensure ordering
				if (t->transactionType == Transaction::DATA_READ)
				{
					pendingReadRequests[t->address].push_back(cycle);
				}
				else if (t->transactionType == Transaction::DATA_WRITE)
				{
					pendingWriteRequests[t->address].push_back(cycle);
				}
				else
				{
					ERROR("This should never happen");
					exit(-1);
				}
				counter++;
			}

			void read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
			{
				map<uint64_t, list<uint64_t> >::iterator it;
				it = pendingReadRequests.find(address);
				if (it == pendingReadRequests.end())
				{
					ERROR("Cant find a pending read for this one");
					exit(-1);
				}
				else
				{
					if (it->second.size() == 0)
					{
						ERROR("Nothing here, either");
						exit(-1);
					}
				}

				uint64_t added_cycle = pendingReadRequests[address].front();
				uint64_t latency = done_cycle - added_cycle;

				pendingReadRequests[address].pop_front();
				//cout << "Read Callback:  0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
				counter--;
			}

			void write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
			{
				map<uint64_t, list<uint64_t> >::iterator it;
				it = pendingWriteRequests.find(address);
				if (it == pendingWriteRequests.end())
				{
					ERROR("Cant find a pending read for this one");
					exit(-1);
				}
				else
				{
					if (it->second.size() == 0)
					{
						ERROR("Nothing here, either");
						exit(-1);
					}
				}

				uint64_t added_cycle = pendingWriteRequests[address].front();
				uint64_t latency = done_cycle - added_cycle;

				pendingWriteRequests[address].pop_front();
				if (DEBUG_ADDR_MAP)
				{
				cout << "Write Callback: 0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
				}
				counter--;
			}

			bool pendingTrans()
			{
				return (counter==0)?false:true;
			}
	};
#endif

}

#endif

