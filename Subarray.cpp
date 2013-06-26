/*
 * Subarray.cpp
 *
 *  Created on: Aug 20, 2012
 *      Author: shawn
 */

#include "Subarray.h"
#include "BusPacket.h"
#include <string.h> // for memcpy
#include <assert.h>

namespace DRAMSim
{
	using namespace std;

	unsigned Subarray::getByteOffsetInRow(const BusPacket *busPacket)
	{
		// This offset will simply be all of the bits in the unaligned address that
		// have been removed (i.e. the lower 6 bits for BL=8 and the lower 5 bits
		// for BL=4) plus the column offset

#ifdef DATA_RELIABILITY_ECC
		unsigned transactionSize = (ECC_DATA_BUS_BITS/8)*BL;
#else
		unsigned transactionSize = (JEDEC_DATA_BUS_BITS/8)*BL;
#endif
		uint64_t transactionMask =  transactionSize - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
		unsigned byteOffset = busPacket->data->getAddr() & transactionMask;
		unsigned columnOffset = (busPacket->column * DEVICE_WIDTH)/8;
		unsigned offset = columnOffset + byteOffset;

		DEBUG("[DPKT] "<< *(busPacket->data) << " \t r="<<busPacket->row<<" c="<<busPacket->column<<" byte offset="<< byteOffset<< "-> "<<offset);

		return offset;
	}

	void Subarray::write(const BusPacket *busPacket)
	{
		unsigned byteOffset = getByteOffsetInRow(busPacket);
		RowMapType::iterator it;
		it = rowEntries.find(busPacket->row);
		byte *rowData;
		if (it == rowEntries.end())
		{
			// row doesn't exist yet, allocate it
			rowData = (byte *)calloc((NUM_COLS*DEVICE_WIDTH)/8,sizeof(byte));
			rowEntries[busPacket->row] = rowData;
		}
		else
		{
			rowData = it->second;
		}

#ifdef DATA_RELIABILITY_ECC
		size_t transactionSize = ECC_DATA_BUS_BITS * BL / 8;
#else
		size_t transactionSize = busPacket->data->getNumBytes();
#endif

		// if we out of bound a row, this is a problem
		if (byteOffset + transactionSize > NUM_COLS*DEVICE_WIDTH)
		{
			ERROR("Transaction out of bounds a row, check alignment of the address");
			exit(-1);
		}
		// copy data to the row
		memcpy(rowData + byteOffset, busPacket->data->getData(), transactionSize);
	}


	void Subarray::read(BusPacket *busPacket)
	{
		assert(busPacket->data == NULL);

#ifdef DATA_RELIABILITY_ECC
		size_t transactionSize = ECC_DATA_BUS_BITS *BL /8;
#else
		size_t transactionSize = JEDEC_DATA_BUS_BITS * BL /8;
#endif

		busPacket->busPacketType = BusPacket::DATA;
		busPacket->data = new DataPacket(NULL, transactionSize, busPacket->physicalAddress);

		RowMapType::iterator it = rowEntries.find(busPacket->row);
		if (it != rowEntries.end())
		{
			byte *rowData = it->second;
			byte *dataBuf = (byte *)calloc(sizeof(byte), transactionSize);
			memcpy(dataBuf, rowData + (busPacket->column*DEVICE_WIDTH)/8, transactionSize);
			busPacket->data->setData(dataBuf, transactionSize, false);
			DEBUG("[DPKT] Rank returning: "<<*(busPacket->data));
		}
		else
		{
#ifdef RETURN_TRANSACTIONS
			byte *dataBuf = (byte *)calloc(sizeof(byte), transactionSize);
			busPacket->data->setData(dataBuf, transactionSize, false);
			DEBUG("[DPKT] Rank returning: "<<*(busPacket->data));
#endif
		}


	}

} // end of namespace DRAMSim




