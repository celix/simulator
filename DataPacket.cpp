#include "DataPacket.h"
#include <stdio.h>

namespace DRAMSim
{

	ostream &operator<<(ostream &os, const DataPacket &dp)
	{
		if (dp.hasNoData())
		{
			os << "NO DATA";
			return os;
		}
		char tmp[8];
		os << "DATA: ("<< dp._numBytes << ")";
		for (size_t i=0; i < dp._numBytes; i++)
		{
			if (i % 32 == 0) os << "\n";
			// this is easier than trying to figure out how to do it with a stream
			snprintf(tmp, 8, "%02x ", dp._data[i]);
			os << tmp;
		}

		return os;
	}

	DataPacket::DataPacket() : _data(NULL), _numBytes(0), _unalignedAddr(0)	{}


	DataPacket::DataPacket(byte *data, size_t numBytes, uint64_t unalignedAddr) :
				_data(data), _numBytes(numBytes), _unalignedAddr(unalignedAddr)	{}

	DataPacket::~DataPacket()
	{
		if (_data) free(_data);
	}

	// accessors
	size_t DataPacket::getNumBytes() const
	{
		return _numBytes;
	}

	byte *DataPacket::getData() const
	{
		return _data;
	}

	uint64_t DataPacket::getAddr() const
	{
		return _unalignedAddr;
	}

	void DataPacket::setData(byte *data, size_t size, bool copy)
	{
		if (_data) free(_data);

		if (copy == true)
		{
			_data = (byte *)calloc(size,sizeof(byte));
			memcpy(_data, data, size);
		}
		else
		{
			_data = data;
		}
		_numBytes = size;
	}

	bool DataPacket::hasNoData() const
	{
		return (_data == NULL || _numBytes == 0);
	}

}
