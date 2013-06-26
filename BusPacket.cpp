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


//BusPacket.cpp
//
//Class file for bus packet object
//
#include "SimulatorIO.h"
#include "BusPacket.h"
#include <cmath>


namespace DRAMSim
{
	using namespace std;

	BusPacket::BusPacket(BusPacketType packtype, unsigned rk, unsigned bk, unsigned rw, unsigned col, uint64_t physicalAddr, DataPacket *dat, size_t len) :
		busPacketType(packtype),
		physicalAddress(physicalAddr),
		rank(rk),
		bank(bk),
		row(rw),
		column(col),
		data(dat),
		len(len) {}

	void BusPacket::print(uint64_t currentClockCycle, bool dataStart)
	{
		if (this == NULL)
		{
			return;
		}

		if (VERIFICATION_OUTPUT)
		{
			switch (busPacketType)
			{
			case READ:
				SimulatorIO::verifyFile << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",0);"<<endl;
				break;
			case READ_P:
				SimulatorIO::verifyFile << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",1);"<<endl;
				break;
			case WRITE:
				SimulatorIO::verifyFile << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",0 , 0, 'h0);"<<endl;
				break;
			case WRITE_P:
				SimulatorIO::verifyFile << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",1, 0, 'h0);"<<endl;
				break;
			case ACTIVATE:
				SimulatorIO::verifyFile << currentClockCycle <<": activate (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case PRECHARGE:
				SimulatorIO::verifyFile << currentClockCycle <<": precharge (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case REFRESH:
				SimulatorIO::verifyFile << currentClockCycle <<": refresh (" << rank << ");"<<endl;
				break;
			case DATA:
				//TODO: data verification?
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
			}
		}
	}
	void BusPacket::print()
	{
		if (this == NULL) //pointer use makes this a necessary precaution
		{
			return;
		}
		else
		{
			switch (busPacketType)
			{
			case READ:
				PRINT("BP [READ] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case READ_P:
				PRINT("BP [READ_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case WRITE:
				PRINT("BP [WRITE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case WRITE_P:
				PRINT("BP [WRITE_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case ACTIVATE:
				PRINT("BP [ACT] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case PRECHARGE:
				PRINT("BP [PRE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case REFRESH:
				PRINT("BP [REF] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case DATA:
				PRINTN("BP [DATA] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"] data["<<data<<"]=");
				if (data == (void*)(0x680a80) )
				{
					int helloworld=0;
					helloworld++;
				}
				printData();
				PRINT("");
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
			}
		}
	}

	void BusPacket::printData() const
	{
		if (data == NULL)
		{
			PRINTN("NO DATA");
			return;
		}
		PRINTN("'" << hex);
		for (int i=0; i < 4; i++)
		{
			PRINTN(((uint64_t *)data)[i]);
		}
		PRINTN("'" << dec);
	}


#ifdef DATA_RELIABILITY_ECC


	void BusPacket::DATA_ENCODE()
	{
		ECC_HAMMING_SECDED(ENCODE);

#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(ENCODE);
#endif
	}

	void BusPacket::DATA_DECODE()
	{
#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(DECODE);
#endif

		ECC_HAMMING_SECDED(DECODE);
	}

	bool BusPacket::DATA_CHECK()
	{
#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(DECODE);
#endif

		bool result = ECC_HAMMING_SECDED(CHECK);

#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(ENCODE);
#endif
		return result;
	}

	bool BusPacket::DATA_CORRECTION()
	{
#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(DECODE);
#endif

		bool result = ECC_HAMMING_SECDED(CORRECTION);

#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(ENCODE);
#endif
		return result;
	}


	bool BusPacket::ECC_HAMMING_SECDED(RELIABLE_OP eccop, int n, int m)
	{
		if (data->getData() == NULL) return true;

		int c = n - m;
		int dataBytes = JEDEC_DATA_BUS_BITS * BL / 8;
		int eccDataBytes = ECC_DATA_BUS_BITS * BL / 8;
		int l = eccDataBytes*8 / n;
		int r = eccDataBytes*8 % n;
		l = (r == 0) ? l : (l + 1);

		switch (eccop)
		{
		case ENCODE:
		{
			bitset<JEDEC_DATA_BITS> plainDataBits;
			bitset<ECC_DATA_BITS> eccDataBits;
			bitset<ECC_CHECK_BITS> eccCheckBits;
			BitsfromByteArray(plainDataBits,dataBytes);

			for (int iLoop=0; iLoop < l; iLoop++)
			{
				eccCheckBits.reset();
				for (int iCheck = 0; iCheck < c-1; iCheck++)
				{
					int checkBitCounter = (iCheck == 0) ? (2) : (iCheck + 1);
					int positionIncrement = pow(2.0, iCheck);
					int iCounter = 1;
					int iData = POSITION_REVISE(iCheck) + iCounter;

					bool first = true;

					while (iData < m - 1)
					{
						if (first == true)
						{
							eccCheckBits[iCheck] = plainDataBits[iLoop*m + iData];
							first = false;
						}
						else
						{
							eccCheckBits[iCheck] = eccCheckBits[iCheck]^plainDataBits[iLoop*m + iData];
						}

						iData++;
						iCounter++;
						if (iCounter % positionIncrement == 0)
						{
							iData += positionIncrement;
							iCounter = 0;
						}
						while ( iData > POSITION_REVISE(checkBitCounter) )
						{
							checkBitCounter++;
							iData--;
						}
					}
				} // end of iCheck

				for (int iData = 0; iData < n-1; iData++)
				{
					int iCheck = iData - m;
					if (iCheck < 0)
					{
						eccDataBits[iLoop*n + iData] = plainDataBits[iLoop*m + iData];
					}
					else
					{
						eccDataBits[iLoop*n + iData] = eccCheckBits[iCheck];
					}

					if (iData == 0)
					{
						eccDataBits[iLoop*n + (n-1)] = eccDataBits[iLoop*n + iData];
					}
					else
					{
						eccDataBits[iLoop*n + (n-1)] = eccDataBits[iLoop*n + (n-1)] ^ eccDataBits[iLoop*n + iData];
					}
				}

			} // end of iLoop
			data->setData(BitstoByteArray(eccDataBits), eccDataBytes, false);

			return true;
			break;
		}
		case DECODE:
		{
			bitset<ECC_DATA_BITS> eccDataBits;
			bitset<JEDEC_DATA_BITS> plainDataBits;
			BitsfromByteArray(eccDataBits,eccDataBytes);

			for (int iLoop = 0; iLoop < l; iLoop++)
			{
				for (int iData = 0; iData < m; iData++)
				{
					plainDataBits[iLoop*m + iData] = eccDataBits[iLoop*n + iData];
				}
			}
			data->setData(BitstoByteArray(plainDataBits), dataBytes, false);

			return true;
			break;
		}
		case CHECK:
		case CORRECTION:
		{
			int wordErrorNum = 0;
			int dataErrorNum = 0;
			bool noError = true;
			bool killError = true;

			bitset<ECC_CHECK_BITS> eccCheckBits;
			bitset<ECC_DATA_BITS> eccDataBits;
			BitsfromByteArray(eccDataBits,eccDataBytes);


			for (int iLoop=0; iLoop < l; iLoop++)
			{
				int iErrorPosition = 0;

				for (int iCheck = 0; iCheck < c; iCheck++)
				{
					eccCheckBits[iCheck] = eccDataBits[iLoop*n + m + iCheck];
					iErrorPosition = (eccCheckBits[iCheck] == 1) ? (iErrorPosition + pow(2.0,iCheck)) : iErrorPosition;
				}

				if (eccCheckBits[c-1] == 0 && iErrorPosition == 0)
				{
					wordErrorNum = 0;
				}
				else if (eccCheckBits[c-1] == 1 && iErrorPosition != 0)
				{
					noError = false;
					wordErrorNum = 1;
				}
				else if (eccCheckBits[c-1] == 0 && iErrorPosition != 0)
				{
					noError = false;
					killError = false;
					wordErrorNum = 2;
				}
				else if (eccCheckBits[c-1] == 1 && iErrorPosition == 0)
				{
					noError = false;
					killError = false;
					wordErrorNum = 3;
				}
				else
				{
					noError = false;
					killError = false;
					wordErrorNum = 3;
				}

				if (eccop == CORRECTION && wordErrorNum == 1)
				{
					if (isPowerOfTwo(iErrorPosition))
					{
						iErrorPosition = m + dramsim_log2(iErrorPosition);
					}
					else
					{
						int checkBitCounter = 0;
						while (--iErrorPosition > POSITION_REVISE(checkBitCounter) )
						{
							checkBitCounter++;
						}
					}
					eccDataBits[iLoop*n + iErrorPosition].flip();
				}

				dataErrorNum += wordErrorNum;
				wordErrorNum = 0;
			}

			if (eccop == CHECK)
				return noError;
			else
				if (eccop == CORRECTION)
					data->setData(BitstoByteArray(eccDataBits), eccDataBytes, false);
				return killError;

			break;
		}
		default:
			DEBUG("Invalid ECC Operation Type");
			return false;
			break;
		}
		DEBUG("Unexpected Executing Location");
		return false;
	}


	byte *BusPacket::BitstoByteArray(bitset<JEDEC_DATA_BITS> &bits)
	{
		int len=bits.size()/8;
		byte *bytes = (byte *)calloc(len, sizeof(byte));
		for (int j=0;j<len;++j)
		{
			bytes[j]=0;
		}
		for (size_t i=0; i<bits.size(); i++)
		{
			if (bits.test(i))
			{
				bytes[i/8] |= 1<<(7-i%8);
			}
		}
		return bytes;
	}


	byte *BusPacket::BitstoByteArray(bitset<ECC_DATA_BITS> &bits)
	{
		int len=bits.size()/8;
		byte *bytes = (byte *)calloc(len, sizeof(byte));
		for (int j=0;j<len;++j)
		{
			bytes[j]=0;
		}
		for (size_t i=0; i<bits.size(); i++)
		{
			if (bits.test(i))
			{
				bytes[i/8] |= 1<<(7-i%8);
			}
		}
		return bytes;
	}

#ifdef DATA_RELIABILITY_CHIPKILL
	void BusPacket::CHIPKILL(RELIABLE_OP op)
	{
		if (data->getData() == NULL) return;

		int chipkillDataBytes = ECC_DATA_BITS / 8;
		int loop = BL;

		switch (op)
		{
		case ENCODE:
		{
			bitset<ECC_DATA_BITS> chipkillDataBits;
			bitset<ECC_DATA_BITS> eccDataBits;
			BitsfromByteArray(eccDataBits,chipkillDataBytes);

			for (int iLoop=0; iLoop < loop; iLoop++)
			{
				for (int iWord = 0; iWord < (int)(DEVICE_WIDTH); iWord++)
				{
					for (int iData = 0; iData < ECC_WORD_BITS; iData++)
					{
						chipkillDataBits[iData*DEVICE_WIDTH + iWord] = eccDataBits[iWord*ECC_WORD_BITS + iData];
					}
				}
			}
			data->setData(BitstoByteArray(chipkillDataBits), chipkillDataBytes, false);
			break;
		}
		case DECODE:
		{
			bitset<ECC_DATA_BITS> chipkillDataBits;
			bitset<ECC_DATA_BITS> eccDataBits;
			BitsfromByteArray(chipkillDataBits,chipkillDataBytes);

			for (int iLoop=0; iLoop < loop; iLoop++)
			{
				for (int iWord = 0; iWord < (int)(DEVICE_WIDTH); iWord++)
				{
					for (int iData = 0; iData < ECC_WORD_BITS; iData++)
					{
						eccDataBits[iWord*ECC_WORD_BITS + iData] = chipkillDataBits[iData*DEVICE_WIDTH + iWord];
					}
				}
			}
			data->setData(BitstoByteArray(eccDataBits), chipkillDataBytes, false);
			break;
		}
		case CHECK:
		case CORRECTION:
			break;
		} // end of switch

	}
#endif

#endif

} // end of DRAMSim
