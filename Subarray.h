/*
 * Subarray.h
 *
 *  Created on: Aug 20, 2012
 *      Author: shawn
 */

#ifndef SUBARRAY_H_
#define SUBARRAY_H_

#include "SystemConfiguration.h"
#include "BankState.h"
#include "BusPacket.h"
#include <map>


namespace DRAMSim
{
	class Subarray
	{
	public:
		//functions
		Subarray() {};
		void read(BusPacket *busPacket);
		void write(const BusPacket *busPacket);

		//fields
		BankState currentState;

	private:
		unsigned getByteOffsetInRow(const BusPacket *busPacket);
		typedef std::map<uint64_t, byte *> RowMapType;

		RowMapType rowEntries;
	};
}

#endif /* SUBARRAY_H_ */

