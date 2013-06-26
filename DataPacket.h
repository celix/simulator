#ifndef DATA_PACKET_H
#define DATA_PACKET_H


#include <string.h> //memcpy
#include <stdint.h>
#include <cstdlib> //free
#include <iostream>
//#include <bitset>
#include "SystemConfiguration.h"

namespace DRAMSim
{
	using namespace std;

	class DataPacket
	{
	public:
		DataPacket();
		DataPacket(byte *data, size_t numBytes, uint64_t unalignedAddr);
		~DataPacket();

		uint64_t getAddr() const;
		size_t getNumBytes() const;
		size_t getNumSubarray() const;
		byte *getData() const;
		void setData(byte *data, size_t size, bool copy = false);
		bool hasNoData() const;

		friend ostream &operator<<(ostream &os, const DataPacket &dp);

	private:
		// Disable copying for a datapacket
		DataPacket(const DataPacket &other);
		byte *_data;
		size_t _numBytes;
		uint64_t _unalignedAddr;
	};


} // namespace DRAMSim

#endif

